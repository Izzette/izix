// libk/include/format.h

#ifndef _IZIX_LIBK_FORMAT_H
#define _IZIX_LIBK_FORMAT_H 1

#include <stddef.h>

char *strpadl (char *, char, size_t);
char *strpadr (char *, char, size_t);

char *ulltoa (unsigned long long, char *, int);
char *lltoa (long long, char *, int);
char *ultoa (unsigned long, char *, int);
char *ltoa (long, char *, int);
char *uitoa (unsigned int, char *, int);
char *itoa (int, char *, int);

#endif

// vim: set ts=4 sw=4 noet syn=c:
