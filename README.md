<p align="center">
  <em>Only fool is writing same thing many times.</em>
  <br><br>
  — Sun Tzu, probably
</p>

# casegen - Case Generator

Style words into various cases.  
Replace where placeholder written.  
Directives in comments.  
Template still valid code.  

Example:

```php
<?php
// casegen:var Table Prefix = shop

final class Schema
{
// casegen:foreach
    /** The casegen case column. */
    public const COL_CASEGEN_CASE = 'table_prefix_casegen_case';
// casegen:end

    /** @return array<string,string> */
    public static function all(): array
    {
        return [
// casegen:foreach
            self::COL_CASEGEN_CASE => 'casegenCase',
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
    public const COL_USER_PROFILE = 'shop_user_profile';
    /** The created at column. */
    public const COL_CREATED_AT = 'shop_created_at';

    /** @return array<string,string> */
    public static function all(): array
    {
        return [
            self::COL_USER_PROFILE => 'userProfile',
            self::COL_CREATED_AT => 'createdAt',
        ];
    }
}
```

Noice.

---

## Quickstart

```sh
curl -fsSL https://raw.githubusercontent.com/pkristian/casegen/master/install.sh | sh
```

```console
$ printf 'hello world' | casegen -icc
helloWorld
```

Bundle is `-i` (stdin is whole input) plus `-c c` (camel). More on
[installing](#install) and [what else it do](#case-mode) below.

---

## What it for

Making boilerplate that follow from list of names.  
Database columns into class constants.  
Field names into DTO properties or enum cases.  
Entity list into service map.   
Schema into migrations.  
Anywhere same handful of names must appear in multiple casings.

Point of writing placeholder in target case:  
Template stay real code.  
It parse.  
It highlight.  
You see output at glance.  
Not full of `{{ name | snake_case }}`.

For one-off conversion there is plain case mode.  
That one is just filter.

## Options

```
casegen [-i] [-q] [-c CASE] [FILE|-]...
```

| Option | Meaning |
|---|---|
| `-c CASE`, `--case=CASE` | render every input line in `CASE` — see [Cases](#cases) |
| *(no `-c`)* | template mode |
| `-i` | stdin is whole input; error if `FILE` also given |
| `-q`, `--quiet` | no warnings |
| `-h`, `--help` | usage plus case table on stdout, exit 0 |
| `--` | end of options |
| `FILE`, `-` | read in order; `-` is stdin *at that spot* |

Short options bundle. `-c` eat rest of its token as case name. So `-icc` is `-i` plus
`-c c`.

---

## Case mode

With `-c`, every input line is one record, re-rendered in chosen case.

```console
$ printf 'user profile\nHTTPServer\ncreated_at\n' | casegen -c snake -
user_profile
http_server
created_at
```

One input line always make one output line. Blank line — or line with no word characters,
like `//----` — come back blank. Output stay lined up with input, good for `diff` and
`paste`.

### Cases

| Short | Case | `Casegen` + `Case` | Also accepted as |
|---|---|---|---|
| `-c` | `camel` | `casegenCase` | |
| `-P` | `pascal` | `CasegenCase` | `studly` |
| `-s` | `snake` | `casegen_case` | |
| `-S` | `screaming-snake` | `CASEGEN_CASE` | `constant`, `macro`, `upper-snake` |
| `-k` | `kebab` | `casegen-case` | `dash`, `lisp`, `spinal` |
| `-K` | `screaming-kebab` | `CASEGEN-CASE` | `cobol` |
| | `train` | `Casegen-Case` | `http-header` |
| | `title` | `Casegen Case` | |
| | `sentence` | `Casegen case` | |
| `-l` | `lower` | `casegen case` | |
| | `upper` | `CASEGEN CASE` | |
| | `dot` | `casegen.case` | |
| | `path` | `casegen/case` | `slash` |
| | `ada` | `Casegen_Case` | `pascal-snake` |
| | `camel-snake` | `casegen_Case` | |
| | `flat` | `casegencase` | |
| | `upper-flat` | `CASEGENCASE` | |

Aliases work but hide from `--help`. All of these name same case:

```sh
casegen -cs      casegen -c s      casegen -c snake
casegen -csnake  casegen --case=snake      casegen --case snake
```

### Input

Input is operands glued together, in order.  
`-` is stdin *at that spot*, so placement is
plain to see, not hidden in flag:

| Invocation | Reads |
|---|---|
| `casegen -c s f1 f2` | `f1`, then `f2` |
| `casegen -c s -i` | `stdin` only |
| `casegen -c s f1 f2 -` | `f1`, `f2`, then `stdin` |
| `casegen -c s - f1 f2` | stdin first |
| `casegen -c s f1 - f2` | stdin between the two |
| `casegen` | usage on stderr, exit 2 |

`-i` mean *stdin is whole input*. Pass it with a file, that is error. Stdin never read
unless `-i` or `-` ask for it. So behaviour same under terminal, pipe, `< /dev/null`, cron,
CI.

---

## Template mode

Without `-c`, input is template.

**Any line holding `casegen:` is directive** — parsed, then **thrown away**.  
Every other line is content: substituted and emitted.  


```
// casegen:foreach          #  casegen:end          <!-- casegen:raw -->
-- casegen:end-template     /* casegen:raw */       {# casegen:raw-next-line #}
```

Whatever wrap the marker is just part of line that go in fire.

### Records

`casegen:end-template` cut file into template and data.  
Everything under it is newline-delimited records.
Never scanned for directives or placeholders.
Lines with no word characters skipped, so blanks can space data out.

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

Records can come from anywhere else after template, because files and `-` are
concated:

```sh
printf '// session token\n' | casegen columns.php -
```

Same output as before, one more constant on end:

```php
    const COL_IS_ACTIVE = 'is_active';
    const COL_SESSION_TOKEN = 'session_token';
}
```

### Placeholders

Default placeholder is `Casegen Case`. It rendered in all 17 cases. Whichever one line
used, that is case record come back in. Matching is plain substring, unanchored — that is
what make `COL_CASEGEN_CASE` work inside longer token.

Names must be **two words or more**. One word render same in snake, kebab, dot, path, flat
and lower. Nothing left to say which case you meant.

### Directives

| Directive | Argument | Where |
|---|---|---|
| `casegen:foreach` | `[as <Two Words>]` — rename placeholder for this block | top level |
| `casegen:end` | `[<verb>]` — optional label, checked against innermost block | |
| `casegen:raw` | — block, until `casegen:end` | anywhere but inside `raw` |
| `casegen:raw-next-line` | — | anywhere |
| `casegen:placeholder` | `<Two Words>` — rename placeholder from here on | top level |
| `casegen:var` | `<Two Words> = <value>` | top level |
| `casegen:end-template` | — everything after is data | first one wins |

There is one implicit collection. So `foreach` take no argument naming one — it only mark
*which region repeat*. Anything outside loop emitted once. Same shape as awk `BEGIN` /
body / `END`.

### Vars

Var is same object as loop placeholder, but with fixed value instead of per-record one. So
it go through same scanner, and two of them can land inside one token:

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

Template and output both pass `php -l`. Array key pick up two bindings inside one token —
`table_prefix` and `casegen_case` — while class name pick up same two in pascal, from same
line.

Longest match win. Tie go to binding defined first. So result never depend on scan order.
Output never rescanned: value that happen to hold placeholder left alone.

### raw

`raw` stop substitution — not the marker. That is how block own `casegen:end` still found.
`raw-next-line` protect one line, and work inside loop:

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

Header survive because it document placeholder, not use it. Docblock inside loop protected
line by line. Constant under it not protected.

Careful: protected line inside `foreach` emitted once per record, same every time. Fine for
comment. Repeated `const` would not compile.

`raw` may nest inside `foreach`. `foreach` only open at top level: one collection, so loops
do not nest, and looping literal region would only repeat it.

### Errors

Bad template is usage error, not machine failure — exit 2, with location, no usage block.

`broken.php` — placeholder with no `foreach` around it, so no record to bind:

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

Lines already rendered sit on stdout — first four, here. But failed line never half
written: it checked before any of it emitted, so broken render leave no half line behind.

`foreach` over zero records, or records with no `foreach` to sit in, warn on stderr and
exit 0. `-q` kill warnings.

---

## Word splitting

Both modes split by same rules.

```console
$ printf 'XMLHttpRequest\nsha256 hash\nuser2Name\nPostgreSQL\ngetID\n__dunder__\n' | casegen -c lower -
xml http request
sha256 hash
user2 name
postgre sql
get id
dunder
```

- **Digits see-through.** Digit never start word alone, so `sha256`, `utf8`, `int32` and
  `x11` stay whole. `user2Name` → `user2` `name`.
- **Acronym end where next word lowercase begin** — `HTTP|Server`, `XML|Http|Request`. No
  trailing lowercase, run stay whole: `Postgre|SQL`, `get|ID`. No acronym list, no
  dictionary. Price paid: `OAuth` split to `o auth`, and `HTTPServer` round-tripped through
  pascal come back `HttpServer`.
- **Leading and trailing separators dropped**: `_leading` → `leading`, `__dunder__` →
  `dunder`, `___` → blank line.
- **ASCII only.** Bytes >= 0x80 are separators and get dropped, quietly for now:
  `Příliš žluťoučký` → `p li lu ou k`. No transliteration, no Unicode case mapping.

## Exit codes

| Code | Meaning |
|---|---|
| `0` | fine |
| `1` | runtime failure — unreadable file, I/O error |
| `2` | usage error, or bad template |

EOF not confused with read error: directory or closed fd 0 name the path and exit 1, while
empty file or `< /dev/null` is zero records and exit 0.

## Install

One-liner at top install any missing build deps, clone into temp directory, build, install
to `/usr/local/bin`, then delete clone on way out — even when it fail partway.

| Flag | Meaning |
|---|---|
| `--prefix DIR` | install somewhere you already own; no `sudo` needed |
| `--no-deps` | never touch package manager |

```sh
curl -fsSL https://raw.githubusercontent.com/pkristian/casegen/master/install.sh | sh -s -- --prefix ~/.local
```

Piping script from internet into shell is habit worth being deliberate about. `install.sh`
is about hundred readable lines. Fetch it, look first:

```sh
curl -fsSL https://raw.githubusercontent.com/pkristian/casegen/master/install.sh -o install.sh
less install.sh && sh install.sh
```

### From source

Need C11 compiler, CMake 3.16+ and make.

```sh
git clone https://github.com/pkristian/casegen.git && cd casegen && make && sudo make install
```

`make install PREFIX=~/.local` to skip `sudo`. `make uninstall` remove exactly what was
installed, reading manifest CMake wrote — it guess no paths. `DESTDIR` honoured for staged
and package builds.

## Build

```sh
make           # same as `make build`; binary lands at ./casegen
make test      # build, then run the golden-file suite
make clean
```

CMake own the build. `Makefile` is verb list standing in front of it. Editors get
compilation database from `make configure` (or any build), symlinked to
`compile_commands.json` at repo root.

## Tests

Golden-file suite. Each directory under `tests/` hold a `command.sh` that write
`output.returned.*`, and an `output.expected.*` to diff it against.

```sh
make test                        # all of them
make test ARGS="-q splitter"     # quiet, one test
```

## Design notes

[`TODO.md`](TODO.md) carry the reasoning: which rules were chosen, what was rejected and
why, and what is left for later on purpose.

## License

Copyright (C) 2026 Patrik Kristian

GPL-3.0-or-later — see [LICENSE](LICENSE). This program is free software: you may
redistribute and modify it under version 3 of the GNU General Public License, or (at your
option) any later version. It come with no warranty; see license for details.
