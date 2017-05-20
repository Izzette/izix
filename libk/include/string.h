// libk/include/string.h

#ifndef _IZIX_LIBK_STRING_H
#define _IZIX_LIBK_STRING_H 1

#include <stddef.h>
#include <strings.h>

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

#define MKFFSX(suffix, parent_suffix, type_prefix, parent_type_prefix) \
static inline int ffs##suffix (type_prefix int i) { \
	int bitpos; \
\
	bitpos = ffs##parent_suffix (i); \
	if (bitpos) \
		return bitpos; \
\
	const parent_type_prefix int high_bits = \
		i >> (8 * sizeof(parent_type_prefix int)); \
\
	bitpos = ffs##parent_suffix (high_bits); \
	if (bitpos) \
		return bitpos + (8 * sizeof(parent_type_prefix int)); \
\
	return 0; \
}

#if __SIZEOF_INT__ != __SIZEOF_LONG__
MKFFSX(l, , long, )
#else
# define ffsl ffs
#endif

#if __SIZEOF_LONG__ != __SIZEOF_LONG_LONG__
MKFFSX(ll, l, long long, long)
#else
# define ffsll ffsl
#endif

#endif

// vim: set ts=4 sw=4 noet syn=c:
