CC     ?= cc
CFLAGS ?= -Wall -Wextra
LDFLAGS ?= --static

BIN = casegen
SRC = casegen.c

.PHONY: build clean test

build: $(BIN)

$(BIN): $(SRC)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(SRC)

# make test ARGS="-q splitter"  — ARGS is forwarded to the runner untouched.
test: clean build
	bash tests/runTests.sh $(ARGS)

clean:
	rm -f $(BIN) tests/*/output.returned.*
