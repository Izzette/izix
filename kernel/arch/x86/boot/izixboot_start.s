// boot.s

.file		"boot.s"

.code32

.section	.text

	.global	_start
	.type	_start,		@function
// void _start (e820_3x_entry_t *entries, void *entries_end) {
_start:
// We won't be needing or be able to save anything on the stack.
	mov	$0x0,		%ebp
//	push	%ebp
//	mov	%esp,		%ebp

// We won't be needing or be able to save %ebx either.
//	push	%ebx

// The int 15h eax=e820 memory map array.
	mov	0x08(%esp),	%ecx
	mov	0x0c(%esp),	%ebx
	mov	0x10(%esp),	%edx

// We don't need anything else on this stack every again, reinitialize it.
	mov	$0x8000,	%esp

	push	%ecx
	push	%edx
	call	_init
	pop	%edx
	pop	%ecx

	push	%edx
	push	%ecx
	push	%ebx
	call	kernel_main
	add	$0x8,		%sp

	call	_fini

	cli
freeze:
	hlt
	jmp	freeze
// }
	.size	_start,		.-_start

# vim: set ts=8 sw=8 noet syn=asm:
