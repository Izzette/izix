// libk/string/memset.c

#include <stddef.h>

void *memset (void *s, int c, size_t n) {
	const char c_char = c;
	char *s_chars = s;
	size_t i;

	for (i = 0; n > i; ++i)
		s_chars[i] = c_char;

	return s;
}

// vim: set ts=4 sw=4 noet syn=c:
