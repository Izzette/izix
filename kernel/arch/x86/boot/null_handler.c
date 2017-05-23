// kernel/arch/x86/boot/null_handler.c

#include <int/idt.h>

// Cause #DF on jump/call to NULL.
__attribute__((constructor))
void null_handler_construct () {
	asm volatile (
		"		jmp		.Lcreate_null_call_handler_copy;\n"
		".Lnpx:\n" // NULL pointer exception.
		"		int		%0;\n"
		".Lcreate_null_call_handler_copy:\n"
		"		mov		.Lnpx,		%%ax;\n"
		"		mov		%%ax,		0x0;\n"
		:
		:"i"(IDT_DF_VECTOR)
		:"memory","ax");
}

// vim: set ts=4 sw=4 noet syn=c:
