#!/usr/bin/env bash
#
# Golden-file test runner.
#
# Each test is a dir under tests/ containing:
#   command.sh          runs in its own dir; reads its inputs, writes output.returned.*
#   output.expected.*   the golden output to compare against
#
# command.sh owns its I/O — it decides what to read and writes the result to
# output.returned.<ext>. The runner just runs it and diffs returned vs expected.
# Files are matched by glob, so extensions are up to you.
#
# Usage:
#   ./tests/runTests.sh                 run all tests
#   ./tests/runTests.sh 01-usage 03-x   run only the named tests
#   ./tests/runTests.sh -q [names]      quiet: hide passing tests, show only fails
#
set -u

tests_dir="$(cd "$(dirname "$0")" && pwd)"
repo_dir="$(dirname "$tests_dir")"

# Colors — only when stdout is a terminal and NO_COLOR isn't set.
if [ -t 1 ] && [ -z "${NO_COLOR:-}" ]; then
    C_PASS=$'\033[32m'; C_FAIL=$'\033[31m'; C_CYAN=$'\033[36m'; C_EXPECTED=$'\033[33m'
    C_DIM=$'\033[2m'; C_BOLD=$'\033[1m'; C_OFF=$'\033[0m'
else
    C_PASS=; C_FAIL=; C_CYAN=; C_DIM=; C_BOLD=; C_OFF=
fi

# Echo the first existing file matching a glob, or nothing if none match.
first_match() {
    local f
    for f in $1; do
        [ -e "$f" ] && { printf '%s\n' "$f"; return; }
    done
}

# Parse args: -q/--quiet hides passing tests; everything else is a test name.
quiet=0
requested=()
for a in "$@"; do
    case "$a" in
        -q|--quiet) quiet=1 ;;
        *) requested+=("$a") ;;
    esac
done

# Was this test dir named on the command line? (empty names => run everything)
should_run() {
    [ ${#requested[@]} -eq 0 ] && return 0
    local r
    for r in "${requested[@]}"; do [ "$r" = "$1" ] && return 0; done
    return 1
}

# Build the binary and put it on PATH so command.sh can call `casegen` directly.
# ($CASEGEN is also exported as its absolute path, if a test prefers that.)
export CASEGEN="$repo_dir/casegen"
export PATH="$repo_dir:$PATH"
cc -o "$CASEGEN" "$repo_dir/casegen.c" || { echo "${C_FAIL}${C_BOLD}BUILD FAILED${C_OFF}"; exit 1; }

# Wipe leftover returned files before testing.
rm -f "$tests_dir"/*/output.returned.*

pass=0
fail=0
failed_names=()

# Collect the tests we're going to run so we know the total up front.
tests_to_run=()
for cmd in "$tests_dir"/*/command.sh; do
    [ -e "$cmd" ] || continue          # no tests yet
    n="$(basename "$(dirname "$cmd")")"
    should_run "$n" && tests_to_run+=("$n")
done
total=${#tests_to_run[@]}

echo "${C_BOLD}casegen test suite${C_OFF}"
echo "${C_DIM}running $total test(s) from ${tests_dir}${C_OFF}"

idx=0
for name in "${tests_to_run[@]}"; do
    idx=$((idx+1))
    dir="$tests_dir/$name"
    tag="${C_DIM}[$idx/$total]${C_OFF}"

    # Run the test in its own dir; it writes output.returned.* itself.
    (cd "$dir" && bash command.sh) 2>/dev/null
    status=$?

    returned="$(first_match "$dir/output.returned.*")"
    expected="$(first_match "$dir/output.expected.*")"

    if [ "$status" -ne 0 ]; then
        echo "$tag ${C_FAIL}FAIL${C_OFF}: $name ${C_DIM}(command.sh exited $status)${C_OFF}"
        fail=$((fail+1)); failed_names+=("$name"); continue
    fi
    if [ -z "$expected" ]; then
        echo "$tag ${C_FAIL}FAIL${C_OFF}: $name ${C_DIM}(no output.expected.* to compare against)${C_OFF}"
        fail=$((fail+1)); failed_names+=("$name"); continue
    fi
    if [ -z "$returned" ]; then
        echo "$tag ${C_FAIL}FAIL${C_OFF}: $name ${C_DIM}(command.sh wrote no output.returned.*)${C_OFF}"
        fail=$((fail+1)); failed_names+=("$name"); continue
    fi

    if diff -u -L expected -L returned "$expected" "$returned" > /dev/null; then
        [ "$quiet" -eq 1 ] || echo "$tag ${C_PASS}${C_BOLD}PASS${C_OFF}:  ${C_BOLD}$name${C_OFF}"
        pass=$((pass+1))
    else
        echo "$tag ${C_FAIL}${C_BOLD}FAIL${C_OFF}: ${C_BOLD}$name${C_OFF}"
        # expected lines (-) blue, returned lines (+) red
        diff -u -L expected -L returned "$expected" "$returned" | tail -n +4 | sed -E \
            -e "s/^(-.*)$/${C_EXPECTED}\1${C_OFF}/" \
            -e "s/^(\+.*)$/${C_FAIL}\1${C_OFF}/" \
            -e 's/^/    /'
        fail=$((fail+1)); failed_names+=("$name")
    fi
done

echo "${C_DIM}---------------${C_OFF}"

echo "${C_PASS}${C_BOLD}PASSED: $pass${C_OFF}";

if [ "$fail" -eq 0 ]; then
    echo -n "${C_DIM}";
else
    echo -n "${C_BOLD}";
fi
echo "${C_FAIL}FAILED: $fail${C_OFF}";
echo "${C_BOLD}TOTAL:  $total${C_OFF}";
[ "$fail" -eq 0 ]
