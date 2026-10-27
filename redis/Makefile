.PHONY: pre run clean run-cpp

.DEFAULT_GOAL := run

CCACHE := ccache
CC := cc
CFLAGS := -Wall -Wextra -g
CXX := c++
CXXFLAGS := -std=c++20 -O2 -Wall -Wextra -Wpedantic
LDLIBS := -lhiredis

pre:
	@mkdir -p bin/

bin/main.o: main.c | pre
	@$(CCACHE) $(CC) $(CFLAGS) -c $< -o $@

bin/main: bin/main.o
	$(CC) -o $@ $^ $(LDLIBS)

run: bin/main
	@./bin/main

clean:
	@rm -r bin/

run-cpp: pre
	@$(CXX) $(CXXFLAGS) main.cpp -o bin/main $(LDLIBS) && ./bin/main
