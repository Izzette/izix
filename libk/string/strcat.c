// strcat.c

#include <string.h>

#define STR_SEEK_END(str) \
	str += strlen (str)

char *strcat (char *restrict dest, const char *restrict src) {
	char *cur = dest;

	STR_SEEK_END(cur);

	while (*src)
		*cur++ = *src++;

	*cur = '\0';

	return dest;
}

char *strncat (char *restrict dest, const char *restrict src, size_t n) {
	char *cur = dest;

	STR_SEEK_END(cur);

	memccpy (cur, src, '\0', n);

	return dest;
}

// vim: set ts=4 sw=4 noet syn=c:
