CXXFLAGS=-g -Wall

all: example1 example2 example3 example4

example1: example_1.cpp new.tcc
	g++ $(CXXFLAGS) $^ -o $@

example2: example_2.cpp new.tcc
	g++ $(CXXFLAGS) $^ -o $@

example3: example_3.cpp new.tcc
	g++ $(CXXFLAGS) $^ -o $@

example4: example_4.cpp new.tcc
	g++ $(CXXFLAGS) $^ -o $@

clean:
	rm -f example1 example2 example3 example4

.PHONY: all clean
