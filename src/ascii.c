/* Included by casegen.c — not compiled on its own.
   See the note at the top of casegen.c for why there are no headers. */


/* ASCII only, by decision. Hand-rolled rather than <ctype.h> so that bytes >= 0x80
   are unambiguously separators and nothing depends on the locale. */

int isAsciiUpper(const unsigned char c)
{
    return c >= 'A' && c <= 'Z';
}


int isAsciiLower(const unsigned char c)
{
    return c >= 'a' && c <= 'z';
}


int isAsciiDigit(const unsigned char c)
{
    return c >= '0' && c <= '9';
}


int isWordByte(const unsigned char c)
{
    return isAsciiUpper(c) || isAsciiLower(c) || isAsciiDigit(c);
}


unsigned char toAsciiLower(const unsigned char c)
{
    return isAsciiUpper(c) ? (unsigned char)(c - 'A' + 'a') : c;
}


unsigned char toAsciiUpper(const unsigned char c)
{
    return isAsciiLower(c) ? (unsigned char)(c - 'a' + 'A') : c;
}
