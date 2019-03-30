CXXFLAGS=-g -Wall

all: example1 example2 example3

example1: example_1.cpp
	g++ $(CXXFLAGS) $^ -o $@

example2: example_2.cpp
	g++ $(CXXFLAGS) $^ -o $@

example3: example_3.cpp
	g++ $(CXXFLAGS) $^ -o $@

clean:
	rm -f example1 example2 example3

.PHONY: all clean
