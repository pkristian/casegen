# Warnings go to stderr and leave the exit status at 0 — the output is still what the
# template asked for, it just probably is not what the author wanted. -q suppresses
# them, which is the first thing -q has ever had to do.
#
# stdout and stderr are both folded into the one golden file, since the runner diffs a
# single output.returned.* per test.

t() {
    local label="$1"
    shift
    printf '%s\n' "$@" > warn.tmpl

    local flags
    for flags in "" "-q"; do
        echo "== $label${flags:+ (-q)}"
        casegen $flags warn.tmpl > warn.out 2> warn.err </dev/null
        echo "exit=$?"
        echo "-- stdout"
        cat warn.out
        echo "-- stderr"
        cat warn.err
    done
}

{
    t 'foreach over zero records' \
        'class Columns {' \
        '// casegen:foreach' \
        "    const COL_CASEGEN_CASE = 'casegen_case';" \
        '// casegen:end' \
        '}'

    t 'records with no foreach' \
        'nothing repeats here' \
        '// casegen:end-template' \
        'user profile' \
        'created at'
} > output.returned.txt

rm -f warn.tmpl warn.out warn.err
