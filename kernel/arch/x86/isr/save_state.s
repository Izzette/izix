// kernel/arch/x86/isr/save_state.s

.file		"save_state.s"

.macro	save_state
	push	%ebp
	mov	%esp,		%ebp
	pusha
	pushf
.endm

.macro	restore_state
	popf
	popa
	mov	%ebp,		%esp
	pop	%ebp
.endm

// vim: set ts=8 sw=8 noet syn=asm:
