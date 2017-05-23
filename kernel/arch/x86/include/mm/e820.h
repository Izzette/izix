// kernel/arch/x86/include/mm/e820.h

#ifndef IZIX_E820_H
#define IZIX_E820_H 1

#include <stddef.h>

// For editors
#if !defined(IZIX_SUPPORT_E820_LEGACY) && !defined(IZIX_SUPPORT_E820_3X)
#define IZIX_SUPPORT_E820_LEGACY
#define IZIX_SUPPORT_E820_3X
#endif

#ifdef IZIX_SUPPORT_E820_3X
void e820_3x_register (size_t entry_count, void *entries);
void e820_3x_print ();
void e820_3x_add_freemem ();
#endif

#ifdef IZIX_SUPPORT_E820_LEGACY
void e820_legacy_register (size_t entry_count, void *entries);
void e820_legacy_print ();
void e820_legacy_add_freemem ();
#endif

#endif

// vim: set ts=4 sw=4 noet syn=c:
