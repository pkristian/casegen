#ifndef CASEGEN_ASCII_H
#define CASEGEN_ASCII_H

/* ASCII only, by decision. Hand-rolled rather than <ctype.h> so that bytes >= 0x80
   are unambiguously separators and nothing depends on the locale. */

int isAsciiUpper(unsigned char c);
int isAsciiLower(unsigned char c);
int isAsciiDigit(unsigned char c);

/* A word byte is a letter or a digit; everything else separates. */
int isWordByte(unsigned char c);

unsigned char toAsciiLower(unsigned char c);
unsigned char toAsciiUpper(unsigned char c);

#endif /* CASEGEN_ASCII_H */
