// kernel/arch/x86/include/asm/halt.h

#ifndef IZIX_ASM_HALT_H
#define IZIX_ASM_HALT_H 1

static inline void halt () {
	asm volatile (
		"		hlt;\n");
}

#endif

// vim: set ts=4 sw=4 noet syn=c:
