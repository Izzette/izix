// kernel/arch/x86/include/mm/e820.h

#ifndef IZIX_E820_H
#define IZIX_E820_H 1

#include <stddef.h>

#include <mm/paging.h>

// For editors
#ifndef IZIX
#define IZIX_SUPPORT_E820_LEGACY
#define IZIX_SUPPORT_E820_3X
#endif

#ifdef IZIX_SUPPORT_E820_3X
void e820_3x_register (size_t entry_count, void *entries);
void e820_3x_print ();
void e820_3x_add_freemem ();
void e820_3x_clone ();
void e820_3x_map_physical ();
#endif

#ifdef IZIX_SUPPORT_E820_LEGACY
void e820_legacy_register (size_t entry_count, void *entries);
void e820_legacy_print ();
void e820_legacy_add_freemem ();
void e820_legacy_clone ();
void e820_legacy_map_physical (paging_data_t *);
#endif

#endif

// vim: set ts=4 sw=4 noet syn=c:
