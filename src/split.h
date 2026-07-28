#ifndef CASEGEN_SPLIT_H
#define CASEGEN_SPLIT_H

#include "stringlist.h"

/* Split one line into lowercased words, appending them to `out`. Pure — no I/O, no
   formatting. A line with no word bytes yields no words at all, which is not an error.

   Boundaries come from separators and *letter* case transitions only; a digit never
   starts a word on its own, so sha256 and int32 stay whole. See TODO.md for why. */
void splitWords(const char *line, StringList *out);

#endif /* CASEGEN_SPLIT_H */
