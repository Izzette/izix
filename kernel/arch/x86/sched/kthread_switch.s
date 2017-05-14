// kerenl/arch/x86/asm/kthread_switch.s

.file		"kthread_switch.s"

	.set	sizeof_registers, 36

.code32

	.global	kthread_switch
	.type	kthread_switch,	@function
// void kthread_switch (kthread_registers_t *current, kthread_registers_t *next) {
kthread_switch:
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
	.size	kthread_switch,	.-kthread_switch

// vim: set ts=8 sw=8 noet syn=asm:
