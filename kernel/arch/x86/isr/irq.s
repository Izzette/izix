// kernel/arch/x86/isr/irq.s

.include	"save_state.s"

.file		"irq.s"

.macro	isr_irq_head, irq_num
	.globl	isr_irq\irq_num
	.type	isr_irq\irq_num, @function
isr_irq\irq_num:
	save_state

	xor	%ecx,		%ecx
	mov	$\irq_num,	%cl

	jmp isr_irq_handle
	.size	isr_irq\irq_num, .-isr_irq0
.endm

.code32

.section	.text.hot

isr_irq_head 0
isr_irq_head 1
// IRQ2 is used internally by the 8259PIC.
isr_irq_head 3
isr_irq_head 4
isr_irq_head 5
isr_irq_head 6
isr_irq_head 7
isr_irq_head 8
isr_irq_head 9
isr_irq_head 10
isr_irq_head 11
isr_irq_head 12
isr_irq_head 13
isr_irq_head 14
isr_irq_head 15

	.type	isr_irq_handle,	@function
// IRQ number in %al
isr_irq_handle:
	call	irq_handler

	restore_state
	iret
	.size	isr_irq_handle,	.-isr_irq_handle

// vim: set ts=8 sw=8 noet syn=asm:
