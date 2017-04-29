// kernel/boot/izixboot_main.c

#include <stddef.h>
#include <stdint.h>

#include <izixboot/memmap.h>
#include <izixboot/gdt.h>
#include <izixboot/gdt32.h>

#include <string.h>
#include <format.h>

#include <tty/tty_driver.h>
#include <tty/tty_vga_text.h>
#include <kprint/kprint.h>

__attribute__((force_align_arg_pointer))
void kernel_main (const uint32_t entry_count_u32, const uint32_t entries_u32, const uint32_t gdtr_u32) {
#pragma GCC diagnostic ignored "-Wint-to-pointer-cast"
	const e820_3x_entry_t *entries = (e820_3x_entry_t *)entries_u32;
	const gdt_register_t *gdtr = (gdt_register_t *)gdtr_u32;
#pragma GCC diagnostic pop
	const size_t entry_count = (size_t)entry_count_u32;

	size_t i;

	tty_driver_t tty_driver_vga_text = get_tty_vga ();

	tty_driver_vga_text.init (&tty_driver_vga_text);

	set_kprint_tty_driver (tty_driver_vga_text);

	kprintf (
		"Kernel command line: "
			"entry_count=%zd "
			"entries=%p "
			"gdtr=%p\n",
		entry_count,
		entries,
		gdtr);

	kputs ("e820: BIOS-provided physical RAM map:\n");

	for (i = 0; entry_count > i; ++i) {
		if (0 == entries[i].length)
			continue;
		if (0 == (E820_3X_XATTRS_DO_NOT_IGNORE & entries[i].xattrs))
			continue;

		uint32_t base, end;
		char *type;
		char *xattr;

		base = entries[i].base;
		end  = base + entries[i].length - 1;

		switch (entries[i].type) {
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

		if (0 != (E820_3X_XATTRS_NON_VOLITALE & entries[i].xattrs))
			xattr = " persistent";
		else
			xattr = "";

		kprintf ("BIOS-e820: [mem 0x%016x-0x%016x] %s%s\n", base, end, type, xattr);
	}

	tty_driver_vga_text.release (&tty_driver_vga_text);
}

// vim: set ts=4 sw=4 noet syn=c:
