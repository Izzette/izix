// kernel/arch/x86/mm/e820.c

// For some reason, this is much bigger when compiled with optimizations, so we will
// not optimize it.
#pragma GCC optimize ("O0")

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#include <attributes.h>
#include <string.h>

#include <kprint/kprint.h>
#include <kpanic/kpanic.h>
#include <mm/freemem.h>
#include <mm/malloc.h>
#include <mm/e820.h>
#include <mm/page.h>
#include <mm/paging.h>

#define E820_FORMAT_STR_BASE "mm/e820: length=0x%016llx base=*0x%016llx %s"

typedef enum e820_type_enum {
	e820_usable    = 1,
	e820_reserved  = 2,
	e820_acpi_data = 3,
	e820_acpi_nvs  = 4,
	e820_bad       = 5
} e820_type_t;

static size_t e820_entry_count;

static page_attrs_t e820_type_attrs[5];

COLD
static char *e820_get_type_str (e820_type_t type) {
	switch (type) {
		case e820_usable:
			return "usable";
		case e820_reserved:
			return "reserved";
		case e820_acpi_data:
			return "ACPI data";
		case e820_acpi_nvs:
			return "ACPI NVS";
		case e820_bad:
			return "bad";
		default:
			kputs ("mm/e820: Found invalid e820 type in entry!\n");
			kpanic ();
	}
}

COLD
static void e820_header_print () {
	kputs ("mm/e820: BIOS-provided physical RAM map:\n");
}

// Get bounded length of memory region, or zero if it is out of bounds.
COLD
static size_t e820_get_bounded_length (uint64_t length, uint64_t base) {
	if (UINTPTR_MAX < base)
		return 0;

	if (SIZE_MAX < length)
		length = SIZE_MAX;

	// Unfortunetly, freemem computes exclusive maxs, and thus cannot handle regions
	// whose exclusive max is equal to zero, thus we sacrafice the last byte and compute
	// the exclusive max here.
	if (UINTPTR_MAX < length + base)
		return UINTPTR_MAX - base;

	return length;
}

COLD
static void e820_add_freemem (uint64_t length_u64, uint64_t base_u64) {
	const size_t length = e820_get_bounded_length (length_u64, base_u64);
	if (!length)
		return;

	// Gurenteed to be within bounds because e820_get_bounded_length returned non-zero.
	void *const base = (void *)(size_t)base_u64;

	const freemem_region_t region = new_freemem_region (base, length);

	const bool add_success = freemem_add_region (region);
	if (!add_success) {
		kputs ("mm/e820: Failed to add freemem region!\n");
		kpanic ();
	}
}

COLD
static void e820_map_page (
		uint64_t length_u64,
		uint64_t base_u64,
		e820_type_t type,
		bool volatility,
		paging_data_t *paging_data
) {
	const size_t length = e820_get_bounded_length (length_u64, base_u64);
	if (!length)
		return;

	// Gurenteed to be within bounds because e820_get_bounded_length returned non-zero.
	void *const base = (void *)(size_t)base_u64;

	page_t *page_base = base;
	size_t page_length = length;

	// Round down to the nearest page!
	if ((size_t)page_base % PAGE_SIZE) {
		page_length += (size_t)page_base % PAGE_SIZE;
		page_base = (void *)page_base - (size_t)page_base % PAGE_SIZE;
	}

	// Round up to the nearest page!
	if (page_length % PAGE_SIZE)
		page_length += PAGE_SIZE - page_length % PAGE_SIZE;

	while (page_length) {
		page_t *virtual = page_base;

		page_attrs_t compat_attrs = e820_type_attrs[type - 1];
		if (!volatility)
			compat_attrs.write_through = true;
		if (paging_table_present (virtual, paging_data)) {
			page_attrs_t old_attrs = paging_get_attrs (virtual, paging_data);
			compat_attrs = paging_compatable_attrs (compat_attrs, old_attrs);
		}

		paging_set_map (virtual, virtual, paging_data);
		paging_set_attrs (virtual, compat_attrs, paging_data);

		page_length -= PAGE_SIZE;
		page_base += 1;
	}
}

CONSTRUCTOR
void e820_construct () {
	e820_type_attrs[e820_usable - 1]    = (page_attrs_t){
		.present        = true,
		.writable       = true,
		.user           = false,
		.write_through  = false,
		.cache_disabled = false,
		.accessed       = false,
		.dirty          = false,
		.global         = false
	};
	e820_type_attrs[e820_acpi_nvs - 1]  = (page_attrs_t){
		.present        = false, // Page fault on access.
		.writable       = false,
		.user           = false,
		.write_through  = true,
		.cache_disabled = true,
		.accessed       = false,
		.dirty          = false,
		.global         = false
	};
	e820_type_attrs[e820_acpi_data - 1] = (page_attrs_t){
		.present        = false, // Page fault on access.
		.writable       = false,
		.user           = false,
		.write_through  = false,
		.cache_disabled = false,
		.accessed       = false,
		.dirty          = false,
		.global         = false
	};
	e820_type_attrs[e820_reserved - 1]  = (page_attrs_t){
		.present        = false, // Page fault on access.
		.writable       = false,
		.user           = false,
		.write_through  = false,
		.cache_disabled = true,
		.accessed       = false,
		.dirty          = false,
		.global         = false
	};
	e820_type_attrs[e820_bad - 1]       = (page_attrs_t){
		.present        = false, // Page fault on access.
		.writable       = false,
		.user           = false,
		.write_through  = false,
		.cache_disabled = false,
		.accessed       = false,
		.dirty          = false,
		.global         = false
	};
}

#ifdef IZIX_SUPPORT_E820_3X

typedef enum e820_3x_ignore_enum {
	e820_3x_ignore      = 0b0,
	e820_3x_acknowledge = 0b1
} e820_3x_ignore_t;

typedef enum e820_3x_volatility_enum {
	e820_3x_volatile     = 0b0,
	e820_3x_non_volatile = 0b1
} e820_3x_volatility_t;

typedef struct PACKED e820_3x_xattrs_struct {
	e820_3x_ignore_t     ignored     : 1;
	e820_3x_volatility_t volatility  : 1;
	uint32_t             _rsv0       : 30;
} e820_3x_xattrs_t;

typedef struct PACKED e820_entry_3x_struct {
	uint64_t         base;
	uint64_t         length;
#if 4 <= __SIZEOF_INT__ // In C99 6.7.2.2 enums are guaranteed to be the size of an int.
	e820_type_t      type  : 32;
#else
	e820_type_t      type;
	uint32_t         _rsv0 : 32 -  8 * __SIZEOF_INT__;
#endif
	e820_3x_xattrs_t xattrs;
} e820_entry_3x_t;

static e820_entry_3x_t *e820_entries_3x;

COLD
static void e820_entry_3x_print (e820_entry_3x_t entry) {
	if (!entry.length || e820_3x_ignore == entry.xattrs.ignored)
		kputs ("mm/e820: ignored\n");

	const char *type = e820_get_type_str (entry.type);
	const char *xattr =
		e820_3x_non_volatile == entry.xattrs.volatility ?
		" (non-volatile)" : "";

	kprintf (
		E820_FORMAT_STR_BASE "%s\n",
		entry.length, entry.base, type, xattr);
}

COLD
static void e820_entry_3x_add_freemem (e820_entry_3x_t entry) {
	if (!entry.length ||
			e820_usable != entry.type ||
			e820_3x_ignore == entry.xattrs.ignored)
		return;

	e820_add_freemem (entry.length, entry.base);
}

COLD
static void e820_entry_3x_map_page (e820_entry_3x_t entry, paging_data_t *paging_data) {
	if (!entry.length || e820_3x_ignore == entry.xattrs.ignored)
		return;

	bool volatility = e820_3x_volatile == entry.xattrs.volatility;
	e820_map_page (entry.length, entry.base, entry.type, volatility, paging_data);
}

COLD
void e820_3x_print () {
	e820_header_print ();

	size_t i;
	for (i = 0; e820_entry_count > i; ++i)
		e820_entry_3x_print (e820_entries_3x[i]);
}

COLD
void e820_3x_register (size_t entry_count, void *entries) {
	e820_entry_count = entry_count;
	e820_entries_3x = entries;
}

COLD
void e820_3x_add_freemem () {
	size_t i;
	for (i = 0; e820_entry_count > i; ++i)
		e820_entry_3x_add_freemem (e820_entries_3x[i]);
}

COLD
void e820_3x_clone () {
	const size_t entries_size = e820_entry_count * sizeof(e820_entry_3x_t);

	e820_entry_3x_t *new_entries = malloc (entries_size);
	if (!new_entries) {
		kputs ("mm/e820: Failed to allocate new e820 entries!\n");
		kpanic ();
	}

	memcpy (new_entries, e820_entries_3x, entries_size);

	e820_entries_3x = new_entries;
}

COLD
void e820_3x_map_physical (paging_data_t *paging_data) {
	size_t i;
	for (i = 0; e820_entry_count > i; ++i)
		e820_entry_3x_map_page (e820_entries_3x[i], paging_data);
}

#endif // IZIX_SUPPORT_E820_3X

#ifdef IZIX_SUPPORT_E820_LEGACY

typedef struct e820_entry_legacy_struct {
	uint64_t         base;
	uint64_t         length;
	e820_type_t      type; // The actual size doesn't matter on little-endian systems.
} e820_entry_legacy_t;

static e820_entry_legacy_t *e820_entries_legacy;

COLD
static void e820_entry_legacy_print (e820_entry_legacy_t entry) {
	if (!entry.length)
		kputs ("mm/e820: ignored\n");

	const char *type = e820_get_type_str (entry.type);

	kprintf (
		E820_FORMAT_STR_BASE "\n",
		entry.length, entry.base, type);
}

COLD
static void e820_entry_legacy_add_freemem (e820_entry_legacy_t entry) {
	if (!entry.length ||
			e820_usable != entry.type)
		return;

	e820_add_freemem (entry.length, entry.base);
}

COLD
static void e820_entry_legacy_map_page (e820_entry_legacy_t entry, paging_data_t *paging_data) {
	if (!entry.length)
		return;

	e820_map_page (entry.length, entry.base, entry.type, true, paging_data);
}

COLD
void e820_legacy_print () {
	e820_header_print ();

	size_t i;
	for (i = 0; e820_entry_count > i; ++i)
		e820_entry_legacy_print (e820_entries_legacy[i]);
}

COLD
void e820_legacy_register (size_t entry_count, void *entries) {
	e820_entry_count = entry_count;
	e820_entries_legacy = entries;
}

COLD
void e820_legacy_add_freemem () {
	size_t i;
	for (i = 0; e820_entry_count > i; ++i)
		e820_entry_legacy_add_freemem (e820_entries_legacy[i]);
}

COLD
void e820_legacy_clone () {
	const size_t entries_size = e820_entry_count * sizeof(e820_entry_legacy_t);

	e820_entry_legacy_t *new_entries = malloc (entries_size);
	if (!new_entries) {
		kputs ("mm/e820: Failed to allocate new e820 entries!\n");
		kpanic ();
	}

	memcpy (new_entries, e820_entries_legacy, entries_size);

	e820_entries_legacy = new_entries;
}

COLD
void e820_legacy_map_physical (paging_data_t *paging_data) {
	size_t i;
	for (i = 0; e820_entry_count > i; ++i)
		e820_entry_legacy_map_page (e820_entries_legacy[i], paging_data);
}

#endif // IZIX_SUPPORT_E820_LEGACY

// vim: set ts=4 sw=4 noet syn=c:
