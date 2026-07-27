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

## Invocation

```
casegen [-i] [-q] -c CASE [FILE|-]...
```

Input is the concatenation of the operands, in order. `-` is stdin *at that
position*, so placement is explicit instead of being encoded in a flag:

| invocation | input |
|---|---|
| `casegen -c s f1 f2` | f1, f2 |
| `casegen -c s -i` | stdin only |
| `casegen -c s f1 f2 -` | f1, f2, stdin |
| `casegen -c s - f1 f2` | stdin, f1, f2 |
| `casegen -c s f1 - f2` | stdin between the two |
| `casegen` | usage on stderr, exit 2 |

`-i` means *stdin is the whole input*; passing it alongside a file is an error whose
message points at `-`. Stdin is never read unless `-i` or `-` asks for it.

`-c` selects case mode: every input line is one record. Without `-c` the input is a
template. The case accepts a short code or a full name in any standard spelling —
`-cs`, `-c s`, `-c snake`, `-csnake`, `--case=snake`, `--case snake`. Short codes
exist only for the six most used plus `lower`: `c P s S k K l`. The aliases in the
tables above (`constant`, `cobol`, `spinal`, `http-header`, `studly`, …) are
accepted but hidden from `--help`.

Exit codes: `0` fine, `1` runtime failure (unreadable file, I/O), `2` usage error.

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

Decided by the operand model: a template file and `-` are both just operands, read
in order, so `casegen columns.tmpl -` appends stdin's records to whatever the file's
own data section already holds. Concatenation — and not as a rule the template layer
has to know about, but as a consequence of how input is assembled. This is also why
there is no `-t` flag.

### Loop model

One implicit collection: the newline-delimited records that follow
`casegen:end-template` in the assembled input. `foreach` takes no
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

### Still open before the engine is written

- **Does substitution happen outside `foreach`?** The parsing rule says every
  content line gets substitution; the loop model says everything outside a
  `foreach` is emitted *"once, verbatim"*. Both cannot hold. Suggested: constants
  substitute everywhere, and the loop placeholder outside a loop is an error —
  it has no record to bind to.
- **A `foreach` over zero records** — silent empty output, or a warning? Warning
  (suppressed by `-q`), exit 0, seems right.
- **Is `raw` inherited into `foreach`?** Nesting needs a stack. If v1 stays flat,
  `foreach` inside `raw` should be a hard error rather than quietly doing something.
- **`casegen:var <Two Words> = <value>`** — proposed, not yet in the verb table. A
  var is just a second placeholder bound to a constant instead of to the loop
  record, so it reuses the entire substitution engine: same renderings, same
  scanner, no new rendering code. Two vars can land inside one token —
  `'table_prefix_entity_name'` → `'shop_customer_order'` — which falls out for
  free and is the main argument for making vars placeholders rather than a
  separate mechanism.

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
- **Acronyms end where the next word's lowercase begins.** A run of uppercase
  splits before its *last* letter when a lowercase follows — `HTTP|Server`,
  `XML|Http|Request`, `O|Auth2|Token`. With no trailing lowercase the run stays
  whole: `Postgre|SQL`, `get|ID`. No acronym list, no dictionary. Accepted
  consequence: round-tripping back through Pascal yields `HttpServer`.

  Rejected: requiring a run of **3 or more** so `OAuth` and `IDs` stay joined. It
  fixes exactly those two and breaks every single-letter prefix — `XValue` →
  `xvalue`, `TResult` → `tresult`, `IService` → `iservice`, `EBadRequest` →
  `ebad request`. Both shapes are `[Upper][Upper][lower]` and no rule separates
  them. The prefixes are systematic (generics, interfaces, exceptions); the
  acronyms are a short closed list. Over-splitting gives `o auth`, wrong but
  visibly so; under-splitting invents plausible words that survive review. If
  `OAuth` ever matters, add a post-split rejoin table — do not touch the
  boundary predicate.
- **Leading and trailing separators are dropped**, not preserved: `_leading` →
  `leading`, `__dunder__` → `dunder`, `___` → a blank line. Keeping affixes is a
  rendering concern; the splitter's job is words.
- **Stdin is opt-in, via `-i` or `-`.** The `isatty` check is gone: bare `casegen`
  prints usage and exits 2 without touching stdin, so behaviour is identical under
  a terminal, a pipe, `< /dev/null`, cron and CI. This retired `tests/hello`, which
  was tty-dependent by construction.
- **EOF is distinguished from a read error.** `ferror` is checked after every
  source; a directory (`EISDIR`) or a closed fd 0 (`EBADF`) names the path and
  exits 1, while an empty file or `< /dev/null` is zero records and exit 0.

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
    add `--strict` to promote to fatal, and `-q` to suppress. `-q` already exists
    and is parsed — it simply has no warnings to suppress yet.
  - **Blocked on the test harness**: `runTests.sh:135` uses `first_match` on
    `output.returned.*`, so it only ever diffs one file. Adding
    `output.expected.err` would sort before `.txt` and silently stop checking
    stdout — every test would still report PASS. Fix the runner to pair files by
    extension first. (`tests/input-errors` sidesteps this by folding stderr and
    exit codes into the single stdout file itself.)
- **Unicode proper** — `ß` uppercases to `SS`, Turkish dotless `ı`, etc. Only if
  ASCII-only ever becomes painful.
- **Marker false positives** — any line containing `casegen:` is eaten, including a
  URL, a docblock mentioning the tool, or prose. Make the marker configurable
  (`--marker`) and warn on an unknown verb instead of silently dropping the line.
- **Trailing separators in loops** — generating a comma-separated list needs
  first/last or index access. PHP tolerates a trailing comma; SQL and JSON do not.
- **Index/position access** — no `$i` in v1, but decide whether the syntax is reserved.
- **Line endings** — CRLF input, and whether output preserves the input's ending.
