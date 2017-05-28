// kernel/arch/x86/include/asm/toggle_int.h

#ifndef IZIX_ASM_ENABLE_INT_H
#define IZIX_ASM_ENABLE_INT_H 1

#include <stdint.h>
#include <stdbool.h>

// Has a definition of eflags
#include <sched/kthread_switch.h>

static inline void enable_int () {
	asm volatile (
		"		sti;\n");
}

static inline void disable_int () {
	asm volatile (
		"		cli;\n");
}

static inline bool int_is_enabled () {
	uint32_t eflags_register;
	asm (
		"		pushf;\n"
		"		movl	(%%esp),		%0;\n"
		"		add		$0x4,			%%esp;\n"
		:"=r"(eflags_register));

	kthread_eflags_t eflags = *(kthread_eflags_t *)&eflags_register;

	return 0b1 == eflags.iflg;
}

#endif

// vim: set ts=4 sw=4 noet syn=c:
