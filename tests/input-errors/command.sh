# Error paths: the exit status and the casegen: diagnostics are the assertion.
# The usage block is filtered out on purpose — pinning it here would mean every
# new case added to --help breaks this test for no reason.
# stdin is closed so nothing can block.

run() {
    local out
    out="$(casegen "$@" 2>&1 >/dev/null </dev/null)"
    echo "exit=$? -- casegen $*"
    printf '%s\n' "$out" | grep '^casegen:' || true
}

{
    run
    run -i a.txt
    run -c
    run -c nosuchcase
    run -Z
    run --bogus
    run -c lower a.txt - - -
    run -c lower nosuch.txt
    run a.txt
} > output.returned.txt
