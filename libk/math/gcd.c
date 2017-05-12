// libk/math/gcd.c

#define SWAP(a, b, t) \
	t = a; \
	a = b; \
	b = t;

#define MKGCD(suffix, type) \
unsigned type gcd##suffix (unsigned type a, unsigned type b) { \
	unsigned char shift; \
\
	if (!b) \
		return a; \
	if (!a) \
		return b; \
\
	for (shift = 0; !((a | b) & 0b1); ++shift) { \
		a >>= 1; \
		b >>= 1; \
	} \
\
	while (!(a & 0b1)) \
		a >>= 1; \
\
	do { \
		while (!(b & 0b1)) \
			b >>= 1; \
\
		if (a > b) { \
			unsigned type t; \
			SWAP(a, b, t) \
		} \
	} while ((b -= a)); \
\
	const unsigned type result = a << shift; \
\
	return result; \
}

MKGCD(ll, long long);

#if __SIZEOF_LONG_LONG__ != __SIZEOF_LONG__
MKGCD(l, long);
#endif

#if __SIZEOF_LONG__ != __SIZEOF_INT__
MKGCD(i, int);
#endif

// vim: set ts=4 sw=4 noet sync=:
