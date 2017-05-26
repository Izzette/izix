// kernel/arch/x86/include/int/idt.h

#ifndef IZIX_IDT_H
#define IZIX_IDT_H 1

#include <stddef.h>
#include <stdint.h>

#include <attributes.h>

#include <mm/gdt.h>

#define IDT_OFFSET_LOW_OFFSET  000
#define IDT_OFFSET_HIGH_OFFSET 020

#define IDT_OFFSET_LENGTH 040
#define IDT_OFFSET_LOW_LENGTH \
	(IDT_OFFSET_HIGH_OFFSET - IDT_OFFSET_LOW_OFFSET)
#define IDT_OFFSET_HIGH_LENGTH \
	(IDT_OFFSET_LENGTH      - IDT_OFFSET_HIGH_OFFSET)

#define IDT_OFFSET_LOW_MASK \
	(((0x00000001UL << IDT_OFFSET_LOW_LENGTH)  - 1) << IDT_OFFSET_LOW_OFFSET)
#define IDT_OFFSET_HIGH_MASK \
	(((0x00000001UL << IDT_OFFSET_HIGH_LENGTH) - 1) << IDT_OFFSET_HIGH_OFFSET)

#define IDT_NUMBER_OF_INTERUPTS 256

#define IDT_DE_VECTOR  0x00
#define IDT_DB_VECTOR  0x01
#define IDT_NMI_VECTOR 0x02
#define IDT_BP_VECTOR  0x03
#define IDT_OF_VECTOR  0x04
#define IDT_BR_VECTOR  0x05
#define IDT_UD_VECTOR  0x06
#define IDT_NM_VECTOR  0x07
#define IDT_DF_VECTOR  0x08
// Co-proccessor segment overrun is unused, formally 0x09
#define IDT_TS_VECTOR  0x0a
#define IDT_NP_VECTOR  0x0b
#define IDT_SS_VECTOR  0x0c
#define IDT_GP_VECTOR  0x0d
#define IDT_PF_VECTOR  0x0e
// 0x0f is reserved
#define IDT_MF_VECTOR  0x10
#define IDT_AC_VECTOR  0x11
#define IDT_MC_VECTOR  0x12
#define IDT_XM_VECTOR  0x13
#define IDT_VE_VECTOR  0x14
// 0x15-0x1d are reserved
#define IDT_SX_VECTOR  0x1e
// 0x1f is reserved

typedef uint8_t interupt_vector_t;

typedef enum idt_type_enum {
	IDT_I386_TASK_GATE     = 0b0101,
	IDT_I286_INTERUPT_GATE = 0b0110,
	IDT_I286_TRAP_GATE     = 0b0111,
	IDT_I386_INTERUPT_GATE = 0b1110,
	IDT_I386_TRAP_GATE     = 0b1111,
} idt_type_t;

typedef struct PACKED idt_entry_struct {
	uint16_t offset_low;
	uint16_t selector;
	uint8_t _rsv;
	idt_type_t type : 4;
	unsigned char storage : 1;
	unsigned char priviledge : 2;
	unsigned char present : 1;
	uint16_t offset_high;
} idt_entry_t;

typedef struct idt_entry_logical_struct {
	void (*isr_offset) ();
	segment_selector_t selector;
	idt_type_t type;
	bool storage;
	unsigned char priviledge : 2;
	bool present;
} idt_entry_logical_t;

typedef struct PACKED idt_register_struct {
	uint16_t limit;
	uint32_t base;
} idt_register_t;

typedef struct idt_register_logical_struct {
	size_t limit : 16;
	idt_entry_t *base;
} idt_register_logical_t;

static inline uint16_t idt_get_offset_low (void (*isr_offset) ()) {
#pragma GCC diagnostic ignored "-Wpointer-to-int-cast"
	return (IDT_OFFSET_LOW_MASK & (size_t)isr_offset)  >> IDT_OFFSET_LOW_OFFSET;
#pragma GCC diagnostic pop
}

static inline uint16_t idt_get_offset_high (void (*isr_offset) ()) {
#pragma GCC diagnostic ignored "-Wpointer-to-int-cast"
	return (IDT_OFFSET_HIGH_MASK & (size_t)isr_offset) >> IDT_OFFSET_HIGH_OFFSET;
#pragma GCC diagnostic pop
}

static inline void (*idt_get_offset (uint16_t offset_low, uint16_t offset_high)) () {
#pragma GCC diagnostic ignored "-Wint-to-pointer-cast"
	return (void (*) ())(
			(offset_low << IDT_OFFSET_LOW_OFFSET) |
			(offset_high << IDT_OFFSET_HIGH_OFFSET));
#pragma GCC diagnostic pop
}
static inline idt_entry_t idt_entry_encode (idt_entry_logical_t logical_entry) {
	idt_entry_t entry = {
		.offset_low = idt_get_offset_low (logical_entry.isr_offset),
		.selector = logical_entry.selector,
		._rsv = 0,
		.type = logical_entry.type,
		.storage = logical_entry.storage ? 0b1 : 0b0,
		.priviledge = logical_entry.priviledge,
		.present = logical_entry.present ? 0b1 : 0b0,
		.offset_high = idt_get_offset_high (logical_entry.isr_offset)
	};

	return entry;
}

static inline idt_entry_logical_t idt_entry_decode (idt_entry_t entry) {
	idt_entry_logical_t logical_entry = {
		.isr_offset = idt_get_offset (entry.offset_low, entry.offset_high),
		.selector = entry.selector,
		.type = entry.type,
		.storage = entry.storage ? true : false,
		.priviledge = entry.priviledge,
		.present = entry.present ? true : false,
	};

	return logical_entry;
}

static inline idt_register_t idt_register_encode (
		idt_register_logical_t logical_registry
) {
#pragma GCC diagnostic ignored "-Wpointer-to-int-cast"
	idt_register_t registry = {
		.limit = sizeof(idt_entry_t) * logical_registry.limit - 1,
		.base = (uint32_t)logical_registry.base
	};
#pragma GCC diagnostic pop

	return registry;
}

static inline idt_register_logical_t idt_register_decode (
		idt_register_t registry
) {
#pragma GCC diagnostic ignored "-Wint-to-pointer-cast"
	idt_register_logical_t logical_registry = {
		.limit = (registry.limit + 1) / sizeof(idt_entry_t),
		.base = (idt_entry_t *)registry.base
	};
#pragma GCC diagnostic pop

	return logical_registry;
}

void idt_init ();
void idt_set_isr (interupt_vector_t, void (*) ());
void idt_load ();

#endif

// vim: set ts=4 sw=4 noet syn=c:
