/* Included by casegen.c — not compiled on its own.
   See the note at the top of casegen.c for why there are no headers.

   The 17 cases, as a table. Adding one is a row, not a function. */


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
const CaseSpec *findCase(const char *s)
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
void renderCase(const StringList *words, const CaseSpec *spec, FILE *out)
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
