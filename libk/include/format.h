// libk/include/format.h

#ifndef _IZIX_LIBK_FORMAT_H
#define _IZIX_LIBK_FORMAT_H 1

#include <stddef.h>
#include <stdarg.h>

#include <attributes.h>

char *strpadl (char *, char, size_t);
char *strpadr (char *, char, size_t);

char *ulltoa (unsigned long long, char *, int);
char *lltoa (long long, char *, int);

#if __SIZEOF_LONG_LONG__ != __SIZEOF_LONG__
char *ultoa (unsigned long, char *, int);
char *ltoa (long, char *, int);
#else
static inline char *ultoa (unsigned long value, char *result, int base) {
	return ulltoa ((unsigned long long)value, result, base);
}
static inline char *ltoa (long value, char *result, int base) {
	return lltoa ((long long)value, result, base);
}
#endif

#if __SIZEOF_LONG__ != __SIZEOF_INT__
char *uitoa (unsigned int, char *, int);
char *itoa (int, char *, int);
#else
static inline char *uitoa (unsigned int value, char *result, int base) {
	return ultoa ((unsigned long)value, result, base);
}
static inline char *itoa (int value, char *result, int base) {
	return ltoa ((long)value, result, base);
}
#endif

int sprintf (char *, const char *, ...)
	FORMAT(printf, 2, 3);
int vsprintf (char *, const char *, va_list)
	FORMAT(printf, 2, 0);

FASTCALL
unsigned char bcd_to_bin (unsigned char);
FASTCALL
unsigned char bin_to_bcd (unsigned char);

#endif

// vim: set ts=4 sw=4 noet syn=c:
