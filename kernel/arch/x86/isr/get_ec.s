// kernel/arch/x86/isr/save_stat.s

.file		"get_ec.s"

.macro	get_ec dest scratch
	mov	0x4(%ebp),	\dest
// Replace error code with the old base pointer.
	mov	(%ebp),		\scratch
	add	$0x4,		%ebp
	mov	\scratch,	(%ebp)
.endm

// vim: set ts=8 sw=8 noet syn=asm:
