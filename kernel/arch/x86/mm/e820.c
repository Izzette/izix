// kernel/arch/x86/mm/e820.c

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#include <kprint/kprint.h>
#include <kpanic/kpanic.h>
#include <mm/freemem.h>
#include <mm/e820.h>

#define E820_FORMAT_STR_BASE "mm/e820: length=0x%016llx base=*0x%016llx %s"

typedef enum e820_type_enum {
	e820_usable    = 1,
	e820_reserved  = 2,
	e820_acpi_data = 3,
	e820_acpi_nvs  = 4,
	e820_bad       = 5
} e820_type_t;

static size_t e820_entry_count;

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

static void e820_header_print () {
	kputs ("mm/e820: BIOS-provided physical RAM map:\n");
}

// Get bounded length of memory region, or zero if it is out of bounds.
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

#ifdef IZIX_SUPPORT_E820_3X

typedef enum e820_3x_ignore_enum {
	e820_3x_ignore      = 0b0,
	e820_3x_acknowledge = 0b1
} e820_3x_ignore_t;

typedef enum e820_3x_volatility_enum {
	e820_3x_volatile     = 0b0,
	e820_3x_non_volatile = 0b1
} e820_3x_volatility_t;

typedef struct __attribute__((packed)) e820_3x_xattrs_struct {
	e820_3x_ignore_t     ignored     : 1;
	e820_3x_volatility_t volatitlity : 1;
	uint32_t             _rsv0       : 30;
} e820_3x_xattrs_t;

typedef struct __attribute__((packed)) e820_entry_3x_struct {
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

static void e820_entry_3x_print (e820_entry_3x_t entry) {
	if (!entry.length || e820_3x_ignore == entry.xattrs.ignored)
		kputs ("mm/e820: ignored\n");

	const char *type = e820_get_type_str (entry.type);
	const char *xattr =
		e820_3x_non_volatile == entry.xattrs.volatitlity ?
		" (non-volatile)" : "";

	kprintf (
		E820_FORMAT_STR_BASE "%s\n",
		entry.length, entry.base, type, xattr);
}

static void e820_entry_3x_add_freemem (e820_entry_3x_t entry) {
	if (!entry.length ||
			e820_usable != entry.type ||
			e820_3x_ignore == entry.xattrs.ignored)
		return;

	e820_add_freemem (entry.length, entry.base);
}

void e820_3x_print () {
	e820_header_print ();

	size_t i;
	for (i = 0; e820_entry_count > i; ++i)
		e820_entry_3x_print (e820_entries_3x[i]);
}

void e820_3x_register (size_t entry_count, void *entries) {
	e820_entry_count = entry_count;
	e820_entries_3x = entries;
}

void e820_3x_add_freemem () {
	size_t i;
	for (i = 0; e820_entry_count > i; ++i)
		e820_entry_3x_add_freemem (e820_entries_3x[i]);
}

#endif // IZIX_SUPPORT_E820_3X

#ifdef IZIX_SUPPORT_E820_LEGACY

typedef struct e820_entry_legacy_struct {
	uint64_t         base;
	uint64_t         length;
	e820_type_t      type; // The actual size doesn't matter on little-endian systems.
} e820_entry_legacy_t;

static e820_entry_legacy_t *e820_entries_legacy;

static void e820_entry_legacy_print (e820_entry_legacy_t entry) {
	if (!entry.length)
		kputs ("mm/e820: ignored\n");

	const char *type = e820_get_type_str (entry.type);

	kprintf (
		E820_FORMAT_STR_BASE "\n",
		entry.length, entry.base, type);
}

static void e820_entry_legacy_add_freemem (e820_entry_legacy_t entry) {
	if (!entry.length ||
			e820_usable != entry.type)
		return;

	e820_add_freemem (entry.length, entry.base);
}

void e820_legacy_print () {
	e820_header_print ();

	size_t i;
	for (i = 0; e820_entry_count > i; ++i)
		e820_entry_legacy_print (e820_entries_legacy[i]);
}

void e820_legacy_register (size_t entry_count, void *entries) {
	e820_entry_count = entry_count;
	e820_entries_legacy = entries;
}

void e820_legacy_add_freemem () {
	size_t i;
	for (i = 0; e820_entry_count > i; ++i)
		e820_entry_legacy_add_freemem (e820_entries_legacy[i]);
}

#endif // IZIX_SUPPORT_E820_LEGACY

// vim: set ts=4 sw=4 noet syn=c:
