# raw suppresses substitution; it never suppresses the marker, which is how the
# block's own end is still found. And the wrapper around a marker is irrelevant.
{
    echo "== raw.tmpl (block, next-line, and a raw region inside a loop)"
    casegen raw.tmpl

    echo "== wrappers.tmpl (every comment syntax, one raw-next-line each)"
    casegen wrappers.tmpl
} > output.returned.txt
