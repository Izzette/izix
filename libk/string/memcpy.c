// libk/string/memcpy.c

#include <stdint.h>
#include <stddef.h>

#define MKMEMCPY(name, type) \
static inline void __##name##_memcpy ( \
		type *restrict dest, \
		const type *restrict src, \
		size_t n \
) { \
	while (n--) \
		*dest++ = *src++; \
}

MKMEMCPY(byte, uint8_t);
MKMEMCPY(word, uint16_t);

void *memcpy (void *restrict dest, const void *restrict src, size_t n) {
#pragma GCC diagnostic ignored "-Wpointer-to-int-cast"
	size_t src_as_num = (size_t)src;
#pragma GCC diagnostic pop

	if (0b1 & (src_as_num | n))
		__byte_memcpy ((uint8_t *)dest, (uint8_t *)src, n);
	else
		__word_memcpy ((uint16_t *)dest, (uint16_t *)src, n >> 1);

	return dest;
}

void *memccpy (void *restrict dest, const void *restrict src, int c, size_t n) {
	char *cur_char = dest;
	const char *src_char = src;

	while (n--) {
		if (c == *src_char)
			return cur_char;

		*cur_char++ = *src_char++;
	}

	return NULL;
}

// vim: set ts=4 sw=4 noet syn=c:
