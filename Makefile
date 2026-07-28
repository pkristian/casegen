CC      ?= cc
CFLAGS  ?= -Wall -Wextra
LDFLAGS ?= --static

BIN    := casegen
SRCDIR := src
# Not "build/" — that is already the name of the phony target below, and make would
# read the two as one thing and call it a circular dependency.
OBJDIR := obj

# New sources are picked up automatically; nothing here lists a file by name.
SRC := $(wildcard $(SRCDIR)/*.c)
OBJ := $(SRC:$(SRCDIR)/%.c=$(OBJDIR)/%.o)
DEP := $(OBJ:.o=.d)

.PHONY: build clean test

build: $(BIN)

$(BIN): $(OBJ)
	$(CC) $(LDFLAGS) -o $@ $(OBJ)

# -MMD -MP writes obj/foo.d listing the headers foo.c pulled in, so editing a header
# rebuilds exactly the objects that read it. Kept out of CFLAGS deliberately: CFLAGS is
# ?= and meant to be overridden, and an override must not be able to switch dependency
# tracking off. The order-only prerequisite (|) keeps the directory's own mtime from
# looking like a reason to rebuild.
$(OBJDIR)/%.o: $(SRCDIR)/%.c | $(OBJDIR)
	$(CC) $(CFLAGS) -MMD -MP -c -o $@ $<

$(OBJDIR):
	mkdir -p $(OBJDIR)

-include $(DEP)

# make test ARGS="-q splitter"  — ARGS is forwarded to the runner untouched.
test: clean build
	bash tests/runTests.sh $(ARGS)

clean:
	rm -rf $(OBJDIR)
	rm -f $(BIN) tests/*/output.returned.*
