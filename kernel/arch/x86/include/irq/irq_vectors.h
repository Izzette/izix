// kernel/arch/x86/include/irq/irq_vectors.h

#ifndef IZIX_IRQ_VECTORS_H
#define IZIX_IRQ_VECTORS_H 1

typedef enum irq_vectors_enum {
	IRQ_VECTOR_IRQ0  = 0x20,
	IRQ_VECTOR_IRQ1  = 0x21,
	IRQ_VECTOR_IRQ2  = 0x22,
	IRQ_VECTOR_IRQ3  = 0x23,
	IRQ_VECTOR_IRQ4  = 0x24,
	IRQ_VECTOR_IRQ5  = 0x25,
	IRQ_VECTOR_IRQ6  = 0x26,
	IRQ_VECTOR_IRQ7  = 0x27,
	IRQ_VECTOR_IRQ8  = 0x28,
	IRQ_VECTOR_IRQ9  = 0x29,
	IRQ_VECTOR_IRQ10 = 0x2a,
	IRQ_VECTOR_IRQ11 = 0x2b,
	IRQ_VECTOR_IRQ12 = 0x2c,
	IRQ_VECTOR_IRQ13 = 0x2d,
	IRQ_VECTOR_IRQ14 = 0x2e,
	IRQ_VECTOR_IRQ15 = 0x2f
} irq_vectors_t;

#endif

// vim: set ts=4 sw=4 noet syn=c:
