// kernel/arch/x86/include/asm/freeze.h

#ifndef IZIX_ASM_FREEZE_H
#define IZIX_ASM_FREEZE_H 1

#include <asm/toggle_int.h>
#include <asm/halt.h>

static inline void freeze () {
	disable_int ();

	for (;;)
		halt ();
}

#endif

// vim: set ts=4 sw=4 noet syn=c:
