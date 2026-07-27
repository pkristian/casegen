#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


typedef struct
{
    char **item; /* array of owned strings */
    size_t count; /* how many are used     */
    size_t cap; /* how many fit          */
} StringList;


static void outOfMemory(void)
{
    fprintf(stderr, "casegen: out of memory\n");
    exit(1);
}


static void stringsPush(StringList *list, const char *s)
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


static void stringsFree(StringList *list)
{
    for (size_t i = 0; i < list->count; i++)
        free(list->item[i]);
    free(list->item);
    list->item = NULL;
    list->count = 0;
    list->cap = 0;
}


static void loadFromStdin(StringList *lineList)
{
    char *lineContent = NULL; /* getline allocates and reallocs this for us */
    size_t cap = 0; /* current capacity of that buffer           */
    ssize_t len; /* length of the line just read, or -1 at EOF */
    long lineNo = 0;

    while ((len = getline(&lineContent, &cap, stdin)) != -1)
    {
        lineNo++;

        /* getline keeps the trailing newline — strip it, and \r for CRLF */
        while (len > 0 && (lineContent[len - 1] == '\n' || lineContent[len - 1] == '\r'))
            lineContent[--len] = '\0';

        stringsPush(lineList, lineContent);
    }

    free(lineContent); /* once, after the loop — not inside it */
    if (ferror(stdin))
    {
        fprintf(stderr, "casegen: read error on stdin: %s\n", strerror(errno));
        exit(1);
    }
}


/* ASCII only, by decision. Hand-rolled rather than <ctype.h> so that bytes >= 0x80
   are unambiguously separators and nothing depends on the locale. */

static int isAsciiUpper(const unsigned char c)
{
    return c >= 'A' && c <= 'Z';
}


static int isAsciiLower(const unsigned char c)
{
    return c >= 'a' && c <= 'z';
}


static int isAsciiDigit(const unsigned char c)
{
    return c >= '0' && c <= '9';
}


static int isWordByte(const unsigned char c)
{
    return isAsciiUpper(c) || isAsciiLower(c) || isAsciiDigit(c);
}


static unsigned char toAsciiLower(const unsigned char c)
{
    return isAsciiUpper(c) ? (unsigned char)(c - 'A' + 'a') : c;
}


/* Does a new word start at `cur`? Digits never trigger a boundary on their own —
   only separators (handled by the caller) and letter case transitions do.

       lower|digit -> Upper     user2|Name, Postgre|SQL
       Upper Upper -> Upper+lower   HTTP|Server, O|Auth2|Token  */
static int isWordBoundary(const unsigned char prev, const unsigned char cur, const unsigned char next)
{
    if (!isAsciiUpper(cur))
        return 0;
    if (!isAsciiUpper(prev))
        return 1;
    return isAsciiLower(next);
}


static void flushWord(char *word, size_t *wordLen, StringList *out)
{
    if (*wordLen == 0)
        return;
    word[*wordLen] = '\0';
    stringsPush(out, word);
    *wordLen = 0;
}


/* Split one line into lowercased words. Pure — no I/O, no formatting.
   A line with no word bytes yields no words at all, which is not an error. */
static void splitWords(const char *line, StringList *out)
{
    const unsigned char *s = (const unsigned char*)line;
    const size_t len = strlen(line);

    char *word = malloc(len + 1); /* no word can be longer than the line */
    if (!word)
        outOfMemory();
    size_t wordLen = 0;

    for (size_t i = 0; i < len; i++)
    {
        const unsigned char c = s[i];

        if (!isWordByte(c)) /* separator: ASCII punctuation, space, and every byte >= 0x80 */
        {
            flushWord(word, &wordLen, out);
            continue;
        }

        /* When wordLen is 0 there is no previous byte to compare against; when it is
           not, s[i - 1] is guaranteed to be a word byte. s[i + 1] is safe to read —
           at the last byte it is the terminating NUL, which is not a lowercase letter. */
        if (wordLen > 0 && isWordBoundary(s[i - 1], c, s[i + 1]))
            flushWord(word, &wordLen, out);

        word[wordLen++] = toAsciiLower(c);
    }

    flushWord(word, &wordLen, out);
    free(word);
}


int main(const int argc, const char *argv[])
{
    StringList lineList = {0};
    loadFromStdin(&lineList);

    /* One input line = one output line, so a line with no words prints blank. */
    for (size_t i = 0; i < lineList.count; i++)
    {
        StringList words = {0};
        splitWords(lineList.item[i], &words);

        for (size_t w = 0; w < words.count; w++)
        {
            if (w > 0)
                putchar(' ');
            fputs(words.item[w], stdout);
        }
        putchar('\n');

        stringsFree(&words);
    }

    stringsFree(&lineList);
    return 0;
}
