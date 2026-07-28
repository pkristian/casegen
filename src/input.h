#ifndef CASEGEN_INPUT_H
#define CASEGEN_INPUT_H

#include <stddef.h>

#include "stringlist.h"

/* Operands are concatenated into one list of lines, which loses track of where each
   line came from. A template diagnostic wants to say "columns.tmpl:14", not an offset
   into the concatenation, so record one span per operand and look the answer up. */
typedef struct
{
    const char *path; /* the operand as given; "-" means stdin */
    size_t first; /* index of its first line in the assembled list */
    size_t count;
} SourceSpan;


typedef struct
{
    SourceSpan *item;
    size_t count;
    size_t cap;
} SourceMap;


/* Append one source's lines to `out`, newline (and CR) stripped. A source is a path,
   or "-" for stdin at that position in the operand list. Exits 1 on an I/O error,
   naming the path. `map` may be NULL when the caller does not need line provenance. */
void loadSource(const char *path, StringList *out, SourceMap *map);

/* Assembled line index -> the operand it came from and its line number within it. */
void locateLine(const SourceMap *map, size_t index, const char **path, size_t *lineNo);

void spansFree(SourceMap *map);

#endif /* CASEGEN_INPUT_H */
