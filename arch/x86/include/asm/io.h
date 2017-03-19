// arch/x86/include/asm/io.h

// Shamelessly copied straight from <linux-4.10/arch/x86/include/asm/io.h>.

/**
 * Definitions for the x86 IO instructions inb/inw/inl/outb/outw/outl.
 * Copyright (C) 2017-ish  Linus Torvalds and the Linux Authors
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

/**
 * Thanks to James van Artsdalen for a better timing-fix than
 * the two short jumps: using outb's to a nonexistent port seems
 * to guarantee better timings even on fast machines.
 *
 * On the other hand, I'd like to be sure of a non-existent port:
 * I feel a bit unsafe about using 0x80 (should be safe, though)
 *
 *		Linus
 */

static inline void native_io_delay () {
	asm volatile ("outb %al, $0x80");
}

static inline void slow_down_io () {
	native_io_delay();
#ifdef REALLY_SLOW_IO
	native_io_delay();
	native_io_delay();
	native_io_delay();
#endif
}

#define BUILDIO(bwl, bw, type)						\
static inline void out##bwl (unsigned type value, int port)	{ \
	asm volatile ( \
			"out" #bwl " %" #bw "0, %w1" \
			: : "a"(value), "Nd"(port)); \
} \
\
static inline unsigned type in##bwl (int port) { \
	unsigned type value; \
	asm volatile ( \
			"in" #bwl " %w1, %" #bw "0" \
			: "=a"(value) : "Nd"(port)); \
	return value; \
} \
\
static inline void out##bwl##_p (unsigned type value, int port) { \
	out##bwl (value, port); \
	slow_down_io (); \
} \
\
static inline unsigned type in##bwl##_p (int port) { \
	unsigned type value = in##bwl (port); \
	slow_down_io (); \
	return value; \
} \
\
static inline void outs##bwl (int port, const void *addr, unsigned long count) { \
	asm volatile ( \
			"rep; outs" #bwl \
			: "+S"(addr), "+c"(count) : "d"(port)); \
} \
\
static inline void ins##bwl (int port, void *addr, unsigned long count) { \
	asm volatile ( \
			"rep; ins" #bwl \
			: "+D"(addr), "+c"(count) : "d"(port)); \
}

BUILDIO(b, b, char)
BUILDIO(w, w, short)
BUILDIO(l, , int)

// vim: set ts=4 sw=4 noet syn=c:
