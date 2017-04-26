// libk/include/string.h

#ifndef _IZIX_LIBK_STRING_H
#define _IZIX_LIBK_STRING_H 1

#include <stddef.h>

const void *memchr (const void *, int, size_t);

void *memcpy (void *restrict, const void *, size_t);
void *memccpy (void *restrict, const void *, int, size_t);

char *strcat (char *restrict, const char *restrict);
char *strncat (char *restrict, const char *restrict, size_t);

size_t strlen (const char *);
size_t strnlen (const char *, size_t);

#endif

// vim: set ts=4 sw=4 noet syn=c:
