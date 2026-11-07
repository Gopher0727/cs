CC := cc
CFLAGS := -Wall -Wextra -g -MMD -MP

SRCS := $(wildcard src/*.c)
OBJS := $(patsubst src/%.c,bin/%.o,$(SRCS))
DEPS := $(OBJS:.o=.d)
TARGET := bin/main
LDLIBS :=

EXCLUDES := . .. \
	.gitignore \
	src/ \
	Makefile

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
	@list=$$(mktemp) || exit 1; \
	trap 'rm -f "$$list"' EXIT; \
	find . -mindepth 1 -maxdepth 1 \
		$(foreach x,$(EXCLUDES),! -name '$(patsubst %/,%,$(x))') \
		-print0 > "$$list" || exit 1; \
	if [ ! -s "$$list" ]; then \
	        echo "Nothing to delete."; \
		exit 0; \
	fi; \
	printf "Will delete:\n"; \
	xargs -0 -n 1 printf ' * %s\n' < "$$list"; \
	printf "Confirm deletion? [y/N] "; \
	read -r answer || exit 0; \
	[ "$$answer" = "y" ] || exit 0; \
	xargs -0 rm -rf < "$$list" && echo "Deleted."
