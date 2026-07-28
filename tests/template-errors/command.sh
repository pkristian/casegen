# Template errors: the exit status and the casegen: diagnostic are the assertion.
# Every case is written to the same bad.tmpl so the file name in the message is stable,
# and stdin is closed so nothing can block.
#
# Unlike input-errors there is no usage block to filter — a template error prints the
# diagnostic alone, because the usage text has nothing to say about templates.

t() {
    local label="$1"
    shift
    printf '%s\n' "$@" > bad.tmpl

    local out
    out="$(casegen bad.tmpl 2>&1 >/dev/null </dev/null)"
    echo "exit=$? -- $label"
    printf '%s\n' "$out" | grep '^casegen:' || true
}

{
    t 'uppercase verb'          'content' '// casegen:FOREACH'
    t 'unknown verb'            '// casegen:nosuchverb'
    t 'marker with no verb'     '// casegen:'
    t 'foreach in foreach'      '// casegen:foreach' '// casegen:foreach' '// casegen:end' '// casegen:end'
    t 'foreach in raw'          '// casegen:raw' '// casegen:foreach' '// casegen:end' '// casegen:end'
    t 'raw in raw'              '// casegen:raw' '// casegen:raw' '// casegen:end' '// casegen:end'
    t 'end label mismatch'      '// casegen:foreach' '// casegen:end raw'
    t 'block never closed'      '// casegen:foreach' '  x'
    t 'end with nothing open'   '// casegen:end'
    t 'raw takes no args'       '// casegen:raw extra'
    t 'foreach arg not "as"'    '// casegen:foreach bogus' '// casegen:end'
    t 'one-word placeholder'    '// casegen:placeholder Solo'
    t 'one-word foreach as'     '// casegen:foreach as Solo' '// casegen:end'
    t 'var without ='           '// casegen:var No Equals here'
    t 'var with empty value'    '// casegen:var Table Prefix ='
    t 'duplicate var'           '// casegen:var Table Prefix = shop' '// casegen:var Table Prefix = other'
    t 'var over the loop name'  '// casegen:var Casegen Case = shop'
    t 'as is undone at end'     '// casegen:foreach as Field Label' '// casegen:end' '// casegen:var Casegen Case = shop'
    t 'declaration in a block'  '// casegen:foreach' '// casegen:var A B = c' '// casegen:end'
    t 'placeholder outside loop' 'const X = CASEGEN_CASE;'
} > output.returned.txt

rm -f bad.tmpl
