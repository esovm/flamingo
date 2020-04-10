CFLAGS := -std=c99 -Wall -Wextra -pedantic -Iinclude
LDLIBS = -ledit

ifeq ($(MODE), debug)
	CFLAGS += -O0 -g
	BUILDDIR := build/debug
else
	CFLAGS += -O3 -flto -Wno-unused-parameter
	BUILDDIR := build/release
endif

HDRS := $(wildcard $(SRCDIR)/*.h)
SRCS := $(wildcard $(SRCDIR)/*.c)
OBJS := $(addprefix $(BUILDDIR)/$(NAME)/, $(notdir $(SRCS:.c=.o)))

build/$(NAME): $(OBJS)
	@printf "%8s %-40s %s\n" $(CC) $@ "$(CFLAGS)"
	@mkdir -p build
	@$(CC) $(LDLIBS) $(CFLAGS) $^ -o $@

$(BUILDDIR)/$(NAME)/%.o: $(SRCDIR)/%.c $(HDRS)
	@printf "%8s %-40s %s\n" $(CC) $< "$(CFLAGS)"
	@mkdir -p $(BUILDDIR)/$(NAME)
	@$(CC) -c $(CFLAGS) -o $@ $<

.PHONY: default
