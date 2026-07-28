#include "ascii.h"


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
