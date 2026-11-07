CC := cc
CFLAGS := -Wall -Wextra -g -MMD -MP

SRCS := $(wildcard src/*.c)
OBJS := $(patsubst src/%.c,bin/%.o,$(SRCS))
DEPS := $(OBJS:.o=.d)
TARGET := bin/main
LDLIBS :=

GRAY = \033[90m
CYAN = \033[36m
RESET = \033[0m

.PHONY: all run watch clean

all: $(TARGET)

$(TARGET): $(OBJS)
	@mkdir -p $(dir $@)
	@printf '$(GRAY)%s$(RESET)\n' '$(CC) $^ -o $@'
	@$(CC) $^ -o $@ $(LDLIBS)

bin/%.o: src/%.c
	@mkdir -p $(dir $@)
	@printf '$(GRAY)%s$(RESET)\n' '$(CC) $(CFLAGS) -c $< -o $@'
	@$(CC) $(CFLAGS) -c $< -o $@

-include $(DEPS)

run: $(TARGET)
	@printf '$(CYAN)[RUN] %s$(RESET)\n' '$(TARGET)'
	@./$(TARGET)

watch:
	watchexec -c -r -e c,h -- $(MAKE) run

clean:
	rm -rf bin
