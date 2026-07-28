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

# A compilation database, for editors rather than for building: CLion, clangd, VS Code
# and nvim all read it to learn how each file is compiled. Normally you would trace a
# build with `bear -- make`, but every file here goes through one pattern rule with one
# set of flags, so the answer is known without watching anything.
#
# Regenerated when a source changes, when one is added or removed, or when this file
# changes — the flags it records live here. -MMD -MP are deliberately left out: they
# exist to write .d files during a build, and nothing indexing the code should be
# producing those.
#
# $(SRCDIR) is in the prerequisites for the *removal* case, and is not redundant with
# $(SRC). Deleting a source leaves every remaining one older than the database, so make
# would call it up to date and keep listing a file that no longer exists. A directory's
# mtime moves whenever an entry appears or disappears, which is exactly the signal.
#
# Not removed by `clean`. It is an editor artifact, not a build output, and deleting it
# would break indexing until someone noticed and ran this again.
compile_commands.json: $(SRC) $(SRCDIR) Makefile
	@printf '[\n' > $@
	@for src in $(SRC); do \
	    printf '  {\n    "directory": "%s",\n    "file": "%s",\n    "command": "%s"\n  },\n' \
	        "$(CURDIR)" "$$src" \
	        "$(CC) $(CFLAGS) -c -o $(OBJDIR)/$$(basename $$src .c).o $$src"; \
	done | sed '$$s/,$$//' >> $@
	@printf ']\n' >> $@
	@echo "wrote $@ ($(words $(SRC)) entries)"

# make test ARGS="-q splitter"  — ARGS is forwarded to the runner untouched.
test: clean build
	bash tests/runTests.sh $(ARGS)

clean:
	rm -rf $(OBJDIR)
	rm -f $(BIN) tests/*/output.returned.*
