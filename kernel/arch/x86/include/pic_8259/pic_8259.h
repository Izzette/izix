// kernel/arch/x86/include/pic_8259/pic_8259.h

#ifndef IZIX_PIC_8259_H
#define IZIX_PIC_8259_H 1

#include <irq/irq.h>
#include <dev/dev_driver.h>

void pic_8259_mask (irq_t);
void pic_8259_unmask (irq_t);
bool pic_8259_is_masked (irq_t);
void pic_8259_send_eoi (irq_t);
void pic_8259_reinit ();
dev_driver_t *pic_8259_get_device_driver ();

#endif

// vim: set ts=4 sw=4 noet syn=c:
