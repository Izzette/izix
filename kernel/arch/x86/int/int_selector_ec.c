// kernel/arch/x86/int/int_selector_ec.c

#include <kprint/kprint.h>
#include <int/int_selector_ec.h>

void int_print_selector_ec (int_selector_ec_t selector_ec) {
	int_selector_ec_logical_t logical_selector_ec = int_selector_ec_decode (selector_ec);

	kprintf (
		"int/int_selector_ec: %s %s selector error occurred for selector=0x%04hx.\n",
		logical_selector_ec.external ? "External" : "Internal",
		int_get_selector_table_name (logical_selector_ec.table),
		logical_selector_ec.selector);
}

// vim: set ts=4 sw=4 noet syn=c:
