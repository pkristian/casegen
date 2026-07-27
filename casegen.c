#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


typedef struct
{
    char **item; /* array of owned strings */
    size_t count; /* how many are used     */
    size_t cap; /* how many fit          */
} StringList;


static void outOfMemory(void)
{
    fprintf(stderr, "casegen: out of memory\n");
    exit(1);
}


static void stringsPush(StringList *list, const char *s)
{
    if (list->count == list->cap)
    {
        const size_t newCap = list->cap ? list->cap * 2 : 16;
        char **grown = realloc(list->item, newCap * sizeof *grown);
        if (!grown)
            outOfMemory();
        list->item = grown;
        list->cap = newCap;
    }
    list->item[list->count] = strdup(s); /* the copy — this is the important bit */
    if (!list->item[list->count])
        outOfMemory();
    list->count++;
}


static void stringsFree(StringList *list)
{
    for (size_t i = 0; i < list->count; i++)
        free(list->item[i]);
    free(list->item);
    list->item = NULL;
    list->count = 0;
    list->cap = 0;
}


/* ASCII only, by decision. Hand-rolled rather than <ctype.h> so that bytes >= 0x80
   are unambiguously separators and nothing depends on the locale. */

static int isAsciiUpper(const unsigned char c)
{
    return c >= 'A' && c <= 'Z';
}


static int isAsciiLower(const unsigned char c)
{
    return c >= 'a' && c <= 'z';
}


static int isAsciiDigit(const unsigned char c)
{
    return c >= '0' && c <= '9';
}


static int isWordByte(const unsigned char c)
{
    return isAsciiUpper(c) || isAsciiLower(c) || isAsciiDigit(c);
}


static unsigned char toAsciiLower(const unsigned char c)
{
    return isAsciiUpper(c) ? (unsigned char)(c - 'A' + 'a') : c;
}


static unsigned char toAsciiUpper(const unsigned char c)
{
    return isAsciiLower(c) ? (unsigned char)(c - 'a' + 'A') : c;
}


/* Does a new word start at `cur`? Digits never trigger a boundary on their own —
   only separators (handled by the caller) and letter case transitions do.

       lower|digit -> Upper     user2|Name, Postgre|SQL
       Upper Upper -> Upper+lower   HTTP|Server, O|Auth2|Token  */
static int isWordBoundary(const unsigned char prev, const unsigned char cur, const unsigned char next)
{
    if (!isAsciiUpper(cur))
        return 0;
    if (!isAsciiUpper(prev))
        return 1;
    return isAsciiLower(next);
}


static void flushWord(char *word, size_t *wordLen, StringList *out)
{
    if (*wordLen == 0)
        return;
    word[*wordLen] = '\0';
    stringsPush(out, word);
    *wordLen = 0;
}


/* Split one line into lowercased words. Pure — no I/O, no formatting.
   A line with no word bytes yields no words at all, which is not an error. */
static void splitWords(const char *line, StringList *out)
{
    const unsigned char *s = (const unsigned char*)line;
    const size_t len = strlen(line);

    char *word = malloc(len + 1); /* no word can be longer than the line */
    if (!word)
        outOfMemory();
    size_t wordLen = 0;

    for (size_t i = 0; i < len; i++)
    {
        const unsigned char c = s[i];

        if (!isWordByte(c)) /* separator: ASCII punctuation, space, and every byte >= 0x80 */
        {
            flushWord(word, &wordLen, out);
            continue;
        }

        /* When wordLen is 0 there is no previous byte to compare against; when it is
           not, s[i - 1] is guaranteed to be a word byte. s[i + 1] is safe to read —
           at the last byte it is the terminating NUL, which is not a lowercase letter. */
        if (wordLen > 0 && isWordBoundary(s[i - 1], c, s[i + 1]))
            flushWord(word, &wordLen, out);

        word[wordLen++] = toAsciiLower(c);
    }

    flushWord(word, &wordLen, out);
    free(word);
}


/* ------------------------------------------------------------------ cases */

typedef enum
{
    STYLE_LOWER,
    STYLE_UPPER,
    STYLE_CAP /* first byte upper, rest lower */
} WordStyle;


typedef struct
{
    const char *name;
    char code; /* short code, 0 if the case has none */
    const char *sep;
    WordStyle first;
    WordStyle rest;
    int isAlias; /* aliases work but are hidden from --help */
} CaseSpec;


static const CaseSpec CASES[] = {
    /*  name                code  sep   first        rest                alias */
    {   "camel",            'c',  "",   STYLE_LOWER, STYLE_CAP,          0 },
    {   "pascal",           'P',  "",   STYLE_CAP,   STYLE_CAP,          0 },
    {   "snake",            's',  "_",  STYLE_LOWER, STYLE_LOWER,        0 },
    {   "screaming-snake",  'S',  "_",  STYLE_UPPER, STYLE_UPPER,        0 },
    {   "kebab",            'k',  "-",  STYLE_LOWER, STYLE_LOWER,        0 },
    {   "screaming-kebab",  'K',  "-",  STYLE_UPPER, STYLE_UPPER,        0 },
    {   "train",            0,    "-",  STYLE_CAP,   STYLE_CAP,          0 },
    {   "title",            0,    " ",  STYLE_CAP,   STYLE_CAP,          0 },
    {   "sentence",         0,    " ",  STYLE_CAP,   STYLE_LOWER,        0 },
    {   "lower",            'l',  " ",  STYLE_LOWER, STYLE_LOWER,        0 },
    {   "upper",            0,    " ",  STYLE_UPPER, STYLE_UPPER,        0 },
    {   "dot",              0,    ".",  STYLE_LOWER, STYLE_LOWER,        0 },
    {   "path",             0,    "/",  STYLE_LOWER, STYLE_LOWER,        0 },
    {   "ada",              0,    "_",  STYLE_CAP,   STYLE_CAP,          0 },
    {   "camel-snake",      0,    "_",  STYLE_LOWER, STYLE_CAP,          0 },
    {   "flat",             0,    "",   STYLE_LOWER, STYLE_LOWER,        0 },
    {   "upper-flat",       0,    "",   STYLE_UPPER, STYLE_UPPER,        0 },

    /* aliases — same output, other people's names for it */
    {   "constant",         0,    "_",  STYLE_UPPER, STYLE_UPPER,        1 },
    {   "macro",            0,    "_",  STYLE_UPPER, STYLE_UPPER,        1 },
    {   "upper-snake",      0,    "_",  STYLE_UPPER, STYLE_UPPER,        1 },
    {   "cobol",            0,    "-",  STYLE_UPPER, STYLE_UPPER,        1 },
    {   "dash",             0,    "-",  STYLE_LOWER, STYLE_LOWER,        1 },
    {   "lisp",             0,    "-",  STYLE_LOWER, STYLE_LOWER,        1 },
    {   "spinal",           0,    "-",  STYLE_LOWER, STYLE_LOWER,        1 },
    {   "http-header",      0,    "-",  STYLE_CAP,   STYLE_CAP,          1 },
    {   "studly",           0,    "",   STYLE_CAP,   STYLE_CAP,          1 },
    {   "pascal-snake",     0,    "_",  STYLE_CAP,   STYLE_CAP,          1 },
    {   "slash",            0,    "/",  STYLE_LOWER, STYLE_LOWER,        1 },
};

static const size_t CASE_COUNT = sizeof CASES / sizeof CASES[0];


/* A single-byte argument is looked up as a short code, anything longer as a name. */
static const CaseSpec *findCase(const char *s)
{
    if (s[0] != '\0' && s[1] == '\0')
    {
        for (size_t i = 0; i < CASE_COUNT; i++)
            if (CASES[i].code == s[0])
                return &CASES[i];
        return NULL;
    }
    for (size_t i = 0; i < CASE_COUNT; i++)
        if (strcmp(CASES[i].name, s) == 0)
            return &CASES[i];
    return NULL;
}


/* Words arrive already lowercased from splitWords, so LOWER is a no-op here. */
static void renderCase(const StringList *words, const CaseSpec *spec, FILE *out)
{
    for (size_t i = 0; i < words->count; i++)
    {
        if (i > 0)
            fputs(spec->sep, out);

        const char *w = words->item[i];
        const WordStyle style = (i == 0) ? spec->first : spec->rest;

        for (size_t j = 0; w[j] != '\0'; j++)
        {
            unsigned char c = (unsigned char)w[j];
            if (style == STYLE_UPPER || (style == STYLE_CAP && j == 0))
                c = toAsciiUpper(c);
            fputc(c, out);
        }
    }
}


/* ------------------------------------------------------------------ input */

/* Read whole lines, newline (and CR) stripped, appending to `out`. */
static void readLines(FILE *f, StringList *out)
{
    char *lineContent = NULL; /* getline allocates and reallocs this for us */
    size_t cap = 0; /* current capacity of that buffer            */
    ssize_t len; /* length of the line just read, or -1 at EOF */

    while ((len = getline(&lineContent, &cap, f)) != -1)
    {
        /* getline keeps the trailing newline — strip it, and \r for CRLF */
        while (len > 0 && (lineContent[len - 1] == '\n' || lineContent[len - 1] == '\r'))
            lineContent[--len] = '\0';

        stringsPush(out, lineContent);
    }

    free(lineContent); /* once, after the loop — not inside it */
}


/* A source is a path, or "-" for stdin at that position in the operand list. */
static void loadSource(const char *path, StringList *out)
{
    if (strcmp(path, "-") == 0)
    {
        readLines(stdin, out);
        if (ferror(stdin))
        {
            fprintf(stderr, "casegen: stdin: %s\n", strerror(errno));
            exit(1);
        }
        return;
    }

    FILE *f = fopen(path, "r");
    if (!f)
    {
        fprintf(stderr, "casegen: %s: %s\n", path, strerror(errno));
        exit(1);
    }
    readLines(f, out);
    if (ferror(f))
    {
        fprintf(stderr, "casegen: %s: %s\n", path, strerror(errno));
        exit(1);
    }
    fclose(f);
}


/* ------------------------------------------------------------------- args */

typedef struct
{
    const CaseSpec *spec; /* NULL means template mode */
    int quiet;
    int stdinOnly; /* -i */
    StringList sources;
} Options;


static void usage(FILE *out)
{
    fputs("Usage: casegen [-i] [-q] -c CASE [FILE|-]...\n"
          "\n"
          "Input:\n"
          "  FILE...          read in order; \"-\" is stdin at that position\n"
          "  -i               stdin only; error if any FILE is given\n"
          "  (none)           this message, exit 2 — stdin is never read implicitly\n"
          "\n"
          "Mode:\n"
          "  -c, --case=CASE  render every input line in CASE\n"
          "  (no -c)          template mode\n"
          "\n"
          "Other:\n"
          "  -q, --quiet      suppress warnings\n"
          "  -h, --help       this message on stdout, exit 0\n"
          "  --               end of options\n"
          "\n"
          "Cases:\n", out);

    StringList sample = {0};
    stringsPush(&sample, "casegen");
    stringsPush(&sample, "case");

    for (size_t i = 0; i < CASE_COUNT; i++)
    {
        if (CASES[i].isAlias)
            continue;
        if (CASES[i].code)
            fprintf(out, "  -%c  %-18s", CASES[i].code, CASES[i].name);
        else
            fprintf(out, "      %-18s", CASES[i].name);
        renderCase(&sample, &CASES[i], out);
        fputc('\n', out);
    }

    stringsFree(&sample);
}


static void usageError(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    fputs("casegen: ", stderr);
    vfprintf(stderr, fmt, ap);
    fputc('\n', stderr);
    va_end(ap);

    usage(stderr);
    exit(2);
}


static void parseArgs(const int argc, char *argv[], Options *opt)
{
    int endOfFlags = 0;

    for (int i = 1; i < argc; i++)
    {
        char *a = argv[i];

        if (!endOfFlags && strcmp(a, "--") == 0)
        {
            endOfFlags = 1;
            continue;
        }

        /* "-" on its own is an operand, not a flag */
        if (endOfFlags || a[0] != '-' || a[1] == '\0')
        {
            stringsPush(&opt->sources, a);
            continue;
        }

        if (a[1] == '-') /* long option */
        {
            char *name = a + 2;
            char *eq = strchr(name, '=');
            char *value = NULL;

            if (eq)
            {
                *eq = '\0';
                value = eq + 1;
            }

            if (strcmp(name, "case") == 0)
            {
                if (!value)
                {
                    if (i + 1 >= argc)
                        usageError("--case requires a case name");
                    value = argv[++i];
                }
                opt->spec = findCase(value);
                if (!opt->spec)
                    usageError("unknown case \"%s\"", value);
            }
            else if (strcmp(name, "quiet") == 0)
                opt->quiet = 1;
            else if (strcmp(name, "help") == 0)
            {
                usage(stdout);
                exit(0);
            }
            else
                usageError("unknown option \"--%s\"", name);

            continue;
        }

        /* short bundle: -i, -q, -iq, -cs, -ic snake, -ics */
        int valueTaken = 0;
        for (const char *p = a + 1; *p != '\0' && !valueTaken; p++)
        {
            switch (*p)
            {
                case 'i':
                    opt->stdinOnly = 1;
                    break;
                case 'q':
                    opt->quiet = 1;
                    break;
                case 'h':
                    usage(stdout);
                    exit(0);
                case 'c':
                {
                    const char *value = p + 1; /* rest of the token, if any */
                    if (*value == '\0')
                    {
                        if (i + 1 >= argc)
                            usageError("-c requires a case");
                        value = argv[++i];
                    }
                    opt->spec = findCase(value);
                    if (!opt->spec)
                        usageError("unknown case \"%s\"", value);
                    valueTaken = 1;
                    break;
                }
                default:
                    usageError("unknown option \"-%c\"", *p);
            }
        }
    }
}


/* -i and file operands are mutually exclusive on purpose: the point of -i is that
   the whole input is the pipe. Once files are in play the user has to say where
   the stream belongs, and "-" is how they say it. */
static void validateArgs(Options *opt)
{
    if (opt->stdinOnly && opt->sources.count > 0)
    {
        fprintf(stderr, "casegen: -i means stdin only, but %zu file(s) were given\n",
                opt->sources.count);
        fprintf(stderr, "casegen: use \"-\" to say where the stream belongs, e.g.\n");
        fprintf(stderr, "casegen:     casegen -c snake %s -\n", opt->sources.item[0]);
        exit(2);
    }

    if (opt->stdinOnly)
        stringsPush(&opt->sources, "-");

    if (opt->sources.count == 0)
        usageError("no input given");

    size_t stdinCount = 0;
    for (size_t i = 0; i < opt->sources.count; i++)
        if (strcmp(opt->sources.item[i], "-") == 0)
            stdinCount++;

    if (stdinCount > 1)
        usageError("\"-\" given %zu times; stdin can only be read once", stdinCount);
}


int main(const int argc, char *argv[])
{
    Options opt = {0};
    parseArgs(argc, argv, &opt);
    validateArgs(&opt);

    if (!opt.spec)
    {
        fprintf(stderr, "casegen: template mode is not implemented yet\n");
        fprintf(stderr, "casegen: use -c CASE to render input lines (see --help)\n");
        exit(1);
    }

    StringList lines = {0};
    for (size_t i = 0; i < opt.sources.count; i++)
        loadSource(opt.sources.item[i], &lines);

    /* One input line = one output line, so a line with no words prints blank. */
    for (size_t i = 0; i < lines.count; i++)
    {
        StringList words = {0};
        splitWords(lines.item[i], &words);
        renderCase(&words, opt.spec, stdout);
        fputc('\n', stdout);
        stringsFree(&words);
    }

    stringsFree(&lines);
    stringsFree(&opt.sources);
    return 0;
}
