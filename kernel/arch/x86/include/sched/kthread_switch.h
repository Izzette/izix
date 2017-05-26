// kernel/arch/x86/include/sched/kthread_switch.h

#ifndef IZIX_SWITCH_KTHREAD_H
#define IZIX_SWITCH_KTHREAD_H 1

#include <stdint.h>

#include <attributes.h>

typedef struct PACKED kthread_eflags_struct {
	unsigned char cflg  : 1;
	unsigned char _rsv0 : 1; // always 1
	unsigned char pflg  : 1;
	unsigned char _rsv1 : 1;
	unsigned char aflg  : 1;
	unsigned char _rsv2 : 1;
	unsigned char zflg  : 1;
	unsigned char sflg  : 1;
	unsigned char tflg  : 1;
	unsigned char iflg  : 1;
	unsigned char dflg  : 1;
	unsigned char iopl  : 2;
	unsigned char nt    : 1;
	unsigned char _rsv3 : 1; // always 0
	unsigned char rflg  : 1;
	unsigned char vm    : 1;
	unsigned char ac    : 1;
	unsigned char viflg : 1;
	unsigned char vip   : 1;
	unsigned char id    : 1;
	unsigned short _rsv4 : 10;
} kthread_eflags_t;

// Order defined by assembly in kthread_switch.s
typedef struct kthread_registers_struct {
	kthread_eflags_t eflags;
	uint32_t edi;
	uint32_t esi;
	void *ebp;
	void *esp;
	uint32_t ebx;
	uint32_t edx;
	uint32_t ecx;
	uint32_t eax;
} kthread_registers_t;

static inline void kthread_registers_pushw (
		kthread_registers_t *registers,
		uint16_t w
) {
	//    pushw   <w>
	registers->esp -= sizeof(uint16_t);
	*(uint16_t *)registers->esp = w;
}

static inline void kthread_registers_pushl (
		kthread_registers_t *registers,
		uint32_t dw
) {
	//    pushl   <dw>
	registers->esp -= sizeof(uint32_t);
	*(uint32_t *)registers->esp = dw;
}

static inline void kthread_registers_pushq (
		kthread_registers_t *registers,
		uint32_t qw
) {
	//    pushq   <qw>
	registers->esp -= sizeof(uint64_t);
	*(uint64_t *)registers->esp = qw;
}

static inline void kthread_registers_push_caller (
		kthread_registers_t *registers,
		void *caller
) {
#pragma GCC diagnostic ignored "-Wpointer-to-int-cast"
	// caller:
	//    call    <...>
	kthread_registers_pushl (registers, (uint32_t)caller);
#pragma GCC diagnostic pop
}

static inline void kthread_registers_push_frame (
		kthread_registers_t *registers
) {
#pragma GCC diagnostic ignored "-Wpointer-to-int-cast"
	//    push    %ebp
	kthread_registers_pushl (registers, (uint32_t)registers->ebp);
#pragma GCC diagnostic pop

	//    mov     %esp,    %ebp
	registers->ebp = registers->esp;
}

static inline kthread_registers_t new_kthread_registers (void *stack_bottom) {
	kthread_registers_t registers = {
		.eflags = {
			.cflg  = 0,
			._rsv0 = 1, // always 1
			.pflg  = 0,
			._rsv1 = 0,
			.aflg  = 0,
			._rsv2 = 0,
			.zflg  = 0,
			.sflg  = 0,
			.tflg  = 0,
			.iflg  = 1, // enable interupts
			.dflg  = 0,
			.iopl  = 0, // ring 0 IO perms
			.nt    = 0,
			._rsv3 = 0, // always 0
			.rflg  = 0,
			.vm    = 0,
			.ac    = 0,
			.viflg = 0,
			.vip   = 0,
			.id    = 0,
			._rsv4 = 0
		},
		.edi = 0,
		.esi = 0,
		.esp = stack_bottom,
		.ebp = 0,
		.ebx = 0,
		.edx = 0,
		.ecx = 0,
		.eax = 0
	};

	return registers;
}

void kthread_switch (
		volatile kthread_registers_t *volatile,
		volatile kthread_registers_t *volatile);

#endif

// vim: set ts=4 sw=4 noet syn=c:
