// libk/include/string.h

#ifndef _IZIX_LIBK_STRING_H
#define _IZIX_LIBK_STRING_H 1

#include <stddef.h>

const void *memchr (const void *, int, size_t);

void *memcpy (void *restrict, const void *, size_t);
void *memccpy (void *restrict, const void *, int, size_t);

void *memset (void *s, int c, size_t n);

char *strcat (char *restrict, const char *restrict);
char *strncat (char *restrict, const char *restrict, size_t);

size_t strlen (const char *);
size_t strnlen (const char *, size_t);

static inline char *strcpy (char *dest, const char *str) {
	*dest = '\0';
	strcat (dest, str);

	return dest;
}
static inline char *strncpy (char *dest, const char *str, size_t n) {
	*dest = '\0';
	strncat (dest, str, n);

	return dest;
}

#endif

// vim: set ts=4 sw=4 noet syn=c:
