// kernel/arch/x86/mm/e820.c

#include <stddef.h>

#include <izixboot/memmap.h>

#include <kprint/kprint.h>

static size_t e820_entry_count;
static e820_3x_entry_t *e820_entries;

static inline void print_entry (e820_3x_entry_t entry) {
	if (!entry.length || !(E820_3X_XATTRS_DO_NOT_IGNORE & entry.xattrs))
		kputs ("mm/e820: IGNORED\n");

	char *type;
	char *xattr;

	switch (entry.type) {
		case E820_TYPE_USABLE:
			type = "usable";
			break;
		case E820_TYPE_RESERVED:
			type = "reserved";
			break;
		case E820_TYPE_RECLAIM:
			type = "ACPI data";
			break;
		case E820_TYPE_NVS:
			type = "ACPI NVS";
			break;
		case E820_TYPE_BAD:
			type = "bad";
			break;
		default:
			type = "unknown";
	}

	if (0 != (E820_3X_XATTRS_NON_VOLITALE & entry.xattrs))
		xattr = " (persistent)";
	else
		xattr = "";

	kprintf (
		"mm/e820: length=0x%016llx base=*0x%016llx %s%s\n",
		entry.length, entry.base, type, xattr);
}

void e820_register (size_t entry_count, e820_3x_entry_t *entries) {
	e820_entry_count = entry_count;
	e820_entries = entries;
}

void e820_dump_entries () {
	size_t i;

	kputs ("mm/e820: BIOS-provided physical RAM map:\n");

	for (i = 0; e820_entry_count > i; ++i)
		print_entry (e820_entries[i]);
}

// vim: set ts=4 sw=4 noet syn=c:
