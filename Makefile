main: main.cpp
	g++ main.cpp -o main -lavformat -lavutil -lavcodec -lswresample

.PHONY: clean test test_customdata test_plain

test_plain: main plain.ts
	@echo "Testing with Plain"
	./main plain.ts

test_customdata: main customdata.ts
	@echo "Testing with Customdata"
	./main customdata.ts

test: test_plain test_customdata

clean:
	rm -rf main