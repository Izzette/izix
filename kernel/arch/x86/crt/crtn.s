// kernel/arch/x86/crt/crtn.s

.file		"crtn.s"

.code32

.section	.init

// void _init () {
	// GCC will nicely put the contents of crtend.o's .init section here.

	pop	%ebp
	ret
// }

.section	.fini

// void _fini () {
	// GCC will nicely put the contents of crtend.o's .fini section here.

	pop	%ebp
	ret
// }

// vim: set ts=8 sw=8 noet syn=asm:
