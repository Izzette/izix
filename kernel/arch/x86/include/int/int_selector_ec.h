// kernel/arch/x86/include/int/int_selector_ec.h

#ifndef IZIX_INT_SELECTOR_EC_H
#define IZIX_INT_SELECTOR_EC_H 1

#include <mm/gdt.h>

typedef enum int_selector_table_enum {
	IDT_SELECTOR_TABLE_GDT  = 0b00,
	IDT_SELECTOR_TABLE_IDT1 = 0b01, // IDT could be either *_IDT1 or *_IDT2.
	IDT_SELECTOR_TABLE_LDT  = 0b10,
	IDT_SELECTOR_TABLE_IDT2 = 0b11
} int_selector_table_t;

typedef struct __attribute__((packed)) int_selector_ec_struct {
	unsigned char external : 1;
	int_selector_table_t table : 2;
	segment_selector_t selector : 13;
	uint16_t _rsv;
} int_selector_ec_t;

typedef struct int_selector_ec_logical_struct {
	bool external;
	int_selector_table_t table : 2;
	segment_selector_t selector;
} int_selector_ec_logical_t;

static inline char *int_get_selector_table_name (int_selector_table_t table) {
	if (IDT_SELECTOR_TABLE_GDT == table)
		return "GDT";
	if (IDT_SELECTOR_TABLE_LDT == table)
		return "LDT";

	return "IDT";
}

static inline int_selector_ec_t int_selector_ec_encode (
		int_selector_ec_logical_t logical_selector_ec
) {
	int_selector_ec_t selector_ec = {
		.external = logical_selector_ec.external ? 0b1 : 0b0,
		.table = logical_selector_ec.table,
		.selector = logical_selector_ec.selector,
		._rsv = 0
	};

	return selector_ec;
}

static inline int_selector_ec_logical_t int_selector_ec_decode (
		int_selector_ec_t selector_ec
) {
	int_selector_ec_logical_t logical_selector_ec = {
		.external = selector_ec.external ? true : false,
		.table = selector_ec.table,
		.selector = selector_ec.selector
	};

	return logical_selector_ec;
}

void int_print_selector_ec (int_selector_ec_t);

#endif

// vim: set ts=4 sw=4 noet syn=c:
