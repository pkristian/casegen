/* Template mode: directives, placeholders, and the substitution scanner.

   The one idea worth stating up front: a placeholder is a *name*, not a syntax. Write
   the name in the case you want out, and that is what you get —

       const COL_CASEGEN_CASE = 'casegen_case';   ->   const COL_USER_PROFILE = 'user_profile';

   so the template is a working example of its own output. Everything below exists to
   support that: render the name in all 17 cases, look for any of them, and emit the
   value in whichever one matched. */

#include "template.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ascii.h"
#include "cases.h"
#include "input.h"
#include "mem.h"
#include "split.h"
#include "stringlist.h"


#define MARKER "casegen:"
#define DEFAULT_PLACEHOLDER "Casegen Case"

/* raw may nest inside foreach and nothing else nests, so 2 is the real ceiling.
   The extra slack is only so the bound check has something to say. */
#define MAX_BLOCK_DEPTH 8


/* Newlines are already stripped by readLines, so these are the only two left. */
static int isSpaceByte(const unsigned char c)
{
    return c == ' ' || c == '\t';
}


static char *dupRange(const char *s, const size_t len)
{
    char *out = malloc(len + 1);
    if (!out)
        outOfMemory();
    memcpy(out, s, len);
    out[len] = '\0';
    return out;
}


/* "columns.tmpl:14", into caller storage — a message about one line sometimes has to
   name another (where the block it fails to close was opened). */
static void formatLocation(const SourceMap *map, const size_t index, char *buf,
                           const size_t cap)
{
    const char *path;
    size_t lineNo;
    locateLine(map, index, &path, &lineNo);
    snprintf(buf, cap, "%s:%zu", path, lineNo);
}


/* A bad template is the user getting it wrong, like a bad flag, so it exits 2 — but it
   prints no usage block, because the usage block has nothing to say about templates. */
static _Noreturn void templateError(const SourceMap *map, const size_t index,
                                    const char *fmt, ...)
{
    char where[512];
    formatLocation(map, index, where, sizeof where);

    va_list ap;
    va_start(ap, fmt);
    fprintf(stderr, "casegen: %s: ", where);
    vfprintf(stderr, fmt, ap);
    fputc('\n', stderr);
    va_end(ap);

    exit(2);
}


static void templateWarning(const SourceMap *map, const size_t index, const int quiet,
                            const char *fmt, ...)
{
    if (quiet)
        return;

    char where[512];
    formatLocation(map, index, where, sizeof where);

    va_list ap;
    va_start(ap, fmt);
    fprintf(stderr, "casegen: %s: warning: ", where);
    vfprintf(stderr, fmt, ap);
    fputc('\n', stderr);
    va_end(ap);
}


/* ---------------------------------------------------------------- bindings -- */


/* A placeholder: a name rendered in every case, and a value rendered the same way.
   The loop binding's value is swapped once per record; a var's value is fixed. Both are
   this struct, which is the whole reason vars cost nothing — the scanner cannot tell
   them apart except when reporting the one error that distinguishes them. */
typedef struct
{
    char *name; /* as written, so `foreach as` can save and restore it */
    StringList nameWords;
    char *needle[CASE_COUNT]; /* NULL on alias rows: they duplicate a canonical one */
    size_t needleLen[CASE_COUNT];
    StringList valueWords;
    int isLoop;
    int declared; /* the loop binding starts undeclared: it is the built-in default */
    size_t definedAt;
} Binding;


static void bindingSetName(Binding *b, const char *name)
{
    free(b->name);
    stringsFree(&b->nameWords);
    for (size_t i = 0; i < CASE_COUNT; i++)
    {
        free(b->needle[i]);
        b->needle[i] = NULL;
        b->needleLen[i] = 0;
    }

    b->name = strdup(name);
    if (!b->name)
        outOfMemory();
    splitWords(name, &b->nameWords);

    /* Aliases are skipped because they render identically to a canonical case: keeping
       them would mean the same needle several times over, matching the same text. */
    for (size_t i = 0; i < CASE_COUNT; i++)
    {
        if (CASES[i].isAlias)
            continue;
        b->needle[i] = renderCaseAlloc(&b->nameWords, &CASES[i]);
        b->needleLen[i] = strlen(b->needle[i]);
    }
}


static void bindingSetValue(Binding *b, const char *value)
{
    stringsFree(&b->valueWords);
    splitWords(value, &b->valueWords);
}


static void bindingFree(Binding *b)
{
    free(b->name);
    stringsFree(&b->nameWords);
    stringsFree(&b->valueWords);
    for (size_t i = 0; i < CASE_COUNT; i++)
        free(b->needle[i]);
}


typedef struct
{
    Binding *item;
    size_t count;
    size_t cap;
} BindingList;


/* Returns the new, zeroed slot. Note that the pointer dies at the next push — always
   reach the loop binding as &list->item[0], never through a saved pointer. */
static Binding *bindingsPush(BindingList *list)
{
    if (list->count == list->cap)
    {
        const size_t newCap = list->cap ? list->cap * 2 : 8;
        Binding *grown = realloc(list->item, newCap * sizeof *grown);
        if (!grown)
            outOfMemory();
        list->item = grown;
        list->cap = newCap;
    }
    Binding *b = &list->item[list->count++];
    memset(b, 0, sizeof *b);
    return b;
}


static void bindingsFree(BindingList *list)
{
    for (size_t i = 0; i < list->count; i++)
        bindingFree(&list->item[i]);
    free(list->item);
    list->item = NULL;
    list->count = 0;
    list->cap = 0;
}


static int sameWords(const StringList *a, const StringList *b)
{
    if (a->count != b->count)
        return 0;
    for (size_t i = 0; i < a->count; i++)
        if (strcmp(a->item[i], b->item[i]) != 0)
            return 0;
    return 1;
}


/* One word renders the same in snake, kebab, dot, path, flat and lower alike, so the
   case the author wrote would no longer say which case they meant. Two words is what
   makes all 17 renderings distinct, which is what the scanner reads. */
static void requireTwoWords(const SourceMap *map, const size_t index, const char *what,
                            const char *name)
{
    StringList words = {0};
    splitWords(name, &words);
    const size_t count = words.count;
    stringsFree(&words);

    if (count < 2)
        templateError(map, index,
                      "%s needs a name of two or more words, got \"%s\" — one word looks "
                      "the same in most cases, leaving nothing to tell them apart",
                      what, name);
}


/* Two bindings answering to the same name would make the longest-match tie-break decide
   which one wins, which is no way to find out. `skip` is the binding being renamed. */
static void requireNameFree(const BindingList *list, const SourceMap *map, const size_t index,
                            const char *name, const size_t skip)
{
    StringList words = {0};
    splitWords(name, &words);

    for (size_t i = 0; i < list->count; i++)
    {
        if (i == skip || !sameWords(&words, &list->item[i].nameWords))
            continue;

        stringsFree(&words);

        if (!list->item[i].declared)
            templateError(map, index,
                          "\"%s\" is the built-in loop placeholder — pick another name, "
                          "or rename it first with casegen:placeholder",
                          name);

        char where[512];
        formatLocation(map, list->item[i].definedAt, where, sizeof where);
        templateError(map, index, "\"%s\" is already the %s defined at %s", name,
                      list->item[i].isLoop ? "loop placeholder" : "var", where);
    }

    stringsFree(&words);
}


/* ---------------------------------------------------------------- scanning -- */


/* Copy `line` to `out`, replacing each placeholder with its value rendered in the case
   the template wrote the placeholder in. Longest match wins and ties go to the binding
   defined first, so two bindings can land inside one token — `table_prefix_casegen_case`
   becomes `shop_customer_order` — without the result depending on scan order.
   Output is never rescanned: a value that happens to contain a needle is left alone.

   A NULL `out` checks the line without emitting it, so a line that turns out to be an
   error does not leave half of itself on stdout first. */
static void substituteLine(const char *line, const BindingList *list, const int inLoop,
                           const SourceMap *map, const size_t index, FILE *out)
{
    for (size_t i = 0; line[i] != '\0';)
    {
        const Binding *hit = NULL;
        size_t hitCase = 0;
        size_t hitLen = 0;

        for (size_t b = 0; b < list->count; b++)
        {
            const Binding *binding = &list->item[b];
            for (size_t c = 0; c < CASE_COUNT; c++)
            {
                /* > and not >=, so an equal-length match keeps the earlier binding */
                if (!binding->needle[c] || binding->needleLen[c] <= hitLen)
                    continue;
                if (strncmp(line + i, binding->needle[c], binding->needleLen[c]) != 0)
                    continue;
                hit = binding;
                hitCase = c;
                hitLen = binding->needleLen[c];
            }
        }

        if (!hit)
        {
            if (out)
                fputc(line[i], out);
            i++;
            continue;
        }

        if (hit->isLoop && !inLoop)
            templateError(map, index,
                          "placeholder \"%s\" outside foreach — there is no record to "
                          "bind it to",
                          hit->needle[hitCase]);

        if (out)
        {
            char *value = renderCaseAlloc(&hit->valueWords, &CASES[hitCase]);
            fputs(value, out);
            free(value);
        }
        i += hitLen;
    }
}


/* ---------------------------------------------------------------- lexing -- */


/* Trailing comment closers, stripped once before the line is tokenized. Doing it first
   rather than last is what lets `<!--casegen:end-->` lex the same as `<!-- casegen:end -->`:
   otherwise the verb token would come out as `end-->`. */
static const char *const DELIMITERS[] = {"-->", "--%>", "*/", "?>", "#}"};

#define DELIMITER_COUNT (sizeof DELIMITERS / sizeof DELIMITERS[0])


/* Split a marker line into verb and args, both freshly allocated and trimmed. Returns 0
   for a content line, having allocated nothing.

   Note what is *not* here: no comment-syntax awareness. Whatever wraps the marker is
   simply part of a line that gets thrown away, which is why every comment style works. */
static int parseDirective(const char *line, char **verb, char **args)
{
    const char *marker = strstr(line, MARKER);
    if (!marker)
        return 0;

    const char *tail = marker + strlen(MARKER);
    size_t len = strlen(tail);

    while (len > 0 && isSpaceByte((unsigned char)tail[len - 1]))
        len--;

    /* Repeatedly, because one marker can sit inside two wrappers at once — a PHP tag
       holding a C comment closes both on the way out. */
    for (int stripped = 1; stripped;)
    {
        stripped = 0;
        for (size_t d = 0; d < DELIMITER_COUNT; d++)
        {
            const size_t dLen = strlen(DELIMITERS[d]);
            if (len < dLen || memcmp(tail + len - dLen, DELIMITERS[d], dLen) != 0)
                continue;
            len -= dLen;
            while (len > 0 && isSpaceByte((unsigned char)tail[len - 1]))
                len--;
            stripped = 1;
            break;
        }
    }

    size_t start = 0;
    while (start < len && isSpaceByte((unsigned char)tail[start]))
        start++;

    size_t verbEnd = start;
    while (verbEnd < len && !isSpaceByte((unsigned char)tail[verbEnd]))
        verbEnd++;

    *verb = dupRange(tail + start, verbEnd - start);

    size_t argsStart = verbEnd;
    while (argsStart < len && isSpaceByte((unsigned char)tail[argsStart]))
        argsStart++;

    *args = dupRange(tail + argsStart, len - argsStart);
    return 1;
}


/* ---------------------------------------------------------------- blocks -- */


typedef struct
{
    const char *verb; /* "foreach" or "raw" — a literal, not owned */
    size_t line;
} Block;


/* One line of a foreach body, held by index so diagnostics keep their provenance.
   `raw` is per line because raw-next-line can protect a single line inside the loop. */
typedef struct
{
    size_t index;
    int raw;
} BodyLine;


typedef struct
{
    BodyLine *item;
    size_t count;
    size_t cap;
} Body;


static void bodyPush(Body *body, const size_t index, const int raw)
{
    if (body->count == body->cap)
    {
        const size_t newCap = body->cap ? body->cap * 2 : 16;
        BodyLine *grown = realloc(body->item, newCap * sizeof *grown);
        if (!grown)
            outOfMemory();
        body->item = grown;
        body->cap = newCap;
    }
    body->item[body->count++] = (BodyLine){index, raw};
}


static int rawIsOpen(const Block *blocks, const size_t depth)
{
    for (size_t i = 0; i < depth; i++)
        if (strcmp(blocks[i].verb, "raw") == 0)
            return 1;
    return 0;
}


static void emitLine(const char *text, const BindingList *list, const int raw, const int inLoop,
                     const SourceMap *map, const size_t index, FILE *out)
{
    if (raw)
    {
        fputs(text, out);
        fputc('\n', out);
        return;
    }

    /* Outside a loop a placeholder is an error, so check the whole line before writing
       any of it. Inside one there is nothing left to fail on. */
    if (!inLoop)
        substituteLine(text, list, inLoop, map, index, NULL);

    substituteLine(text, list, inLoop, map, index, out);
    fputc('\n', out);
}


static void requireNoArgs(const SourceMap *map, const size_t index, const char *verb,
                          const char *args)
{
    if (args[0] != '\0')
        templateError(map, index, "casegen:%s takes no arguments, got \"%s\"", verb, args);
}


static void requireTopLevel(const SourceMap *map, const size_t index, const char *verb,
                            const Block *blocks, const size_t depth)
{
    if (depth == 0)
        return;

    char where[512];
    formatLocation(map, blocks[depth - 1].line, where, sizeof where);
    templateError(map, index,
                  "casegen:%s inside %s (opened at %s) — declarations belong at the top "
                  "level, where it is plain which lines they cover",
                  verb, blocks[depth - 1].verb, where);
}


/* ---------------------------------------------------------------- rendering -- */


void renderTemplate(const StringList *lines, const SourceMap *map, const int quiet)
{
    /* Pass 1: find where the template stops and the data begins. This has to happen up
       front because a foreach near the top needs records that appear at the bottom. */
    size_t templateEnd = lines->count;
    for (size_t i = 0; i < lines->count; i++)
    {
        char *verb = NULL;
        char *args = NULL;
        if (!parseDirective(lines->item[i], &verb, &args))
            continue;

        const int isEnd = strcmp(verb, "end-template") == 0;
        free(verb);
        free(args);

        if (isEnd)
        {
            templateEnd = i;
            break;
        }
    }

    /* Records are read verbatim: never scanned for directives or placeholders. A line
       with no word bytes is skipped, so blank lines can space the data out. */
    StringList records = {0};
    for (size_t i = templateEnd + 1; i < lines->count; i++)
    {
        StringList words = {0};
        splitWords(lines->item[i], &words);
        const size_t count = words.count;
        stringsFree(&words);
        if (count > 0)
            stringsPush(&records, lines->item[i]);
    }

    BindingList bindings = {0};
    Binding *loop = bindingsPush(&bindings); /* always index 0 */
    bindingSetName(loop, DEFAULT_PLACEHOLDER);
    loop->isLoop = 1;

    Block blocks[MAX_BLOCK_DEPTH] = {{0}};
    size_t depth = 0;
    Body body = {0};
    int inForeach = 0;
    size_t foreachLine = 0;
    int rawNext = 0; /* raw-next-line, waiting for a content line to land on */
    char *savedName = NULL; /* set by `foreach as`, restored at its end */
    int savedDeclared = 0;
    size_t savedDefinedAt = 0;
    size_t loopsRun = 0;

    /* Pass 2. */
    for (size_t i = 0; i < templateEnd; i++)
    {
        const char *line = lines->item[i];
        char *verb = NULL;
        char *args = NULL;

        if (!parseDirective(line, &verb, &args))
        {
            /* Directives never consume raw-next-line, so it survives to the next line
               that actually gets emitted — which is the line it was written to protect. */
            const int raw = rawIsOpen(blocks, depth) || rawNext;
            rawNext = 0;

            if (inForeach)
                bodyPush(&body, i, raw);
            else
                emitLine(line, &bindings, raw, 0, map, i, stdout);
            continue;
        }

        if (verb[0] == '\0')
            templateError(map, i, "the marker has no directive after it");

        for (const char *p = verb; *p != '\0'; p++)
            if (isAsciiUpper((unsigned char)*p))
                templateError(map, i,
                              "directive \"%s\" is not lowercase — this is a case tool, "
                              "so it will not quietly convert one for you",
                              verb);

        if (strcmp(verb, "end-template") == 0)
        {
            free(verb);
            free(args);
            break; /* unreachable: pass 1 already stopped the loop here */
        }

        if (strcmp(verb, "foreach") == 0)
        {
            if (depth > 0)
            {
                char where[512];
                formatLocation(map, blocks[depth - 1].line, where, sizeof where);
                templateError(map, i, "foreach inside %s (opened at %s) — %s",
                              blocks[depth - 1].verb, where,
                              strcmp(blocks[depth - 1].verb, "raw") == 0
                                  ? "a raw region is literal text, so looping it would "
                                    "only repeat it"
                                  : "there is one collection, so loops do not nest");
            }

            if (args[0] != '\0')
            {
                if (strncmp(args, "as", 2) != 0 || !isSpaceByte((unsigned char)args[2]))
                    templateError(map, i,
                                  "foreach takes only \"as <Two Words>\", got \"%s\"", args);

                const char *name = args + 2;
                while (isSpaceByte((unsigned char)*name))
                    name++;

                requireTwoWords(map, i, "foreach as", name);
                requireNameFree(&bindings, map, i, name, 0);

                /* The rename is undone at this block's end, and so is where it came
                   from — otherwise a later collision would be blamed on this line. */
                savedName = strdup(bindings.item[0].name);
                if (!savedName)
                    outOfMemory();
                savedDeclared = bindings.item[0].declared;
                savedDefinedAt = bindings.item[0].definedAt;

                bindingSetName(&bindings.item[0], name);
                bindings.item[0].declared = 1;
                bindings.item[0].definedAt = i;
            }

            blocks[depth++] = (Block){"foreach", i};
            inForeach = 1;
            foreachLine = i;
            body.count = 0;
        }
        else if (strcmp(verb, "raw") == 0)
        {
            requireNoArgs(map, i, verb, args);

            if (rawIsOpen(blocks, depth))
                templateError(map, i, "raw is already open — nesting it changes nothing");
            if (depth == MAX_BLOCK_DEPTH)
                templateError(map, i, "blocks nested more than %d deep", MAX_BLOCK_DEPTH);

            blocks[depth++] = (Block){"raw", i};
        }
        else if (strcmp(verb, "raw-next-line") == 0)
        {
            requireNoArgs(map, i, verb, args);
            rawNext = 1;
        }
        else if (strcmp(verb, "placeholder") == 0)
        {
            requireTopLevel(map, i, verb, blocks, depth);
            requireTwoWords(map, i, "placeholder", args);
            requireNameFree(&bindings, map, i, args, 0);
            bindingSetName(&bindings.item[0], args);
            bindings.item[0].declared = 1;
            bindings.item[0].definedAt = i;
        }
        else if (strcmp(verb, "var") == 0)
        {
            requireTopLevel(map, i, verb, blocks, depth);

            char *eq = strchr(args, '=');
            if (!eq)
                templateError(map, i,
                              "var needs \"<Two Words> = <value>\", got \"%s\"", args);

            /* Cut the name out of args in place, trailing spaces and all, so the name
               reads back cleanly in any diagnostic that quotes it. */
            char *nameEnd = eq;
            while (nameEnd > args && isSpaceByte((unsigned char)nameEnd[-1]))
                nameEnd--;
            *nameEnd = '\0';

            const char *value = eq + 1;
            while (isSpaceByte((unsigned char)*value))
                value++;

            requireTwoWords(map, i, "var", args);
            requireNameFree(&bindings, map, i, args, bindings.count);

            StringList valueWords = {0};
            splitWords(value, &valueWords);
            const size_t valueCount = valueWords.count;
            stringsFree(&valueWords);
            if (valueCount == 0)
                templateError(map, i,
                              "var \"%s\" has no value — it would erase the placeholder "
                              "rather than replace it",
                              args);

            Binding *var = bindingsPush(&bindings);
            bindingSetName(var, args);
            bindingSetValue(var, value);
            var->declared = 1;
            var->definedAt = i;
        }
        else if (strcmp(verb, "end") == 0)
        {
            if (depth == 0)
                templateError(map, i, "casegen:end with no open block");

            const Block block = blocks[--depth];
            if (args[0] != '\0' && strcmp(args, block.verb) != 0)
            {
                char where[512];
                formatLocation(map, block.line, where, sizeof where);
                templateError(map, i, "end %s closes the %s opened at %s", args,
                              block.verb, where);
            }

            if (strcmp(block.verb, "foreach") == 0)
            {
                if (records.count == 0)
                    templateWarning(map, foreachLine, quiet,
                                    "foreach matched 0 records — is a "
                                    "casegen:end-template section missing?");

                for (size_t r = 0; r < records.count; r++)
                {
                    bindingSetValue(&bindings.item[0], records.item[r]);
                    for (size_t k = 0; k < body.count; k++)
                        emitLine(lines->item[body.item[k].index], &bindings,
                                 body.item[k].raw, 1, map, body.item[k].index, stdout);
                }

                loopsRun++;
                inForeach = 0;
                body.count = 0;

                if (savedName)
                {
                    bindingSetName(&bindings.item[0], savedName);
                    bindings.item[0].declared = savedDeclared;
                    bindings.item[0].definedAt = savedDefinedAt;
                    free(savedName);
                    savedName = NULL;
                }
            }
        }
        else
            templateError(map, i, "unknown directive \"casegen:%s\"", verb);

        free(verb);
        free(args);
    }

    if (depth > 0)
        templateError(map, blocks[depth - 1].line, "%s is never closed",
                      blocks[depth - 1].verb);

    if (records.count > 0 && loopsRun == 0 && !quiet)
        fprintf(stderr,
                "casegen: warning: %zu record(s) after casegen:end-template, but the "
                "template has no casegen:foreach to put them in\n",
                records.count);

    free(body.item);
    bindingsFree(&bindings);
    stringsFree(&records);
}
