// kernel/arch/x86/include/asm/freeze.h

#ifndef IZIX_ASM_FREEZE_H
#define IZIX_ASM_FREEZE_H 1

static inline void freeze () {
	asm volatile (
		"		cli;\n"
		".Lfreeze_freeze:\n"
		"		hlt;\n"
		"		jmp		.Lfreeze_freeze;\n");
}

#endif

// vim: set ts=4 sw=4 noet syn=c:
