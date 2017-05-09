// kernel/arch/x86/isr/df.s

.file		"df.s"

.code32

.section	.text

	.globl	isr_df
	.type	isr_df,		@function
isr_df:
	push	%ebp
	mov	%esp,		%ebp

	push	$isr_df_msg
	call	kputs
	add	$0x4,		%esp

	cli
.Lisr_df_freeze:
	hlt
	jmp	.Lisr_df_freeze

	.size	isr_df,		.-isr_df

.section	.rodata

	.type	isr_df_msg,	@object
isr_df_msg:
	.asciz	"Caught double fault!\nBailing ..."
	.size	isr_df_msg,	.-isr_df_msg

// vim: set ts=8 sw=8 noet syn=asm:
