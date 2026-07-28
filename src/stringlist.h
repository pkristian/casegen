#ifndef CASEGEN_STRINGLIST_H
#define CASEGEN_STRINGLIST_H

#include <stddef.h>

/* A growable array of owned strings — used for both input lines and the words a
   line splits into. */
typedef struct
{
    char **item; /* array of owned strings */
    size_t count; /* how many are used     */
    size_t cap; /* how many fit          */
} StringList;

/* Pushes a *copy* of `s`; the list owns it from then on. */
void stringsPush(StringList *list, const char *s);

/* Frees every string and the array, and leaves the list reusable rather than dead. */
void stringsFree(StringList *list);

#endif /* CASEGEN_STRINGLIST_H */
