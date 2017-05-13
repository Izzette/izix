// kerenl/arch/x86/asm/switch_kthread.s

.file		"switch_kthread.s"

	.set	sizeof_registers, 36

.code32

	.global	switch_kthread
	.type	switch_kthread,	@function
// void switch_kthread (kthread_registers_t *current, kthread_registers_t *next) {
switch_kthread:
// Save everything.
	pushal
	pushf

// No iteruptions.
	cli

// Keep our base pointer in place.
	mov	%esp,		%ebp

// memcpy (current, <registers pushed to stack>, sizeof(kthread_register_t));
	push	$sizeof_registers
	push	%ebp
	push	0x4+sizeof_registers(%ebp)
	call	memcpy
	add	$0xc,		%esp

// Our next currently running task (next).
	mov	0x8+sizeof_registers(%ebp), %ebx
	mov	%ebx,		kthread_running_task

// Our new stack.
	mov	0x10(%ebx),	%eax
	sub	$sizeof_registers, %eax

// memcpy (<registers pushed to stack>, next, sizeof(kthread_register_t));
	push	$sizeof_registers
	push	%ebx
	push	%eax
	call	memcpy
	add	$0xc,		%esp

// Use our new stack.
	mov	%eax,		%esp

// Return to our last state.
	popf
	popal
// Because we switch our stack, ret will now return to the previous %eip.
	ret
// }
	.size	switch_kthread,	.-switch_kthread

// vim: set ts=8 sw=8 noet syn=asm:
