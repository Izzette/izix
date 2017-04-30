// kernel/arch/x86/include/mm/e820.h

#ifndef IZIX_E820_H
#define IZIX_E820_H 1

#include <stddef.h>

#include <izixboot/memmap.h>

void e820_register (size_t entry_count, e820_3x_entry_t *entries);
void e820_dump_entries ();

#endif

// vim: set ts=4 sw=4 noet syn=c:
