#ifndef CASEGEN_ARGS_H
#define CASEGEN_ARGS_H

#include "cases.h"
#include "stringlist.h"

typedef struct
{
    const CaseSpec *spec; /* NULL means template mode */
    int quiet;
    int stdinOnly; /* -i */
    StringList sources;
} Options;


/* Fill `opt` from argv. Prints the usage block and exits 2 on a usage error, or exits
   0 on --help; either way it does not return in that case. */
void parseArgs(int argc, char *argv[], Options *opt);

/* Check the combination of flags and operands, and expand -i into a "-" operand so
   that everything downstream only ever sees a list of sources. */
void validateArgs(Options *opt);

#endif /* CASEGEN_ARGS_H */
