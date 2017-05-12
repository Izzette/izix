// kernel/boot/main_loop.c

#if defined(ARCH_X86)
# include <asm/toggle_int.h>
# include <asm/halt.h>
#endif

void main_loop () {
#if defined(ARCH_X86)
	enable_int ();
#endif

	for (;;) {
#if defined(ARCH_X86)
		halt ();
#endif
	}
}

// vim: set ts=4 sw=4 noet syn=c:
