/* The 17 cases, as a table. Adding one is a row, not a function. */

#include "cases.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "ascii.h"
#include "mem.h"


const CaseSpec CASES[] = {
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

/* The table above is the authority; this catches CASE_COUNT drifting from it either way.
   See cases.h for why the declaration there must stay unbounded for this to work. */
static_assert(sizeof CASES / sizeof CASES[0] == CASE_COUNT,
              "CASE_COUNT in cases.h no longer matches the rows in CASES");


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
char *renderCaseAlloc(const StringList *words, const CaseSpec *spec)
{
    const size_t sepLen = strlen(spec->sep);
    size_t len = words->count ? sepLen * (words->count - 1) : 0;
    for (size_t i = 0; i < words->count; i++)
        len += strlen(words->item[i]);

    char *rendered = malloc(len + 1);
    if (!rendered)
        outOfMemory();

    size_t n = 0;
    for (size_t i = 0; i < words->count; i++)
    {
        if (i > 0)
        {
            memcpy(rendered + n, spec->sep, sepLen);
            n += sepLen;
        }

        const char *w = words->item[i];
        const WordStyle style = (i == 0) ? spec->first : spec->rest;

        for (size_t j = 0; w[j] != '\0'; j++)
        {
            const unsigned char c = (unsigned char)w[j];
            const int upper = (style == STYLE_UPPER) || (style == STYLE_CAP && j == 0);
            rendered[n++] = (char)(upper ? toAsciiUpper(c) : c);
        }
    }

    rendered[n] = '\0';
    return rendered;
}


void renderCase(const StringList *words, const CaseSpec *spec, FILE *out)
{
    char *rendered = renderCaseAlloc(words, spec);
    fputs(rendered, out);
    free(rendered);
}
