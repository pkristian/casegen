#ifndef CASEGEN_TEMPLATE_H
#define CASEGEN_TEMPLATE_H

#include "input.h"
#include "stringlist.h"

/* Render the assembled input as a template, writing to stdout.

   `lines` is everything the operands produced, template and data together — the first
   casegen:end-template line is the seam, and finding it is this function's job, not the
   caller's. `map` gives diagnostics their file:line. `quiet` suppresses warnings.

   Exits 2 on a malformed template, after printing file:line and the reason. */
void renderTemplate(const StringList *lines, const SourceMap *map, int quiet);

#endif /* CASEGEN_TEMPLATE_H */
