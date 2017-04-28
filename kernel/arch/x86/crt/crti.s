// kernel/arch/x86/crt/crti.s

.file		"crti.s"

.code32

.section	.init

	.globl	_init
	.type	_init,		@function
// void _init () {
_init:
	push	%ebp
	mov	%esp,		%ebp

	// GCC will nicely put the contents of crtbegin.o's .init section here.
// }

.section	.fini

	.globl	_fini
	.type	_fini,		@function
// void _fini () {
_fini:
	push	%ebp
	mov	%esp,		%ebp

	// GCC will nicely put the contents of crtbegin.o's .fini section here.
// }

// vim: set ts=8 sw=8 noet syn=asm:
