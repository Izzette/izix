// libk/include/math.h

#ifndef IZIX_LIBK_MATH_H
#define IZIX_LIBK_MATH_H 1

#include <stdbool.h>

#define MKROUND_INT(suffix, type) \
static inline unsigned type roundu##suffix##_down (unsigned type v, unsigned type by) { \
	return v - v % by; \
} \
\
static inline unsigned type roundu##suffix##_up (unsigned type v, unsigned type by) { \
	if (!(v % by)) \
		return v; \
\
	return by + roundu##suffix##_down (v, by); \
} \
static inline unsigned type round##suffix##_down (type v, unsigned type by) { \
	return v - v % by; \
} \
\
static inline unsigned type round##suffix##_up (type v, unsigned type by) { \
	if (!(v % by)) \
		return v; \
\
	return by + round##suffix##_down (v, by); \
}

MKROUND_INT(ll, long long);
MKROUND_INT(l, long);
MKROUND_INT(i, int);

#define MKABS(suffix, type) \
static inline unsigned type abs##suffix (type v) { \
	if (0 > v) \
		return -1 * v; \
\
	return v; \
}

MKABS(ll, long long);
MKABS(l, long);
MKABS(i, int);

#define MKIS_POWER2(suffix, type) \
static inline bool is_power2u##suffix (unsigned type v) { \
	while (v >> 1) { \
		v >>= 1; \
	} \
\
	return 1 == v; \
} \
\
static inline bool is_power2##suffix (type v) { \
	return is_power2u##suffix (abs##suffix (v)); \
}

MKIS_POWER2(ll, long long);
MKIS_POWER2(l, long);
MKIS_POWER2(i, int);

unsigned long long gcdll (unsigned long long, unsigned long long);

#if __SIZEOF_LONG_LONG__ != __SIZEOF_LONG__
unsigned long gcdl (unsigned long, unsigned long);
#else
static inline unsigned long gcdl (unsigned long a, unsigned long b) {
	return gcdll (a, b);
}
#endif

#if __SIZEOF_LONG__ != __SIZEOF_INT__
unsigned long gcdi (unsigned int, unsigned int);
#else
static inline unsigned int gcdi (unsigned int a, unsigned int b) {
	return gcdl (a, b);
}
#endif

#define MKLCM(suffix, type) \
static inline unsigned type lcmu##suffix (unsigned type a, unsigned type b) { \
	if (!(a | b)) \
		return 0; \
\
	return a * b / gcd##suffix (a, b); \
} \
static inline unsigned type lcm##suffix (type a, type b) { \
	return lcmu##suffix (abs##suffix (a), abs##suffix (b)); \
}

MKLCM(ll, long long);
MKLCM(l, long);
MKLCM(i, int);

#endif

// vim: set ts=4 sw=4 noet syn=c:
