// kernel/arch/x86/include/asm/invlpg.h

#ifndef IZIX_ASM_INVLPG_H
#define IZIX_ASM_INVLPG_H 1

#include <mm/page.h>

#pragma GCC diagnostic ignored "-Wunused-parameter"
static inline void invlpg (page_t *physical_page_offset) {
#ifdef __i486__
	asm volatile (
		"invlpg		(%0);\n"
		:
		:"r"(physical_page_offset)
		:"memory");
#else
	asm volatile (
		"mov		%%cr3,				%%eax;\n"
		"mov		%%eax,				%%cr3;\n"
		:
		:
		:"memory", "eax");
#endif
}
#pragma GCC diagnostic pop

#endif

// vim: set ts=4 sw=4 noet syn=c:
