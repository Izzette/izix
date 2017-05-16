// kernel/arch/x86/crt/crti_zero_bss.s

.file		"crti_zero_bss.s"

.code32

.section	.init

	.type	init_zero_bss,	@function
init_zero_bss:
	push	%ebp
	mov	%esp,		%ebp

// Non SysV function, inlined into init()
	push	%eax
	push	%edi
	pushf

// %edi contains word offset from .bss start.
	xor	%edi,		%edi

// while (...)
	jmp	.Linit_zero_bss_chk_end

.Linit_zero_bss_write:
// %eax is set in .init_zero_bss_chk_end to the byte offset (word-aligned) from .bss start.
	movw	$0,		__gnu_bssstart(%eax)
.Linit_zero_bss_inc:
// Incrment the word offset.
	inc	%edi

.Linit_zero_bss_chk_end:
// Compute byte offset from word offset.
	leal	(,%edi,2),	%eax
// Compare byte offset against the BSS length,
// and write the next word if it is less than.
	cmp	$__gnu_bsslength, %eax
	jl	.Linit_zero_bss_write

.Linit_zero_bss_fin:
	popf
	pop	%edi

	mov	%ebp,		%esp
	pop	%ebp
	.size	init_zero_bss,	.-init_zero_bss

// vim: set ts=8 sw=8 noet syn=asm:
