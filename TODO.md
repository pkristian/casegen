# Cases

Input word split: `Casegen` + `Case`

## Most used

| Case | Output | Aliases |
|---|---|---|
| camelCase | `casegenCase` | lower camel, dromedary case |
| PascalCase | `CasegenCase` | upper camel, StudlyCaps |
| snake_case | `casegen_case` | — |
| SCREAMING_SNAKE_CASE | `CASEGEN_CASE` | CONSTANT_CASE, UPPER_SNAKE_CASE, MACRO_CASE |
| kebab-case | `casegen-case` | dash-case, lisp-case, spinal-case, caterpillar-case |
| SCREAMING-KEBAB-CASE | `CASEGEN-CASE` | COBOL-CASE |

## Common

| Case | Output | Aliases |
|---|---|---|
| Train-Case | `Casegen-Case` | HTTP-Header-Case |
| Title Case | `Casegen Case` | — |
| Sentence case | `Casegen case` | — |
| lower case | `casegen case` | — |
| UPPER CASE | `CASEGEN CASE` | — |
| dot.case | `casegen.case` | — |
| path/case | `casegen/case` | slash case |
| Ada_Case | `Casegen_Case` | Camel_Snake_Case, Pascal_Snake_Case |
| camel_Snake_Case | `casegen_Case` | — |
| flatcase | `casegencase` | lowercase (no separator) |
| UPPERFLATCASE | `CASEGENCASE` | COBOL flat |

Note: "Train-Case" is ambiguous across libraries — most JS libs use `Casegen-Case`,
some Ruby/Go libs use `CASEGEN-CASE`. Pick one, document it, accept the aliases.

## Directives

### Parsing rule

Strictly line by line. No comment-syntax awareness, no multi-line lexing, no lookahead.

1. Read one line.
2. If it contains the marker `casegen:` (case-sensitive), the **entire line is a
   directive** — parse it and **do not emit it to output**.
3. Otherwise the line is content: apply placeholder substitution and emit.

Consequence: whatever wraps the marker is irrelevant. `//`, `#`, `--`, `/* */`,
`<!-- -->`, `<%-- --%>` all work with zero extra code, because the wrapper is
simply part of a line that gets thrown away.

```php
// casegen:foreach          →  directive, consumed
   # casegen:end            →  directive, consumed
<!-- casegen:raw -->        →  directive, consumed
const COL_CASEGEN_CASE = '';→  content, substituted, emitted
```

### Grammar

```
line   := <anything> marker verb [ws args] <anything>
marker := "casegen:"
verb   := [a-z][a-z0-9-]*        // one token, up to first whitespace
args   := rest of line, minus trailing comment delimiters
```

- Verb is **exactly one whitespace-free token**; compounds use kebab
  (`raw-next-line`, not `raw next line`, not `rawnextline`).
- Everything after the first whitespace is arguments.
- Verbs are **strictly lowercase**. Reject `casegen:FOREACH` with an error rather
  than normalizing — this is a case tool, don't blur which layer is converting.
- Strip trailing delimiters from args before splitting: `-->`, `*/`, `--%>`, `?>`, `#}`.
  Do this once in the lexer. Otherwise `<!-- casegen:foreach as CasegenCase -->`
  parses `-->` as an argument.

### Verbs

| Directive | Args |
|---|---|
| `casegen:foreach` | `[as <Placeholder>]` — reserved, parsed and ignored in v1 |
| `casegen:end` | `[<verb>]` — optional label, validated against the open block |
| `casegen:raw` | — (block, until `casegen:end`) |
| `casegen:raw-next-line` | — |
| `casegen:placeholder` | `<Two Words>` |
| `casegen:end-template` | — everything after this line is data, not template |

No trailing/same-line `raw-line` form: the strict rule eats the whole line, so a
directive can never share a line with code it protects. Use `raw-next-line`.

`end-template` is its own kebab verb, **not** `casegen:end template` — that would
parse as the block terminator with a `template` label. Two different things, keep
them lexically distinct.

### Inline data

`casegen:end-template` splits a single file into template + data. Everything below
it is newline-delimited records, read verbatim, never scanned for directives or
placeholders.

```php
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
```

Precedent: Perl `__END__` / `__DATA__`, Ruby `__END__` + the `DATA` filehandle,
PHP's own `__halt_compiler()`, and self-extracting shell archives.

Open: what happens when a file has `end-template` **and** stdin has records —
error, concatenate, or stdin wins? Erroring is the safe default; it's the only
one you can loosen later without breaking anyone.

### Loop model

One implicit collection: newline-delimited records on stdin. `foreach` takes no
collection argument — it marks *which region repeats*. Everything outside a
`foreach` block is emitted once, verbatim. Same shape as awk's `BEGIN` / body / `END`.

```php
<?php

class Columns
{
// casegen:foreach
    const COL_CASEGEN_CASE = 'casegen_case';
// casegen:end
}
```

## Edge cases to decide

- **Acronyms** — `HTTPServer` → `http_server` or `h_t_t_p_server`? Round-trip back
  to Pascal yields `HttpServer`. Most libs normalize; some keep an acronym list.
- **Digits** — `user2Name` / `user2name` / `user_2_name`: does a digit boundary
  start a new word?
- **Leading/trailing separators** — `_private`, `__dunder__`: usually preserved verbatim.
## Decided

- **Digits are transparent.** A digit never creates a word boundary on its own; it
  continues the current word. Boundaries come only from separators and *letter*
  case transitions. So `sha256`, `utf8`, `int32`, `x11` stay one word;
  `user2Name` → `user2` `name`; `2Girls1Cup` → `2` `girls1` `cup`.
  A digit run at the start of a word has nothing to attach to, so it stands alone.
- **ASCII only (0x00–0x7F).** Bytes >= 0x80 are treated as separators and dropped.
  `Příliš žluťoučký` → `p li lu ou k`. No transliteration, no Unicode case mapping.
- **One input line = one output line.** A blank line, or a line with no word
  characters (`//----`), produces a blank output line — it is not skipped.
  Keeps output aligned with input for `diff`/`paste`.

## Later (or never)

- **Warn on dropped non-ASCII bytes.** Right now they are dropped silently, which
  turns `Příliš` into the plausible-looking garbage `p li lu ou k`. Later: count
  them and emit one summary line to stderr at exit, e.g.
  `casegen: warning: dropped 14 non-ASCII bytes on 1 line (first at line 16)`.
  Note the message says *non-ASCII*, not "non-unicode" — the dropped bytes are
  UTF-8; ASCII is what's kept.
  - Only bytes >= 0x80 trigger it. Dropped punctuation (`,` `.` `/` `-` `_`) is
    normal operation and must stay silent.
  - Count once, print once at exit — never warn inside the splitter loop.
  - `NUL` deserves its own message: `fgets` reads it but every `str*` call then
    treats the line as ending there, so half a line is processed with no sign.
  - Default should stay exit 0 (the mangled output is visible on stdout anyway);
    add `--strict` to promote to fatal, and `-q` to suppress.
  - **Blocked on the test harness**: `runTests.sh:92` uses `first_match` on
    `output.returned.*`, so it only ever diffs one file. Adding
    `output.expected.err` would sort before `.txt` and silently stop checking
    stdout — every test would still report PASS. Fix the runner to pair files by
    extension first.
- **Replace the tty check with an argument check.** Right now bare `casegen` uses
  `isatty(STDIN_FILENO)` to detect "nothing piped in" and prints the placeholder
  greeting instead of blocking on the terminal. That is the only way to tell an
  interactive terminal from an empty pipe, but it makes behaviour depend on what
  stdin is attached to, which is not reproducible:

  | invocation | stdin is | result |
  |---|---|---|
  | `casegen` in a terminal | tty | greeting |
  | `cat f \| casegen` | pipe | reads |
  | `casegen < in.txt` | file | reads |
  | `casegen < /dev/null` | file | reads, immediate EOF, no output |

  Known consequence: **`tests/hello` is tty-dependent.** Its `command.sh` runs
  `casegen` with no stdin redirect, so it inherits whatever stdin `runTests.sh`
  had. Passes interactively; fails under `./tests/runTests.sh < /dev/null`, cron,
  or most CI — with nothing wrong in the code.

  Fix once flags exist: trigger on *no format argument given* → print usage to
  stderr, exit 2, before touching stdin. Argument-based, deterministic under any
  stdin, and `casegen --pascal` typed interactively still reads the terminal.

- **Distinguish EOF from read error.** `getline` returns -1 for both. `< /dev/null`
  and an empty file set `feof`; a closed fd 0 sets `ferror`/`EBADF` and a directory
  sets `ferror`/`EISDIR` — both currently exit 0 with no output, silently. Check
  `ferror(stdin)` after the loop and exit non-zero. Zero lines with `feof` set is
  *not* an error.

- **Unicode proper** — `ß` uppercases to `SS`, Turkish dotless `ı`, etc. Only if
  ASCII-only ever becomes painful.
- **Marker false positives** — any line containing `casegen:` is eaten, including a
  URL, a docblock mentioning the tool, or prose. Make the marker configurable
  (`--marker`) and warn on an unknown verb instead of silently dropping the line.
- **Trailing separators in loops** — generating a comma-separated list needs
  first/last or index access. PHP tolerates a trailing comma; SQL and JSON do not.
- **Index/position access** — no `$i` in v1, but decide whether the syntax is reserved.
- **Line endings** — CRLF input, and whether output preserves the input's ending.
