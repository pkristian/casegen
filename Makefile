# CMake owns the build. This file is a verb list, so `make build` and `make test` keep
# working the way they always have — nothing here describes how to compile anything.
# That is CMakeLists.txt's job, and there is deliberately no second copy of it.

BIN := casegen

# Not "build/": that is the name of the phony target below, and make would read the two
# as one thing and call it a circular dependency. CLion's own cmake-build-debug/ and
# friends sit alongside this one quite happily.
BUILDDIR := cmake-build

.PHONY: build configure test clean

# cmake --build re-runs cmake by itself when CMakeLists.txt changes, so the cache file
# only has to exist; it does not have to be up to date with anything.
build: $(BUILDDIR)/CMakeCache.txt
	cmake --build $(BUILDDIR)
	@ln -sf $(BUILDDIR)/compile_commands.json compile_commands.json

configure: $(BUILDDIR)/CMakeCache.txt

$(BUILDDIR)/CMakeCache.txt:
	cmake -S . -B $(BUILDDIR)

# No longer `clean build`. That existed because the old rules could not be fully trusted
# to notice a header change; CMake tracks dependencies properly, so a rebuild already
# guarantees the binary matches the sources, and it is far quicker.
#
# make test ARGS="-q splitter"  — ARGS is forwarded to the runner untouched.
test: build
	bash tests/runTests.sh $(ARGS)

clean:
	rm -rf $(BUILDDIR)
	rm -f $(BIN) compile_commands.json tests/*/output.returned.*
