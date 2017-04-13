// boot.s

.file		"boot.s"

.code32

.section	.text

	.global	_start
	.type	_start,		@function
// void _start (e820_3x_entry_t *entries, void *entries_end) {
_start:
	push	%ebp
	mov	%esp,		%ebp

// The int 15h eax=e820 memory map array.
	mov	0x0c(%ebp),	%ebx
	mov	0x10(%ebp),	%ecx

// We don't need anything else on this stack every again, reinitialize it.
	mov	$0x8000,	%ebp
	mov	%ebp,		%esp

	call	_init

	push	%ecx
	push	%ebx
	call	kernel_main
	add	$0x8,		%sp

	cli
freeze:
	hlt
	jmp	freeze
// }
	.size	_start,		.-_start

# vim: set ts=8 sw=8 noet syn=asm:
