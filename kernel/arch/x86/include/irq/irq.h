// kernel/arch/x86/include/irq/irq.h

#ifndef IZIX_IRQ_H
#define IZIX_IRQ_H 1

#include <attributes.h>

#define IRQ_NUMBER_OF_IRQ_LINES 0x10

typedef unsigned char irq_t;
typedef FASTCALL void (*irq_hook_t) (irq_t);

void irq_init ();
// Removing hooks not yet supported.
void irq_add_pre_hook (irq_t, irq_hook_t);
void irq_add_post_hook (irq_t, irq_hook_t);
FASTCALL
void irq_handler (irq_t);

#endif

// vim: set ts=4 sw=4 noet syn=c:
