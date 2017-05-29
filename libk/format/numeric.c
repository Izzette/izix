// libk/format/numeric.c

#include <attributes.h>

FASTCALL
unsigned char bcd_to_bin (unsigned char bcd) {
	return ((bcd >> 4) * 10) + (bcd & 0x0f);
}

FASTCALL
unsigned char bin_to_bcd (unsigned char bin) {
	return ((bin / 10) << 4) | (bin % 10);
}

// vim: set ts=4 sw=4 noet syn=c:
