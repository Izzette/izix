// kernel/arch/x86/include/asm/invlpg.h

#ifndef IZIX_ASM_INVLPG_H
#define IZIX_ASM_INVLPG_H 1

#include <mm/page.h>

static inline void invlpg (page_t *physical_page_offset) {
   asm volatile (
		"invlpg		(%0);\n"
		:
		:"r"(physical_page_offset)
		:"memory");
}

#endif

// vim: set ts=4 sw=4 noet syn=c:
