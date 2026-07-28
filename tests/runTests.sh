#!/usr/bin/env bash
#
# Golden-file test runner.
#
# Each test is a dir under tests/ containing:
#   command.sh          runs in its own dir; reads its inputs, writes output.returned.*
#   output.expected.*   the golden output to compare against
#   timeout             optional; seconds this one test may run (default 10)
#
# command.sh owns its I/O — it decides what to read and writes the result to
# output.returned.<ext>. The runner just runs it and diffs returned vs expected.
# Files are matched by glob, so extensions are up to you.
#
# The binary is expected to already be built; `make test` does that for you.
#
# Usage:
#   ./tests/runTests.sh                 run all tests
#   ./tests/runTests.sh 01-usage 03-x   run only the named tests
#   ./tests/runTests.sh -q [names]      quiet: hide passing tests, show only fails
#
# TEST_TIMEOUT=30 ./tests/runTests.sh   raise the default per-test limit
#
# The same arguments pass through the Makefile:
#   make test                           run all tests
#   make test ARGS="-q splitter"        forwarded to this script as-is
#
set -u

tests_dir="$(cd "$(dirname "$0")" && pwd)"
repo_dir="$(dirname "$tests_dir")"

# Colors — only when stdout is a terminal and NO_COLOR isn't set.
if [ -t 1 ] && [ -z "${NO_COLOR:-}" ]; then
    C_PASS=$'\033[32m'; C_FAIL=$'\033[31m'; C_CYAN=$'\033[36m'; C_EXPECTED=$'\033[33m'
    C_DIM=$'\033[2m'; C_BOLD=$'\033[1m'; C_OFF=$'\033[0m'
else
    C_PASS=; C_FAIL=; C_CYAN=; C_EXPECTED=; C_DIM=; C_BOLD=; C_OFF=
fi

# Microseconds since the epoch. $EPOCHREALTIME (bash 5+) costs no fork; its decimal
# separator is locale-dependent, so strip whichever of . or , it used.
if [ -n "${EPOCHREALTIME:-}" ]; then
    now_us() { local t="${EPOCHREALTIME/[.,]/}"; printf '%s' "$t"; }
else
    now_us() { date +%s%6N; }   # GNU date fallback for bash 4 and older
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

# Put the binary on PATH so command.sh can call `casegen` directly.
# ($CASEGEN is also exported as its absolute path, if a test prefers that.)
# Building is the Makefile's job — run `make test`, or `make build` first.
export CASEGEN="$repo_dir/casegen"
export PATH="$repo_dir:$PATH"
[ -x "$CASEGEN" ] || { echo "${C_FAIL}${C_BOLD}NO BINARY${C_OFF}: $CASEGEN — run ${C_BOLD}make build${C_OFF} first"; exit 1; }

# Per-test wall-clock limit, in seconds. Override globally with TEST_TIMEOUT, or
# for one test by putting a number in that test's own `timeout` file.
default_timeout="${TEST_TIMEOUT:-1}"
have_timeout=1
command -v timeout > /dev/null || {
    echo "${C_FAIL}warning${C_OFF}: no ${C_BOLD}timeout${C_OFF} command — tests run unbounded"
    have_timeout=0
}

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
echo -en "${C_CYAN}${C_BOLD}"
echo -e "+--------------------------+"
echo -e "|    casegen test suite    |"
echo -e "+<========================>+"
echo -en "${C_OFF}"
echo "${C_DIM}running $total test(s) from ${tests_dir}${C_OFF}"

idx=0
for name in "${tests_to_run[@]}"; do
    idx=$((idx+1))
    dir="$tests_dir/$name"

    tag="    $idx/$total"
    tag="${tag: -7}"
    tag="${C_DIM}[${tag}]${C_OFF}"

    # This test's limit: its own `timeout` file if it has one, else the default.
    limit="$default_timeout"
    [ -f "$dir/timeout" ] && read -r limit < "$dir/timeout"

    # Run the test in its own dir; it writes output.returned.* itself.
    # -k gives it a second to die politely before SIGKILL.
    started_us="$(now_us)"
    if [ "$have_timeout" -eq 1 ]; then
        (cd "$dir" && timeout -k 1 "$limit" bash command.sh) 2>/dev/null
    else
        (cd "$dir" && bash command.sh) 2>/dev/null
    fi
    status=$?
    # Wall-clock for command.sh alone — the diff below is the runner's cost, not the test's.
    took="(${C_CYAN}$(( ($(now_us) - started_us) / 1000 ))ms${C_OFF})"

    returned="$(first_match "$dir/output.returned.*")"
    expected="$(first_match "$dir/output.expected.*")"

    # 124 = timeout fired; 137 = it had to escalate to SIGKILL.
    if [ "$status" -eq 124 ] || [ "$status" -eq 137 ]; then
        echo "$tag ${C_FAIL}${C_BOLD} TIMEOUT${C_OFF}: ${C_BOLD}$name${C_OFF} $took ${C_DIM}killed after ${limit}s${C_OFF}"
        fail=$((fail+1)); failed_names+=("$name"); continue
    fi
    if [ "$status" -ne 0 ]; then
        echo "$tag ${C_FAIL}    FAIL${C_OFF}: $name ${C_DIM}(command.sh exited $status)${C_OFF} $took"
        fail=$((fail+1)); failed_names+=("$name"); continue
    fi
    if [ -z "$expected" ]; then
        echo "$tag ${C_FAIL}    FAIL${C_OFF}: $name ${C_DIM}(no output.expected.* to compare against)${C_OFF} $took"
        fail=$((fail+1)); failed_names+=("$name"); continue
    fi
    if [ -z "$returned" ]; then
        echo "$tag ${C_FAIL}    FAIL${C_OFF}: $name ${C_DIM}(command.sh wrote no output.returned.*)${C_OFF} $took"
        fail=$((fail+1)); failed_names+=("$name"); continue
    fi

    if diff -u -L expected -L returned "$expected" "$returned" > /dev/null; then
        [ "$quiet" -eq 1 ] || echo "$tag ${C_PASS}${C_BOLD}    PASS${C_OFF}: ${C_BOLD}$name${C_OFF} $took"
        pass=$((pass+1))
    else
        echo "$tag ${C_FAIL}${C_BOLD}    FAIL${C_OFF}: ${C_BOLD}$name${C_OFF} $took"
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
