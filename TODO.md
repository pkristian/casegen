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
casegen [-i] [-q] [-c CASE] [FILE|-]...
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
- Strip trailing delimiters — `-->`, `*/`, `--%>`, `?>`, `#}` — **before** tokenizing,
  not after. Doing it first is what lets `<!--casegen:end-->` lex the same as
  `<!-- casegen:end -->`; strip afterwards and the verb token comes out as `end-->`.
  Strip repeatedly, too — `<?php /* casegen:raw */ ?>` closes two wrappers at once.

### Verbs

| Directive | Args | Where |
|---|---|---|
| `casegen:foreach` | `[as <Two Words>]` — renames the placeholder for this block | top level only |
| `casegen:end` | `[<verb>]` — optional label, validated against the innermost block | — |
| `casegen:raw` | — (block, until `casegen:end`) | anywhere but inside `raw` |
| `casegen:raw-next-line` | — | anywhere |
| `casegen:placeholder` | `<Two Words>` — renames the placeholder from here on | top level only |
| `casegen:var` | `<Two Words> = <value>` | top level only |
| `casegen:end-template` | — everything after is data, not template | anywhere; first wins |

### Placeholders

A placeholder is a **name**, not a syntax. Write the name in the case you want out and
that is what you get, so a template is a working example of its own output:

```php
const COL_CASEGEN_CASE = 'casegen_case';   →   const COL_USER_PROFILE = 'user_profile';
```

The default name is `Casegen Case`. It is rendered in all 17 non-alias cases to give 17
needles — aliases are skipped because they render identically to a canonical case. Scanning
a content line left to right, **longest match wins**, ties going to the binding defined
first, so nothing depends on scan order.

A `var` is the same object with a fixed value instead of a per-record one. That is the
whole reason it costs nothing: one scanner, one set of renderings, no second mechanism.
Two of them landing in one token falls out for free —
`'table_prefix_casegen_case'` → `'shop_customer_order'`.

Names must be **two or more words**. One word renders the same in snake, kebab, dot, path,
flat and lower alike, so the case the author wrote would no longer say which case they
meant — there would be nothing left for the scanner to read.

Matching is plain substring and unanchored, which is exactly what makes `COL_CASEGEN_CASE`
work. Output is never rescanned: a value that happens to contain a needle is left alone.

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
`foreach` block is emitted once. Same shape as awk's `BEGIN` / body / `END`.
(Once, but not *verbatim*: vars still substitute out there. Only the loop placeholder
cannot, having no record to bind to — see **Decided**.)

```php
<?php

class Columns
{
// casegen:foreach
    const COL_CASEGEN_CASE = 'casegen_case';
// casegen:end
}
```

## Decided

- **Substitution outside `foreach`: vars yes, the loop placeholder no.** This is what
  resolves the contradiction between *"every content line gets substitution"* and
  *"everything outside a `foreach` is emitted once, verbatim"* — both cannot hold. A var
  has a value at all times, so it substitutes anywhere. The loop placeholder outside a
  loop has no record to bind to and is a hard error, reported at `file:line`.
- **Blocks nest, via a stack.** `raw` inside `foreach` is the case that matters and is
  legal. `foreach` only opens at the top level, which keeps *one implicit collection*
  intact and rejects `foreach` inside `foreach` (there is nothing to nest over) and
  `foreach` inside `raw` (a literal region looped is just a literal region repeated).
  `raw` inside `raw` is an error too — it changes nothing. Real depth therefore never
  exceeds 2, but the code uses a stack anyway so `end` labels and the *"never closed"*
  diagnostic work the same at every level.
- **Declarations are top level only.** `var` and `placeholder` take effect from their line
  onward. Allowing them inside a `foreach` would raise the question of what a declaration
  means on the third replay of the body, and there is no answer worth having.
- **`raw` suppresses substitution, not the marker.** Directive lines are always consumed —
  that is how a `raw` block's own `casegen:end` is still found. The cost is that a line
  containing `casegen:` can never be emitted literally. See *marker false positives* below.
- **A `foreach` over zero records warns and exits 0**, suppressed by `-q`, and the message
  names the likely cause (a missing `casegen:end-template`). Records with no `foreach` to
  put them in warns too. This is the first thing `-q` has ever had to suppress.
- **Records that split to zero words are skipped**, so blank lines can space out a data
  block. Note this is the opposite of case mode's *one input line = one output line* —
  different mode, different job: there, a blank line is output; here, it is punctuation.
- **Template errors exit 2, not 1.** A malformed template is the user getting it wrong,
  like a bad flag — not a machine failure, which is what 1 means. They print
  `casegen: file:line: message` and no usage block, because the usage block has nothing
  to say about templates. A line that turns out to be an error is checked before any of it
  is written, so a failed render never leaves half a line on stdout.

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
    add `--strict` to promote to fatal, and `-q` to suppress. `-q` now suppresses
    the two template warnings, so the plumbing is there to reuse.
  - **Blocked on the test harness**: `runTests.sh:135` uses `first_match` on
    `output.returned.*`, so it only ever diffs one file. Adding
    `output.expected.err` would sort before `.txt` and silently stop checking
    stdout — every test would still report PASS. Fix the runner to pair files by
    extension first. (`tests/input-errors` and `tests/template-warnings` both
    sidestep this by folding stderr and exit codes into the single stdout file.)
- **Unicode proper** — `ß` uppercases to `SS`, Turkish dotless `ı`, etc. Only if
  ASCII-only ever becomes painful.
- **Marker false positives** — any line containing `casegen:` is eaten, including a
  URL, a docblock mentioning the tool, or prose. An unknown verb is at least loud now
  (`unknown directive "casegen:whatever"`, exit 2) rather than a line that silently
  vanishes, so the failure mode is a stopped build and not a missing line. What is left
  is making the marker configurable with `--marker`.
- **Trailing separators in loops** — generating a comma-separated list needs
  first/last or index access. PHP tolerates a trailing comma; SQL and JSON do not.
- **Index/position access** — no `$i` in v1, but decide whether the syntax is reserved.
- **Line endings** — CRLF input, and whether output preserves the input's ending.
