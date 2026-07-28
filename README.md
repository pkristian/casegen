# casegen

Short for **case generator**. Converts words between cases — and renders templates in
which the placeholder *is* written in the case you want back.

```php
// casegen:foreach
    const COL_CASEGEN_CASE = 'casegen_case';
// casegen:end
```

Feed that `user profile`, and you get:

```php
    const COL_USER_PROFILE = 'user_profile';
```

One line, two different cases, no syntax to learn — the placeholder is a *name*, written
however you want it to come out. Which means a casegen template is a working example of
its own output, and stays valid-looking code in whatever language it targets.

---

## Install

```sh
git clone https://github.com/pkristian/casegen.git && cd casegen && make && sudo make install
```

Needs a C11 compiler, CMake 3.16+ and make. That puts `casegen` in `/usr/local/bin`.
To avoid `sudo`, install somewhere you already own:

```sh
git clone https://github.com/pkristian/casegen.git && cd casegen && make && make install PREFIX=~/.local
```

`make uninstall` removes exactly what was installed, reading the manifest CMake wrote —
it does not guess at paths. `DESTDIR` is honoured for staged/package builds.

## Build

```sh
make           # same as `make build`; binary lands at ./casegen
make test      # build, then run the golden-file suite
make clean
```

CMake owns the build; the `Makefile` is a verb list in front of it. Editors get a
compilation database from `make configure` (or any build), symlinked to
`compile_commands.json` at the repo root.

---

## Case mode

With `-c`, every input line is one record, re-rendered in the chosen case.

```console
$ printf 'user profile\nHTTPServer\ncreated_at\n' | casegen -c snake -
user_profile
http_server
created_at
```

One input line always produces one output line, so a blank line — or a line with no word
characters at all, like `//----` — comes back blank. Output stays aligned with input for
`diff` and `paste`.

### Cases

| | Case | `Casegen` + `Case` | Also accepted as |
|---|---|---|---|
| `-c` | camel | `casegenCase` | |
| `-P` | pascal | `CasegenCase` | `studly` |
| `-s` | snake | `casegen_case` | |
| `-S` | screaming-snake | `CASEGEN_CASE` | `constant`, `macro`, `upper-snake` |
| `-k` | kebab | `casegen-case` | `dash`, `lisp`, `spinal` |
| `-K` | screaming-kebab | `CASEGEN-CASE` | `cobol` |
| | train | `Casegen-Case` | `http-header` |
| | title | `Casegen Case` | |
| | sentence | `Casegen case` | |
| `-l` | lower | `casegen case` | |
| | upper | `CASEGEN CASE` | |
| | dot | `casegen.case` | |
| | path | `casegen/case` | `slash` |
| | ada | `Casegen_Case` | `pascal-snake` |
| | camel-snake | `casegen_Case` | |
| | flat | `casegencase` | |
| | upper-flat | `CASEGENCASE` | |

The aliases work but are hidden from `--help`. All of these name the same case:

```sh
casegen -cs      casegen -c s      casegen -c snake
casegen -csnake  casegen --case=snake      casegen --case snake
```

### Input

Input is the concatenation of the operands, in order. `-` is stdin *at that position*, so
placement is explicit rather than encoded in a flag:

| | |
|---|---|
| `casegen -c s f1 f2` | f1, then f2 |
| `casegen -c s -i` | stdin only |
| `casegen -c s f1 f2 -` | f1, f2, then stdin |
| `casegen -c s - f1 f2` | stdin first |
| `casegen -c s f1 - f2` | stdin between the two |
| `casegen` | usage on stderr, exit 2 |

`-i` means *stdin is the whole input*; passing it alongside a file is an error. Stdin is
never read unless `-i` or `-` asks for it, so behaviour is identical under a terminal, a
pipe, `< /dev/null`, cron and CI.

---

## Template mode

Without `-c`, the input is a template.

**Any line containing `casegen:` is a directive** — parsed, then dropped. Every other line
is content: substituted and emitted. That single rule is why every comment syntax works
with no code that knows about any of them:

```
// casegen:foreach          #  casegen:end          <!-- casegen:raw -->
-- casegen:end-template     /* casegen:raw */       {# casegen:raw-next-line #}
```

Whatever wraps the marker is simply part of a line that gets thrown away.

### Records

`casegen:end-template` splits one file into template and data. Everything below it is
newline-delimited records, read verbatim — never scanned for directives or placeholders.
Lines with no word characters are skipped, so blanks can space the data out.

```console
$ cat columns.tmpl
<?php

class Columns
{
// casegen:foreach
    const COL_CASEGEN_CASE = 'casegen_case';
// casegen:end
}
// casegen:end-template
user profile
created at
is active

$ casegen columns.tmpl
<?php

class Columns
{
    const COL_USER_PROFILE = 'user_profile';
    const COL_CREATED_AT = 'created_at';
    const COL_IS_ACTIVE = 'is_active';
}
```

Records can also come from anywhere else in the operand list, because a template file and
`-` are both just operands read in order:

```console
$ printf 'session token\n' | casegen columns.tmpl -
...
    const COL_IS_ACTIVE = 'is_active';
    const COL_SESSION_TOKEN = 'session_token';
}
```

That is a consequence of how input is assembled, not a rule the template layer knows
about — which is also why there is no `-t` flag.

### Placeholders

The default placeholder is `Casegen Case`. It is rendered in all 17 cases, and whichever
one a line used is the case the record comes back in. Matching is plain substring and
unanchored, which is exactly what makes `COL_CASEGEN_CASE` work inside a longer token.

Names must be **two or more words**. One word renders identically in snake, kebab, dot,
path, flat and lower, leaving nothing to say which case you meant.

### Directives

| Directive | Argument | Where |
|---|---|---|
| `casegen:foreach` | `[as <Two Words>]` — renames the placeholder for this block | top level |
| `casegen:end` | `[<verb>]` — optional label, checked against the innermost block | |
| `casegen:raw` | — block, until `casegen:end` | anywhere but inside `raw` |
| `casegen:raw-next-line` | — | anywhere |
| `casegen:placeholder` | `<Two Words>` — renames the placeholder from here on | top level |
| `casegen:var` | `<Two Words> = <value>` | top level |
| `casegen:end-template` | — everything after is data | first one wins |

There is one implicit collection, so `foreach` takes no argument naming one — it marks
*which region repeats*. Anything outside a loop is emitted once. Same shape as awk's
`BEGIN` / body / `END`.

### Vars

A var is the same object as the loop placeholder, with a fixed value instead of a
per-record one — so it goes through the same scanner, and two of them can land inside one
token:

```console
$ cat v.tmpl
// casegen:var Table Prefix = shop
// casegen:foreach
    '<?= table_prefix_casegen_case ?>' => TablePrefixCasegenCase::class,
// casegen:end
// casegen:end-template
customer order
line item

$ casegen v.tmpl
    '<?= shop_customer_order ?>' => ShopCustomerOrder::class,
    '<?= shop_line_item ?>' => ShopLineItem::class,
```

Longest match wins, ties going to whichever binding was defined first, so the result never
depends on scan order. Output is never rescanned: a value that happens to contain a
placeholder is left alone.

### raw

`raw` suppresses substitution — not the marker, which is how a block's own `casegen:end`
is still found. `raw-next-line` protects a single line, and works inside a loop:

```console
$ cat r.tmpl
// casegen:foreach
    real: CasegenCase
// casegen:raw-next-line
    literal: CasegenCase
// casegen:end
// casegen:end-template
user profile

$ casegen r.tmpl
    real: UserProfile
    literal: CasegenCase
```

`raw` may nest inside `foreach`. `foreach` only opens at the top level: there is one
collection, so loops do not nest, and looping a literal region would only repeat it.

### Errors

A malformed template is a usage error, not a machine failure — exit 2, with the location
and no usage block. A line that turns out to be an error is checked before any of it is
written, so a failed render never leaves half a line on stdout.

```console
$ casegen e.tmpl
casegen: e.tmpl:1: placeholder "CASEGEN_CASE" outside foreach — there is no record to bind it to
```

A `foreach` over zero records, or records with no `foreach` to put them in, warns on
stderr and exits 0. `-q` suppresses warnings.

---

## Word splitting

Both modes split on the same rules.

```console
$ printf 'XMLHttpRequest\nsha256 hash\nuser2Name\nPostgreSQL\ngetID\n__dunder__\n' | casegen -c lower -
xml http request
sha256 hash
user2 name
postgre sql
get id
dunder
```

- **Digits are transparent.** A digit never starts a word on its own, so `sha256`, `utf8`,
  `int32` and `x11` stay whole. `user2Name` → `user2` `name`.
- **Acronyms end where the next word's lowercase begins** — `HTTP|Server`,
  `XML|Http|Request`. With no trailing lowercase the run stays whole: `Postgre|SQL`,
  `get|ID`. No acronym list, no dictionary. The accepted cost is that `OAuth` splits to
  `o auth`, and that round-tripping `HTTPServer` through pascal yields `HttpServer`.
- **Leading and trailing separators are dropped**: `_leading` → `leading`, `__dunder__` →
  `dunder`, `___` → a blank line.
- **ASCII only.** Bytes >= 0x80 are separators and are dropped, silently for now:
  `Příliš žluťoučký` → `p li lu ou k`. No transliteration, no Unicode case mapping.

## Exit codes

| | |
|---|---|
| `0` | fine |
| `1` | runtime failure — unreadable file, I/O error |
| `2` | usage error, or a malformed template |

EOF is distinguished from a read error: a directory or a closed fd 0 names the path and
exits 1, while an empty file or `< /dev/null` is zero records and exit 0.

## Tests

Golden-file suite. Each directory under `tests/` holds a `command.sh` that writes
`output.returned.*`, and an `output.expected.*` to diff it against.

```sh
make test                        # all of them
make test ARGS="-q splitter"     # quiet, one test
```

## Design notes

[`TODO.md`](TODO.md) carries the reasoning: which rules were chosen, what was rejected and
why, and what is deliberately left for later.

## License

Copyright (C) 2026 Patrik Kristian

GPL-3.0-or-later — see [LICENSE](LICENSE). This program is free software: you may
redistribute and modify it under version 3 of the GNU General Public License, or (at your
option) any later version. It comes with no warranty; see the license for details.
