CC     ?= cc
CFLAGS ?= -Wall -Wextra
LDFLAGS ?= --static

BIN = casegen
SRC = src/main.c
# Everything else is #included into main.c, so it is a prerequisite of the
# build but never compiled on its own. New files are picked up automatically.
DEPS = $(wildcard src/*.c)

.PHONY: build clean test

build: $(BIN)

$(BIN): $(DEPS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(SRC)

# make test ARGS="-q splitter"  — ARGS is forwarded to the runner untouched.
test: clean build
	bash tests/runTests.sh $(ARGS)

clean:
	rm -f $(BIN) tests/*/output.returned.*
