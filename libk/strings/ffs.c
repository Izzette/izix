// libk/string/ffs.c

#include <stdint.h>
#include <stddef.h>

// What and why?
#define BITHACK(i) \
	(i & -i)

#define DE_BRUIJN_SEQ 0x077CB531U

static const unsigned char de_bruijn_bitpos[32] = {
	0,  1,  28, 2,  29, 14, 24, 3,
	30, 22, 20, 15, 25, 17, 4,  8,
	31, 27, 13, 23, 21, 19, 16, 7,
	26, 12, 18, 6,  11, 5,  10, 9
};

int ffs (int i) {
	if (0 == i)
		return 0;

	size_t bitpos_index = (DE_BRUIJN_SEQ * BITHACK(i)) >> 27;

	return (int)de_bruijn_bitpos[bitpos_index] + 1;
}

// vim: set ts=4 sw=4 noet syn=c:
