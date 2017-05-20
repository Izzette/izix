// kernel/arch/x86/mm/gdt.c

#include <stdbool.h>

#include <izixboot/gdt.h>
#include <izixboot/gdt32.h>

#include <string.h>

#include <kprint/kprint.h>
#include <kpanic/kpanic.h>
#include <mm/malloc.h>
#include <mm/gdt.h>
#include <sched/tss.h>

#define GDT_TSS_ACCESS_BYTE 0x89
#define GDT_TSS_FLAGS       0x4

static gdt_register_t *gdtr;

static gdt32_logical_register_t get_logical_gdtr (gdt_register_t registry) {
	gdt32_logical_register_t logical_registry;

	gdt32_register_decode (registry, &logical_registry);

	return logical_registry;
}

static gdt32_logical_entry_t get_logical_entry (gdt32_entry_t entry)  {
	gdt32_logical_entry_t logical_entry;

	gdt32_decode (entry, &logical_entry);

	return logical_entry;
}

static void print_logical_gdtr (gdt32_logical_register_t logical_registry) {
	kprintf ("mm/gdt: size=%u offset=%p\n",
		logical_registry.size,
		logical_registry.offset);
}

static void print_logical_entry (gdt32_logical_entry_t logical_entry) {
#pragma GCC diagnostic ignored "-Wint-to-pointer-cast"
	kprintf ("mm/gdt: limit=0x%05x base=%p\n",
		logical_entry.limit,
		(void *)logical_entry.base);
#pragma GCC diagnostic pop
	kprintf ("\taccess: %s %s %s %s %d %s\n",
		logical_entry.access.accessed             ? "AC" : "--",
		logical_entry.access.read_write           ? "RW" : "--",
		logical_entry.access.direction_conforming ? "DC" : "--",
		logical_entry.access.executable           ? "EX" : "--",
		logical_entry.access.priviledge,
		logical_entry.access.present              ? "PR" : "--");
	kprintf ("\tflags: %s %s\n",
		logical_entry.flags.size        ? "SZ" : "--",
		logical_entry.flags.granularity ? "GR" : "--");
}

static void print_logical_tss (gdt32_logical_entry_t logical_entry) {
#pragma GCC diagnostic ignored "-Wint-to-pointer-cast"
	kprintf ("mm/gdt: TSS=%p\n", (void *)logical_entry.base);
#pragma GCC diagnostic pop
}

static gdt_access_t gdt_get_access_byte (gdt32_entry_t entry) {
	return (entry & GDT_ACCESS_DEMASK) >> GDT_ACCESS_OFFSET;
}

static gdt32_flags_t gdt_get_flags (gdt32_entry_t entry) {
	return (entry & GDT_FLAGS_DEMASK) >> GDT_FLAGS_OFFSET;
}

static bool gdt_is_null (gdt32_entry_t entry) {
	return 0 == entry;
}

static bool gdt_is_tss (gdt32_entry_t);

#define MKGDT_IS_EXEC(name, oper) \
static bool gdt_is_##name (gdt32_entry_t entry) { \
	if (gdt_is_null (entry) || gdt_is_tss (entry)) \
		return false; \
\
	gdt32_logical_entry_t logical_entry; \
	gdt32_decode (entry, &logical_entry); \
\
	if (oper logical_entry.access.executable) \
		return true; \
\
	return false; \
}

MKGDT_IS_EXEC(code, )
MKGDT_IS_EXEC(data, !)

static bool gdt_is_tss (gdt32_entry_t entry) {
	return (GDT_TSS_ACCESS_BYTE == gdt_get_access_byte (entry) &&
			GDT_TSS_FLAGS       == gdt_get_flags       (entry));
}

static segment_selector_t gdt_get_segment_selector (size_t i) {
	return GDT_SELECTOR_INC * i;
}

static void __attribute__((noinline)) gdt_reload () {
	// TODO: allow code segement selectors other than 0x08.
	asm volatile (
		"		lgdt		(%0);\n"
		"		ljmp		$0x08,				$gdt_reload_target;\n"
		"gdt_reload_target:\n"
		"		nop;\n"
		:
		:"r"(gdtr));
}

void gdt_register (gdt_register_t *registry) {
	gdt32_logical_register_t logical_registry = get_logical_gdtr (*registry);

	const size_t gdt_entries_length = logical_registry.size * sizeof(gdt32_entry_t);

	gdt32_entry_t *new_entries = malloc (gdt_entries_length);
	gdt_register_t *new_registry = malloc (sizeof(gdt_register_t));

	memcpy (new_entries, logical_registry.offset, gdt_entries_length);

	logical_registry.offset = new_entries;
	*new_registry = gdt32_register_encode (logical_registry);

	gdtr = new_registry;

	gdt_reload ();
}

void gdt_dump_entries () {
	size_t i;

	gdt32_logical_register_t logical_registry = get_logical_gdtr (*gdtr);

	kputs ("mm/gdt: GDT registry:\n");
	print_logical_gdtr (logical_registry);

	kprintf ("mm/gdt: GDT entries %d:\n", logical_registry.size);

	// Skip the first entry because it is the obligitory NULL entry.
	for (i = 0; logical_registry.size > i; ++i) {
		gdt32_entry_t entry = logical_registry.offset[i];

		if (0 == i) {
			if (0 == entry)
				kputs ("mm/gdt: NULL\n");
			else
				kputs ("mm/gdt: *NOT* NULL\n");
		} else {
			gdt32_logical_entry_t logical_entry = get_logical_entry (entry);

			// Entry changes after lgdt and/or ltr??
			if (gdt_is_tss (entry))
				// Assume TSS
				print_logical_tss (logical_entry);
			else
				print_logical_entry (logical_entry);
		}
	}
}

#define MKGDT_GET_SELECTOR(type) \
segment_selector_t gdt_get_nth_##type##_selector (size_t n) { \
	const gdt32_logical_register_t logical_registry = get_logical_gdtr (*gdtr); \
\
	size_t i; \
\
	for (i = 0; logical_registry.size > i; ++i) { \
		gdt32_entry_t entry = logical_registry.offset[i]; \
\
		if (gdt_is_##type (entry)) \
			if (!n--) \
				return gdt_get_segment_selector (i); \
	} \
\
	/* Invalid valid selector. */ \
	return 0x01; \
}

MKGDT_GET_SELECTOR(null)
MKGDT_GET_SELECTOR(code)
MKGDT_GET_SELECTOR(data)
MKGDT_GET_SELECTOR(tss)

segment_selector_t gdt_add_tss (tss_t *tss_ptr) {
	gdt32_logical_register_t logical_registry = get_logical_gdtr (*gdtr);

	const size_t gdt_entries_old_length = logical_registry.size * sizeof(gdt32_entry_t);

	gdt32_entry_t *new_entries = realloc (
			logical_registry.offset, gdt_entries_old_length + 1);
	if (!new_entries) {
		kputs ("mm/gdt: Failed to reallocate GDT!\n");
		kpanic ();
	}

	if (logical_registry.offset != new_entries)
		logical_registry.offset = new_entries;

	logical_registry.size += 1;

	logical_registry.offset[logical_registry.size - 1] =
		(((gdt32_entry_t)(sizeof(tss_t)   & GDT_LIMIT_LOW_BITMASK))  << (GDT_LIMIT_LOW_OFFSET))                         |
		(((gdt32_entry_t)((size_t)tss_ptr & GDT_BASE_LOW_BITMASK))   << (GDT_BASE_LOW_OFFSET))                          |
		(((gdt32_entry_t)(GDT_TSS_ACCESS_BYTE))                      << (GDT_ACCESS_OFFSET))                            |
		(((gdt32_entry_t)(sizeof(tss_t)   & GDT_LIMIT_HIGH_BITMASK)) << (GDT_LIMIT_HIGH_OFFSET - GDT_LIMIT_LOW_LENGTH)) |
		(((gdt32_entry_t)(GDT_TSS_FLAGS))                            << (GDT_FLAGS_OFFSET))                             |
		(((gdt32_entry_t)((size_t)tss_ptr & GDT_BASE_HIGH_BITMASK))  << (GDT_BASE_HIGH_OFFSET - GDT_LIMIT_LOW_LENGTH));

	*gdtr = gdt32_register_encode (logical_registry);

	gdt_reload ();

	return gdt_get_segment_selector (logical_registry.size - 1);
}

// vim: set ts=4 sw=4 noet syn=c:
