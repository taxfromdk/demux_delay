#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <queue>

#define __STDC_CONSTANT_MACROS

extern "C" {
	#include <libavutil/avassert.h>
	#include <libavutil/channel_layout.h>
	#include <libavutil/opt.h>
	#include <libavutil/mathematics.h>
	#include <libavutil/timestamp.h>
	#include <libavformat/avformat.h>
	#include <libswscale/swscale.h>
	#include <libswresample/swresample.h>
	#include <libavutil/file.h>
}


using namespace std;

static AVFormatContext *fmt_ctx = NULL;

struct Packet{
	uint8_t* data;
	uint64_t len;
};

queue<Packet*> Q;

uint64_t bytes_read = 0;

static int read_packet(void *opaque, uint8_t *buf, int buf_size)
{
	int r = 0;

	
	if(Q.size() > 0)
	{
		Packet* p = Q.front();
		if(p->len > buf_size)
		{
			//printf("Cut\r\n");
			memcpy(buf, p->data, buf_size);
			p->len -= buf_size;
			uint8_t* new_data = (uint8_t*)malloc(p->len);
			memcpy(new_data, &p->data[buf_size], p->len);
			free(p->data);
			p->data = new_data;
			r = buf_size; 
		}
		else
		{
			//printf("Full\r\n");
			memcpy(buf, p->data, p->len);
			free(p->data);
			r = p->len;
			Q.pop();
			free(p);
		}
		bytes_read+=r;
		return r;
	}
	return 0;
}

void usage()
{
	printf("Usage example:  ./main input.ts\r\n");
}

int main(int argc, char* argv[])
{
	int ret;

	if(argc != 2)
	{
		usage();
		return 0;
	}

	//Read file data into queue packets
	FILE* f = fopen(argv[1], "rb");
	if(f == NULL)
	{
		printf("Error opening %s\r\n", argv[1]);
		return 1; 
	}
	#define BUF_SIZE 10240
	uint8_t b[BUF_SIZE];
	while(1)
	{
		int l = fread(b, 1, BUF_SIZE, f);
		if(l <= 0)
		{
			break;
		}
		Packet* p = (Packet*)malloc(sizeof(Packet));
		p->data = (uint8_t*)malloc(l);
		memcpy((void*)p->data, (void*)b, l);
		p->len = l;
		Q.push(p);
	}
	fclose(f);
	
	printf("%s read into %d chunks\r\n", argv[1], Q.size());

	printf("We are using libavformat version: %d\r\n", avformat_version());

	
	if (!(fmt_ctx = avformat_alloc_context())) {
        fprintf(stderr, "Could not allocate fmt_ctx\n");
        exit(1);
    }

	size_t avio_ctx_buffer_size = 4096;
	uint8_t* avio_ctx_buffer = (uint8_t*)av_malloc(avio_ctx_buffer_size);
    if (!avio_ctx_buffer) {
        printf("error allocating avio_ctx_buffer\r\n");
        exit(0);
    }
	AVIOContext* avio_ctx = avio_alloc_context(avio_ctx_buffer, avio_ctx_buffer_size, 0,  NULL, &read_packet, NULL, NULL);
	if (!avio_ctx) {
        printf("error allocating avio_ctx\r\n");
        exit(0);
    }
	fmt_ctx->pb = avio_ctx;
	//custom io

	if (avformat_open_input(&fmt_ctx, NULL, NULL, NULL) < 0) {
        fprintf(stderr, "Could not open source\n");
        exit(1);
    }
	
	
	//av_dump_format(fmt_ctx, 0, 0, 0);

	//exit(0);

	AVPacket pkt;
	av_init_packet(&pkt);
    pkt.data = NULL;
    pkt.size = 0;
	int packages = 0;
    while (av_read_frame(fmt_ctx, &pkt) >= 0) {
		
		printf("Package returned after %llu bytes read. It has StreamIndex: %d and is %d bytes long.\r\n", bytes_read, pkt.stream_index, pkt.size);
		if(pkt.stream_index == 1)
		{
			printf("First video package returned after having read %llu bytes.\r\n", bytes_read);
			break;
		}
		
		
		av_packet_unref(&pkt);
		
	}
	avformat_close_input(&fmt_ctx);
    av_freep(&avio_ctx->buffer);
    av_freep(&avio_ctx);
	return 0;
}