# A var is the loop placeholder with a fixed value instead of a per-record one, so it
# goes through the same scanner — which is why two of them can land in one token.
{
    echo "== vars.tmpl (constants, longest match, two bindings in one token)"
    casegen vars.tmpl

    echo "== naming.tmpl (placeholder renames the loop binding, as scopes the rename)"
    casegen naming.tmpl
} > output.returned.txt
