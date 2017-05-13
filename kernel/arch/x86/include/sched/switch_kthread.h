// kernel/arch/x86/include/sched/switch_kthread.h

#ifndef IZIX_SWITCH_KTHREAD_H
#define IZIX_SWITCH_KTHREAD_H 1

#include <stdint.h>

// Order defined by assembly in switch_kthread.s
typedef volatile struct kthread_registers_struct {
	uint32_t eflags;
	uint32_t edi;
	uint32_t esi;
	uint32_t ebp;
	uint32_t esp;
	uint32_t ebx;
	uint32_t edx;
	uint32_t ecx;
	uint32_t eax;
} kthread_registers_t;

void switch_kthread (
		volatile kthread_registers_t *volatile,
		volatile kthread_registers_t *volatile);

#endif

// vim: set ts=4 sw=4 noet syn=c:
