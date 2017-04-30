// kernel/arch/x86/mm/gdt.h

#ifndef IZIX_GDT_H
#define IZIX_GDT_H 1

#include <izixboot/gdt.h>

void gdt_register (gdt_register_t *);
void gdt_dump_entries ();

#endif

// vim: set ts=4 sw=4 noet syn=c:
