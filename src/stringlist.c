/* Included by casegen.c — not compiled on its own.
   See the note at the top of casegen.c for why there are no headers.

   A growable array of owned strings — used for both input lines
   and the words a line splits into. */


typedef struct
{
    char **item; /* array of owned strings */
    size_t count; /* how many are used     */
    size_t cap; /* how many fit          */
} StringList;


void outOfMemory(void)
{
    fprintf(stderr, "casegen: out of memory\n");
    exit(1);
}


void stringsPush(StringList *list, const char *s)
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


void stringsFree(StringList *list)
{
    for (size_t i = 0; i < list->count; i++)
        free(list->item[i]);
    free(list->item);
    list->item = NULL;
    list->count = 0;
    list->cap = 0;
}
