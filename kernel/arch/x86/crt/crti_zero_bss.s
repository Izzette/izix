// kernel/arch/x86/crt/crti_zero_bss.s

.file		"crti_zero_bss.s"

.code32

.section	.init

	.type	init_zero_bss,	@function
init_zero_bss:
	push	%ebp
	mov	%esp,		%ebp

// Non SysV function, inlined into init()
	push	%edi
	pushf

	mov	$__gnu_bssstart, %edi

	jmp	.Linit_zero_bss_chk_end

.Linit_zero_bss_write:
	movw	$0,		(%edi)

.Linit_zero_bss_inc:
	add	$2,		%edi

.Linit_zero_bss_chk_end:
	cmp	$__gnu_bssend,	%edi
	jl	.Linit_zero_bss_write

.Linit_zero_bss_fin:
	popf
	pop	%edi

	mov	%ebp,		%esp
	pop	%ebp
	.size	init_zero_bss,	.-init_zero_bss

// vim: set ts=8 sw=8 noet syn=asm:
