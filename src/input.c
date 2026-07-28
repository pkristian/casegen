/* Reading sources: a path, or "-" for stdin at that position. */

#include "input.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mem.h"


static void spansPush(SourceMap *map, const char *path, const size_t first,
                      const size_t count)
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


/* Linear, but the list has one entry per operand, so it is a handful at most. */
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
static void readLines(FILE *f, StringList *out)
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
