// kernel/kpanic/kpanic.c

#include <kprint/kprint.h>
#include <asm/freeze.h>

void kpanic () {
	kputs ("Kernel panic!\n");
	freeze ();
}

// vim: set ts=4 sw=4 noet syn=c:
