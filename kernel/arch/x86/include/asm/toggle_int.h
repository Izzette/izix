// kernel/arch/x86/include/asm/toggle_int.h

#ifndef IZIX_ASM_ENABLE_INT_H
#define IZIX_ASM_ENABLE_INT_H 1

static inline void enable_int () {
	asm volatile (
		"		sti;\n");
}

static inline void disable_int () {
	asm volatile (
		"		cli;\n");
}

#endif

// vim: set ts=4 sw=4 noet syn=c:
