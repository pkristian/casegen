/* Included by casegen.c — not compiled on its own.
   See the note at the top of casegen.c for why there are no headers.

   Reading sources: a path, or "-" for stdin at that position. */


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


void spansPush(SourceMap *map, const char *path, const size_t first, const size_t count)
{
    if (map->count == map->cap)
    {
        const size_t newCap = map->cap ? map->cap * 2 : 8;
        SourceSpan *grown = realloc(map->item, newCap * sizeof *grown);
        if (!grown)
            outOfMemory();
        map->item = grown;
        map->cap = newCap;
    }
    /* path points into argv, which outlives us — no copy needed. */
    map->item[map->count++] = (SourceSpan){path, first, count};
}


void spansFree(SourceMap *map)
{
    free(map->item);
    map->item = NULL;
    map->count = 0;
    map->cap = 0;
}


/* Assembled line index -> the operand it came from and its line number within it.
   Linear, but the list has one entry per operand, so it is a handful at most. */
void locateLine(const SourceMap *map, const size_t index, const char **path, size_t *lineNo)
{
    for (size_t i = 0; i < map->count; i++)
    {
        const SourceSpan *s = &map->item[i];
        if (index >= s->first && index < s->first + s->count)
        {
            *path = strcmp(s->path, "-") == 0 ? "stdin" : s->path;
            *lineNo = index - s->first + 1;
            return;
        }
    }
    *path = "input"; /* unreachable while every line comes from a source */
    *lineNo = index + 1;
}


/* Read whole lines, newline (and CR) stripped, appending to `out`. */
void readLines(FILE *f, StringList *out)
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


/* A source is a path, or "-" for stdin at that position in the operand list.
   `map` may be NULL when the caller does not need line provenance. */
void loadSource(const char *path, StringList *out, SourceMap *map)
{
    const size_t first = out->count;

    if (strcmp(path, "-") == 0)
    {
        readLines(stdin, out);
        if (ferror(stdin))
        {
            fprintf(stderr, "casegen: stdin: %s\n", strerror(errno));
            exit(1);
        }
    }
    else
    {
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

    if (map)
        spansPush(map, path, first, out->count - first);
}
