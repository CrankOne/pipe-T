all: example1 example2 example3

example1: example_1.cpp
	g++ -Wall $^ -o $@

example2: example_2.cpp
	g++ -Wall $^ -o $@

example3: example_3.cpp
	g++ -Wall $^ -o $@

clean:
	rm -f example1 example2 example3

.PHONY: all clean
