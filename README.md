> If you know snake_case and you know PascalCase, you need not fear the result of a
> hundred refactorings.
>
> — Sun Tzu, probably

# casegen

Short for **case generator**. It converts words between cases, and renders templates in
which the placeholder is written in the case you want back.

That inversion is the whole idea. There is no filter language and no `{{ }}`: you write
the placeholder's name — `Casegen Case` by default — spelled the way the output should be
spelled. `CASEGEN_CASE` comes back a constant, `casegenCase` a camel-case key,
`casegen_case` a column name, and one template may use all of them at once, on the same
line. Give it a list of names and it repeats whichever region you marked, once each.

Because the placeholder is only a name, and the directives hide inside ordinary comments,
a template is real code. It parses, it highlights, your linter reads it, and you can see
what it will produce without running anything.

`schema.php`:

```php
<?php
// casegen:var Table Prefix = shop

final class Schema
{
// casegen:foreach
    /** The casegen case column. */
    public const CASEGEN_CASE = 'table_prefix_casegen_case';
// casegen:end

    /** @return array<string,string> */
    public static function all(): array
    {
        return [
// casegen:foreach
            self::CASEGEN_CASE => 'casegenCase',
// casegen:end
        ];
    }
}
// casegen:end-template
// user profile
// created at
```

```sh
casegen schema.php
```

```php
<?php

final class Schema
{
    /** The user profile column. */
    public const USER_PROFILE = 'shop_user_profile';
    /** The created at column. */
    public const CREATED_AT = 'shop_created_at';

    /** @return array<string,string> */
    public static function all(): array
    {
        return [
            self::USER_PROFILE => 'userProfile',
            self::CREATED_AT => 'createdAt',
        ];
    }
}
```

Two loops over one list, a `shop` prefix bound once, and each record arriving in four
different cases — lower in the docblock, screaming snake as the constant name, snake
inside the prefixed string, camel as the array value. Everything outside a loop is emitted
once. Both files pass `php -l`.

---

## Quickstart

```sh
curl -fsSL https://raw.githubusercontent.com/pkristian/casegen/master/install.sh | sh
```

```console
$ printf 'hello world' | casegen -icc
helloWorld
```

