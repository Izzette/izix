// libk/string/strlen.c

#include <stddef.h>
#include <string.h>

size_t strlen (const char *str) {
	size_t len = 0;

	while (str[len])
		len++;

	return len;
}

size_t strnlen (const char *str, size_t n) {
	void *memchr_result;
	size_t len;

	memchr_result = memchr (str, '\0', n);

	if (!memchr_result)
		len = n;
	else
		len = memchr_result - (void *)str;

	return len;
}

// vim: set ts=4 sw=4 noet syn=c:
