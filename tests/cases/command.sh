# Every case, so the whole table is pinned by one golden file.
for c in camel pascal snake screaming-snake kebab screaming-kebab \
         train title sentence lower upper dot path ada camel-snake \
         flat upper-flat; do
    echo "== $c"
    casegen -c "$c" input.txt
done > output.returned.txt