That bundle is `-i` (stdin is the whole input) and `-c c` (camel). More on
[installing](#install) and on [what else it does](#case-mode) below.

---

## What it's for

Generating the boilerplate that follows from a list of names. Database columns into class
constants, field names into DTO properties or enum cases, an entity list into a service
map, a schema into migrations — anywhere the same handful of names has to appear in four
different casings and stay in step.

The point of writing the placeholder in the target case is that the template stays real
code. It parses, it highlights, your linter reads it, and you can tell at a glance what it
will produce — rather than being a string full of `{{ name | snake_case }}`.

For one-off conversions there is also plain case mode, which is just a filter.

## Options

```
casegen [-i] [-q] [-c CASE] [FILE|-]...
```

| Option | Meaning |
|---|---|
| `-c CASE`, `--case=CASE` | render every input line in `CASE` — see [Cases](#cases) |
| *(no `-c`)* | template mode |
| `-i` | stdin is the whole input; an error if a `FILE` is also given |
| `-q`, `--quiet` | suppress warnings |
| `-h`, `--help` | usage and the case table on stdout, exit 0 |
| `--` | end of options |
| `FILE`, `-` | read in order; `-` is stdin *at that position* |

Short options bundle, and `-c` takes the rest of its token as the case — so `-icc` is
`-i` plus `-c c`.

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

| Short | Case | `Casegen` + `Case` | Also accepted as |
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

| Invocation | Reads |
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

`columns.php`:

```php
<?php

class Columns
{
// casegen:foreach
    const COL_CASEGEN_CASE = 'casegen_case';
// casegen:end
}
// casegen:end-template
// user profile
// created at
// is active
```

```sh
casegen columns.php
```

```php
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

```sh
printf '// session token\n' | casegen columns.php -
```

Same output as before, with one more constant on the end:

```php
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

`entities.php`:

```php
<?php
// casegen:var Table Prefix = shop

return [
// casegen:foreach
    'table_prefix_casegen_case' => TablePrefixCasegenCase::class,
// casegen:end
];
// casegen:end-template
// customer order
// line item
```

```sh
casegen entities.php
```

```php
<?php

return [
    'shop_customer_order' => ShopCustomerOrder::class,
    'shop_line_item' => ShopLineItem::class,
];
```

Both the template and its output pass `php -l`. The array key picks up two bindings inside
one token — `table_prefix` and `casegen_case` — while the class name picks up the same two
in pascal, from the same line.

Longest match wins, ties going to whichever binding was defined first, so the result never
depends on scan order. Output is never rescanned: a value that happens to contain a
placeholder is left alone.

### raw

`raw` suppresses substitution — not the marker, which is how a block's own `casegen:end`
is still found. `raw-next-line` protects a single line, and works inside a loop:

`constants.php`:

```php
<?php
// casegen:raw
/**
 * Generated from a casegen template — the placeholder is CasegenCase.
 * This header documents it, so it has to come through untouched.
 */
// casegen:end

class Constants
{
// casegen:foreach
// casegen:raw-next-line
    /** Rendered from the casegen_case placeholder. */
    public const CASEGEN_CASE = 'casegen_case';
// casegen:end
}
// casegen:end-template
// user profile
// line item
```

```sh
casegen constants.php
```

```php
<?php
/**
 * Generated from a casegen template — the placeholder is CasegenCase.
 * This header documents it, so it has to come through untouched.
 */

class Constants
{
    /** Rendered from the casegen_case placeholder. */
    public const USER_PROFILE = 'user_profile';
    /** Rendered from the casegen_case placeholder. */
    public const LINE_ITEM = 'line_item';
}
```

The header survives because it is documenting the placeholder rather than using it. The
docblock inside the loop is protected line by line, while the constant under it is not.

Note that a protected line inside a `foreach` is emitted once per record, identically —
fine for a comment, but a repeated `const` would not compile.

`raw` may nest inside `foreach`. `foreach` only opens at the top level: there is one
collection, so loops do not nest, and looping a literal region would only repeat it.

### Errors

A malformed template is a usage error, not a machine failure — exit 2, with the location
and no usage block.

`broken.php` — a placeholder with no `foreach` around it, so there is no record to bind:

```php
<?php

class Columns
{
    const COL_CASEGEN_CASE = 'casegen_case';
}
```

```console
$ casegen broken.php
casegen: broken.php:5: placeholder "CASEGEN_CASE" outside foreach — there is no record to bind it to
$ echo $?
2
```

Lines already rendered are on stdout — the first four, here. The line that failed is not
partially written, though: it is checked before any of it is emitted, so a failed render
never leaves half a line behind.

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

| Code | Meaning |
|---|---|
| `0` | fine |
| `1` | runtime failure — unreadable file, I/O error |
| `2` | usage error, or a malformed template |

EOF is distinguished from a read error: a directory or a closed fd 0 names the path and
exits 1, while an empty file or `< /dev/null` is zero records and exit 0.

## Install

The one-liner at the top installs any missing build dependencies, clones into a temporary
directory, builds, installs to `/usr/local/bin`, and deletes the clone on the way out —
including when it fails partway.

| Flag | Meaning |
|---|---|
| `--prefix DIR` | install somewhere you already own; no `sudo` needed |
| `--no-deps` | never touch the package manager |

```sh
curl -fsSL https://raw.githubusercontent.com/pkristian/casegen/master/install.sh | sh -s -- --prefix ~/.local
```

Piping a script from the internet into a shell is a habit worth being deliberate about.
`install.sh` is about a hundred readable lines, so fetch it and look before you run it:

```sh
curl -fsSL https://raw.githubusercontent.com/pkristian/casegen/master/install.sh -o install.sh
less install.sh && sh install.sh
```

### From source

Needs a C11 compiler, CMake 3.16+ and make.

```sh
git clone https://github.com/pkristian/casegen.git && cd casegen && make && sudo make install
```

`make install PREFIX=~/.local` to avoid `sudo`. `make uninstall` removes exactly what was
installed, reading the manifest CMake wrote — it does not guess at paths. `DESTDIR` is
honoured for staged and package builds.

## Build

```sh
make           # same as `make build`; binary lands at ./casegen
make test      # build, then run the golden-file suite
make clean
```

CMake owns the build; the `Makefile` is a verb list in front of it. Editors get a
compilation database from `make configure` (or any build), symlinked to
`compile_commands.json` at the repo root.

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
