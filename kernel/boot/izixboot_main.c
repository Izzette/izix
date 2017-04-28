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

	char buffer[17];
	size_t i;

	tty_driver_t tty_driver_vga_text = get_tty_vga ();

	tty_driver_vga_text.init (&tty_driver_vga_text);

	set_kprint_tty_driver (tty_driver_vga_text);

	kputs ("Kernel command line: entry_count=");
	ulltoa ((unsigned long)entry_count_u32, buffer, 10);
	kputs (buffer);
	kputs (" entries=0x");
	ulltoa ((unsigned long)entries_u32, buffer, 16);
	strpadl (buffer, '0', 8);
	kputs (buffer);
	kputs (" gdtr=0x");
	ulltoa ((unsigned long)gdtr_u32, buffer, 16);
	strpadl (buffer, '0', 8);
	kputs (buffer);
	kputs ("\n");

	kputs ("e820: BIOS-provided physical RAM map:\n");

	for (i = 0; entry_count > i; ++i) {
		if (0 == entries[i].length)
			continue;
		if (0 == (E820_3X_XATTRS_DO_NOT_IGNORE & entries[i].xattrs))
			continue;

		kputs ("BIOS-e820: [mem ");

		kputs ("0x");
		ulltoa (entries[i].base, buffer, 16);
		strpadl (buffer, '0', 16);
		kputs (buffer);

		kputs ("-0x");
		ulltoa (entries[i].base + entries[i].length - 1, buffer, 16);
		strpadl (buffer, '0', 16);
		kputs (buffer);

		kputs ("] ");

		switch (entries[i].type) {
			case E820_TYPE_USABLE:
				kputs ("usable");
				break;
			case E820_TYPE_RESERVED:
				kputs ("reserved");
				break;
			case E820_TYPE_RECLAIM:
				kputs ("ACPI data");
				break;
			case E820_TYPE_NVS:
				kputs ("ACPI NVS");
				break;
			case E820_TYPE_BAD:
				kputs ("bad");
				break;
			default:
				kputs ("unknown");
		}

		if (0 != (E820_3X_XATTRS_NON_VOLITALE & entries[i].xattrs))
			kputs (" persistent");

		kputs ("\n");
	}

	tty_driver_vga_text.release (&tty_driver_vga_text);
}

// vim: set ts=4 sw=4 noet syn=c:
