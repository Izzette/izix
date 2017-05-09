// kernel/arch/x86/mm/gdt.h

#ifndef IZIX_GDT_H
#define IZIX_GDT_H 1

#include <stdint.h>
#include <stdbool.h>

#include <izixboot/gdt.h>

// Must be defined for tss.h
typedef uint16_t segment_selector_t;

#include <sched/tss.h>

#define GDT_SELECTOR_INC 0x08

static inline bool gdt_is_valid_selector (segment_selector_t selector) {
	return 0 == selector % GDT_SELECTOR_INC;
}

void gdt_register (gdt_register_t *);
void gdt_dump_entries ();

// Returns 0x01 as invalid selector if could not be found.
segment_selector_t gdt_get_nth_null_selector (size_t);
segment_selector_t gdt_get_nth_code_selector (size_t);
segment_selector_t gdt_get_nth_data_selector (size_t);
segment_selector_t gdt_get_nth_tss_selector  (size_t);

#define MKGDT_GET_FIRST_SELECTOR(type) \
static inline segment_selector_t gdt_get_first_##type##_selector () { \
	return gdt_get_nth_##type##_selector (0); \
}

MKGDT_GET_FIRST_SELECTOR(null)
MKGDT_GET_FIRST_SELECTOR(code)
MKGDT_GET_FIRST_SELECTOR(data)
MKGDT_GET_FIRST_SELECTOR(tss)

segment_selector_t gdt_add_tss (tss_t *);

#endif

// vim: set ts=4 sw=4 noet syn=c:
