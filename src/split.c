/* Included by casegen.c — not compiled on its own.
   See the note at the top of casegen.c for why there are no headers.

   Splitting a line into lowercased words. */


/* Does a new word start at `cur`? Digits never trigger a boundary on their own —
   only separators (handled by the caller) and letter case transitions do.

       lower|digit -> Upper     user2|Name, Postgre|SQL
       Upper Upper -> Upper+lower   HTTP|Server, O|Auth2|Token  */
int isWordBoundary(const unsigned char prev, const unsigned char cur, const unsigned char next)
{
    if (!isAsciiUpper(cur))
        return 0;
    if (!isAsciiUpper(prev))
        return 1;
    return isAsciiLower(next);
}


void flushWord(char *word, size_t *wordLen, StringList *out)
{
    if (*wordLen == 0)
        return;
    word[*wordLen] = '\0';
    stringsPush(out, word);
    *wordLen = 0;
}


/* Split one line into lowercased words. Pure — no I/O, no formatting.
   A line with no word bytes yields no words at all, which is not an error. */
void splitWords(const char *line, StringList *out)
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
