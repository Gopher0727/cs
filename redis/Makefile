.PHONY: pre run clean

.DEFAULT_GOAL := run

CC := cc
CCACHE := ccache
CFLAGS := -Wall -Wextra -g
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
