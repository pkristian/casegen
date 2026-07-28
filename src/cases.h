#ifndef CASEGEN_CASES_H
#define CASEGEN_CASES_H

#include <stdio.h>

#include "stringlist.h"

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


/* Rows in CASES: 17 real cases and 11 aliases. Spelled out rather than derived with
   sizeof, because the template scanner sizes a per-case array with it and needs a
   constant. cases.c static-asserts that the two never drift apart.

   CASES is deliberately declared *without* a bound. Writing CASES[CASE_COUNT] here
   would make this number authoritative over the table itself: add a row and the
   compiler drops it with a warning, and sizeof in cases.c then reports the clamped
   size, so the assertion would pass while a case had quietly gone missing. */
#define CASE_COUNT 28

extern const CaseSpec CASES[];


/* A single-byte argument is looked up as a short code, anything longer as a name.
   NULL if no case answers to it. */
const CaseSpec *findCase(const char *s);

/* Render `words` in `spec`. The Alloc form returns a string the caller owns; it is the
   primitive, because the template engine renders into memory to compare against. */
char *renderCaseAlloc(const StringList *words, const CaseSpec *spec);
void renderCase(const StringList *words, const CaseSpec *spec, FILE *out);

#endif /* CASEGEN_CASES_H */
