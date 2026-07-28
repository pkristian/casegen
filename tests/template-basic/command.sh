# The core of template mode: a foreach body repeated per record, with the case each
# placeholder was written in deciding the case it comes back in.
{
    echo "== columns.tmpl (inline data after end-template)"
    casegen columns.tmpl

    echo "== columns.tmpl - (stdin appends to the file's own data section)"
    printf 'is deleted\n' | casegen columns.tmpl -

    echo "== every-case.tmpl (one line per row of the case table)"
    casegen every-case.tmpl

    echo "== two-loops.tmpl (content outside a loop is emitted once)"
    casegen two-loops.tmpl

    echo "== plain.tmpl (no directives, no placeholders)"
    casegen plain.tmpl
} > output.returned.txt
