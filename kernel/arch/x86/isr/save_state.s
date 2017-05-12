// kernel/arch/x86/isr/save_state.s

.file		"save_state.s"

.macro	save_state
	pushf
	push	%eax
	push	%ebx
	push	%ecx
	push	%edx
	push	%esi
	push	%edi
.endm

.macro	restore_state
	pop	%edi
	pop	%esi
	pop	%edx
	pop	%ecx
	pop	%ebx
	pop	%eax
	popf
.endm

// vim: set ts=8 sw=8 noet syn=asm:
