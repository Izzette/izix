// kernel/arch/x86/isr/gp.s

.file		"gp.s"

.code32

.section	.text

	.globl	isr_gp
	.type	isr_gp,		@function
isr_gp:
	// Error code.
	popl	%eax

	push	%ebp
	mov	%esp,		%ebp

	push	%ebx

	mov	%eax,		%ebx

	test	%ebx,		%ebx

	jz	.Lisr_gp_other_error

.Lisr_gp_segment_error:
	push	$isr_gp_segment_error_msg
	call	kputs
	add	$0x04,		%esp

	push	%ebx
	call	int_print_selector_ec
	add	$0x4,		%esp

	jmp	.Lisr_gp_fin

.Lisr_gp_other_error:
	push	$isr_gp_other_error_msg
	call	kputs
	add	$0x08,		%esp

.Lisr_gp_fin:
	pop	%ebx

	mov	%ebp,		%esp
	pop	%ebp
	iret
	.size	isr_gp,		.-isr_gp

.section	.rodata

	.type	isr_gp_segment_error_msg, @object
isr_gp_segment_error_msg:
	.asciz	"isr/gp: Caught #GP segment error on selector:\n"
	.size	isr_gp_segment_error_msg, .-isr_gp_segment_error_msg

	.type	isr_gp_other_error_msg, @object
isr_gp_other_error_msg:
	.asciz	"isr/gp: Caught unknown #GP exception!\n"
	.size	isr_gp_other_error_msg, .-isr_gp_other_error_msg

// vim: set ts=8 sw=8 noet syn=asm:
