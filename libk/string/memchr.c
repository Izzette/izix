// libk/string/memchr.c

#include <stddef.h>

const void *memchr (const void *s, int c, size_t n) {
	const unsigned char *s_u8_ptr = s;
	unsigned char c_u8 = c;

	while (n--) {
		if (c_u8 == *s_u8_ptr)
			return s_u8_ptr;

		++s_u8_ptr;
	}

	return NULL;
}

// vim: set ts=4 sw=4 noet syn=c:
