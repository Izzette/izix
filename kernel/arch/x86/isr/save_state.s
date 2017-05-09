// kernel/arch/x86/isr/save_stat.s

.macro	save_state
	push	%eax
	push	%ecx
	push	%edx
.endm

.macro	restore_state
	pop	%edx
	pop	%ecx
	pop	%eax
.endm

// vim: set ts=8 sw=8 noet syn=asm:
