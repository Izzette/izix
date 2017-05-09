// kernel/arch/x86/isr/np.s

.file		"np.s"

.code32

.section	.text

	.globl	isr_np
	.type	isr_np,		@function
isr_np:
	// Error code.
	popl	%eax

	push	%ebp
	mov	%esp,		%ebp

	push	%ebx

	mov	%eax,		%ebx

	push	$isr_np_error_msg
	call	kputs
	add	$0x4,		%esp

	push	%ebx
	call	int_print_selector_ec
	add	$0x4,		%esp

	pop	%ebx

	mov	%ebp,		%esp
	pop	%ebp
	iret
	.size	isr_np,		.-isr_np

.section	.rodata

	.type	isr_np_error_msg, @object
isr_np_error_msg:
	.asciz	"isr/np: Caught #NP exception on selector:\n"
	.size	isr_np_error_msg, .-isr_np_error_msg

// vim: set ts=8 sw=8 noet syn=asm:
