# CMake owns the build. This file is a verb list, so `make build` and `make test` keep
# working the way they always have — nothing here describes how to compile anything.
# That is CMakeLists.txt's job, and there is deliberately no second copy of it.

BIN := casegen

# Not "build/": that is the name of the phony target below, and make would read the two
# as one thing and call it a circular dependency. CLion's own cmake-build-debug/ and
# friends sit alongside this one quite happily.
BUILDDIR := cmake-build

# Where `make install` puts things. /usr/local needs root; PREFIX=~/.local does not.
# DESTDIR is honoured too, for staged installs by a package build.
PREFIX ?= /usr/local

.PHONY: build configure test install uninstall clean

# cmake --build re-runs cmake by itself when CMakeLists.txt changes, so the cache file
# only has to exist; it does not have to be up to date with anything.
build: compile_commands.json
	cmake --build $(BUILDDIR)

configure: compile_commands.json

$(BUILDDIR)/CMakeCache.txt:
	cmake -S . -B $(BUILDDIR)

# Configuring is what writes the database, so this needs no compiling — `make configure`
# is enough to get an editor indexing again after a clean. It has to be a symlink into
# the build tree rather than a file that outlives it: `clean` deletes cmake-build/, and
# a dangling link would be worse than an absent one.
compile_commands.json: $(BUILDDIR)/CMakeCache.txt
	@ln -sf $(BUILDDIR)/compile_commands.json $@

# No longer `clean build`. That existed because the old rules could not be fully trusted
# to notice a header change; CMake tracks dependencies properly, so a rebuild already
# guarantees the binary matches the sources, and it is far quicker.
#
# make test ARGS="-q splitter"  — ARGS is forwarded to the runner untouched.
test: build
	bash tests/runTests.sh $(ARGS)

install: build
	cmake --install $(BUILDDIR) --prefix $(DESTDIR)$(PREFIX)

# cmake writes an install_manifest.txt listing exactly what it put where, so removal
# does not have to guess at paths or repeat the layout rules from CMakeLists.txt.
uninstall:
	@[ -f $(BUILDDIR)/install_manifest.txt ] \
	    || { echo "no $(BUILDDIR)/install_manifest.txt — nothing recorded as installed"; exit 1; }
	xargs rm -fv < $(BUILDDIR)/install_manifest.txt

clean:
	rm -rf $(BUILDDIR)
	rm -f $(BIN) compile_commands.json tests/*/output.returned.*
