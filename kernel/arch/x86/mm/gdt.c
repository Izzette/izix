// kernel/arch/x86/mm/gdt.c

#include <izixboot/gdt.h>
#include <izixboot/gdt32.h>

#include <kprint/kprint.h>

static gdt_register_t *gdtr;

static inline gdt32_logical_register_t get_logical_gdtr (gdt_register_t registry) {
	gdt32_logical_register_t logical_registry;

	gdt32_register_decode (registry, &logical_registry);

	return logical_registry;
}

static inline gdt32_logical_entry_t get_logical_entry (gdt32_entry_t entry)  {
	gdt32_logical_entry_t logical_entry;

	gdt32_decode (entry, &logical_entry);

	return logical_entry;
}

static inline void print_logical_gdtr (gdt32_logical_register_t logical_registry) {
	kprintf ("mm/gdt: size=%u offset=%p\n",
		logical_registry.size,
		logical_registry.offset);
}

static inline void print_logical_entry (gdt32_logical_entry_t logical_entry) {
	kprintf ("mm/gdt: limit=0x%05x base=%p\n", logical_entry.limit, logical_entry.base);
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

void gdt_register (gdt_register_t *registry) {
	gdtr = registry;
}

void gdt_dump_entries () {
	size_t i;

	gdt32_logical_register_t logical_registry = get_logical_gdtr (*gdtr);


	kputs ("mm/gdt: GDT registry:\n");
	print_logical_gdtr (logical_registry);

	kprintf ("mm/gdt: GDT entries:\n", logical_registry.size);

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

			print_logical_entry (logical_entry);
		}
	}
}


// vim: set ts=4 sw=4 noet syn=c:
