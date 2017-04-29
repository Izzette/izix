// libk/format/itoa.c

#include <stddef.h>

static const char value_map[] = {
	'0', '1', '2', '3', '4', '5', '6', '7',
	'8', '9', 'a', 'b', 'c', 'd', 'e', 'f'
};

static inline unsigned int log2_floor (unsigned int v) {
	unsigned int x = 0;

	while (v) {
		v >>= 1;
		x += 1;
	}

	return x;
}

#define MKITOA(prefix, type) \
char *u##prefix##toa (unsigned type value, char *result, int base) { \
	unsigned int log2_base = log2_floor ((unsigned int)base); \
\
	char digits[8 * sizeof(unsigned type) / log2_base + 8 * sizeof(unsigned type) % log2_base]; \
\
	char *cur = result; \
\
	if (0 > base || 16 < base) \
		return NULL; \
\
	if (0 == value) { \
		*cur++ = '0'; \
		*cur = '\0'; \
		return result; \
	} \
\
	char *digits_cur = digits; \
\
	while (0 != value) { \
		size_t digit; \
\
		digit = value % base; \
		value /= base; \
\
		*digits_cur++ = value_map[digit]; \
	} \
\
	digits_cur -= 1; \
	cur = result; \
\
	while (digits <= digits_cur) \
		*cur++ = *digits_cur--; \
\
	*cur = '\0'; \
\
	return result; \
} \
\
char *prefix##toa (type value, char *result, int base) { \
	char *cur, *uret; \
\
	cur = result; \
\
	if (0 > value) { \
		*cur++ = '-'; \
		value *= -1; \
	} \
\
	uret = u##prefix##toa (value, cur, base); \
\
	if (NULL == uret) \
		return NULL; \
\
	return result; \
}

MKITOA(ll, long long);

#if __SIZEOF_LONG_LONG__ != __SIZEOF_LONG__
MKITOA(l, long);
#endif

#if __SIZEOF_LONG__ != __SIZEOF_INT__
MKITOA(i, int);
#endif

// vim: set ts=4 sw=4 noet syn=c:
