// kernel/arch/x86/drivers/pic_8259/pic_8259.c

#include <asm/io.h>
#include <int/idt.h>
#include <irq/irq.h>
#include <irq/irq_vectors.h>

#define PIC_MASTER_CMD  0x0020
#define PIC_MASTER_DATA 0x0021
#define PIC_SLAVE_CMD   0x00a0
#define PIC_SLAVE_DATA  0x00a1

#define PIC_MASTER_VECTOR IRQ_VECTOR_IRQ0
#define PIC_SLAVE_VECTOR  IRQ_VECTOR_IRQ8

#define PIC_EOI 0x20

// TODO: what are this, where do they come from?
#define ICW1_ICW4      0x01 // ICW4 (not) needed
#define ICW1_SINGLE    0x02 // Single (cascade) mode
#define ICW1_INTERVAL4 0x04 // Call address interval 4 (8)
#define ICW1_LEVEL     0x08 // Level triggered (edge) mode
#define ICW1_INIT      0x10 // Initialization - required!

#define ICW4_8086       0x01 // 8086/88 (MCS-80/85) mode
#define ICW4_AUTO       0x02 // Auto (normal) EOI
#define ICW4_BUF_SLAVE  0x08 // Buffered mode/slave
#define ICW4_BUF_MASTER 0x0C // Buffered mode/master
#define ICW4_SFNM       0x10 // Special fully nested (not)

void pic_8259_send_eoi (irq_t irq) {
	if(irq >= 8)
		outb (PIC_EOI, PIC_SLAVE_CMD);

	outb (PIC_EOI, PIC_MASTER_CMD);
}

void pic_8259_reinit () {
	unsigned char mask_master, mask_slave;

	mask_master = inb (PIC_MASTER_DATA);
	mask_slave  = inb (PIC_SLAVE_DATA);

	// Starts the initialization sequence (in cascade mode).
	outb (ICW1_INIT + ICW1_ICW4, PIC_MASTER_CMD);
	io_wait ();
	outb (ICW1_INIT + ICW1_ICW4, PIC_SLAVE_CMD);
	io_wait ();

	// ICW2: Master PIC vector offset
	outb (PIC_MASTER_VECTOR, PIC_MASTER_DATA);
	io_wait ();

	// ICW2: Slave PIC vector offset
	outb (PIC_SLAVE_VECTOR, PIC_SLAVE_DATA);
	io_wait ();

	// ICW3: tell Master PIC that there is a slave PIC at IRQ2 (0000 0100)
	outb (4, PIC_MASTER_DATA);
	io_wait ();

	// ICW3: tell Slave PIC its cascade identity (0000 0010)
	outb (2, PIC_SLAVE_DATA);
	io_wait ();

	outb (ICW4_8086, PIC_MASTER_DATA);
	io_wait ();
	outb (ICW4_8086, PIC_SLAVE_DATA);
	io_wait ();

	// Restore saved masks.
	outb (mask_master, PIC_MASTER_DATA);
	outb (mask_slave, PIC_SLAVE_DATA);
}

// vim: set ts=4 sw=4 noet syn=c:
