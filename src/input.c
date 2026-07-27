/* Included by casegen.c — not compiled on its own.
   See the note at the top of casegen.c for why there are no headers.

   Reading sources: a path, or "-" for stdin at that position. */


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


/* A source is a path, or "-" for stdin at that position in the operand list. */
void loadSource(const char *path, StringList *out)
{
    if (strcmp(path, "-") == 0)
    {
        readLines(stdin, out);
        if (ferror(stdin))
        {
            fprintf(stderr, "casegen: stdin: %s\n", strerror(errno));
            exit(1);
        }
        return;
    }

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
