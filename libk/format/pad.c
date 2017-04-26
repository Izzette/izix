// libk/format/pad.c

#include <stddef.h>
#include <string.h>

static inline char *__strpadl (char *str, char pad, size_t len, size_t cur_len) {
	char *dest, *src, *padend;

	dest = str + len - 1;
	src = str + cur_len - 1;

	while (str <= src)
		*dest-- = *src--;

	dest = str;
	padend = str + len - cur_len;

	while (padend > dest)
		*dest++ = pad;

	return dest + cur_len;
}

static inline char *__strpadr (char *str, char pad, size_t len, size_t cur_len) {
	char *dest, *padend;

	dest = str + cur_len;
	padend = str + len;

	while (padend > dest)
		*dest++ = pad;

	return dest;
}

#define MKPAD(dir) \
char *strpad##dir (char *str, char pad, size_t len) { \
	size_t cur_len; \
	char *dest; \
\
	cur_len = strlen (str); \
\
	if (cur_len >= len) \
		return str; \
\
	dest = __strpad##dir (str, pad, len, cur_len); \
\
	*dest = '\0'; \
\
	return str; \
}

MKPAD(l);
MKPAD(r);

// vim: set ts=4 sw=4 noet syn=c:
