// kernel/arch/x86/irq/irq.c

#include <asm/io.h>
#include <kprint/kprint.h>
#include <irq/irq.h>
#include <pic_8259/pic_8259.h>

void irq_handler (irq_t irq) {
	// Do nothing, for now ...
	pic_8259_send_eoi (irq);
}

// vim: set ts=4 sw=4 noet syn=c:
