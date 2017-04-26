// kernel/boot/izixboot_main.c

#include <stddef.h>
#include <stdint.h>

#include <string.h>
#include <format.h>

#include <video/vga_text.h>

#include <izixboot/memmap.h>
#include <izixboot/gdt.h>
#include <izixboot/gdt32.h>

__attribute__((force_align_arg_pointer))
void kernel_main (const uint32_t entry_count_u32, const uint32_t entries_u32, const uint32_t gdtr_u32) {
#pragma GCC diagnostic ignored "-Wint-to-pointer-cast"
	const e820_3x_entry_t *entries = (e820_3x_entry_t *)entries_u32;
	const gdt_register_t *gdtr = (gdt_register_t *)gdtr_u32;
#pragma GCC diagnostic pop
	const size_t entry_count = (size_t)entry_count_u32;

	char buffer[17];
	size_t i;

	vga_text_initialize ();

	vga_text_writestring ("Kernel command line: entry_count=");
	ulltoa ((unsigned long)entry_count_u32, buffer, 10);
	vga_text_writestring (buffer);
	vga_text_writestring (" entries=0x");
	ulltoa ((unsigned long)entries_u32, buffer, 16);
	strpadl (buffer, '0', 8);
	vga_text_writestring (buffer);
	vga_text_writestring (" gdtr=0x");
	ulltoa ((unsigned long)gdtr_u32, buffer, 16);
	strpadl (buffer, '0', 8);
	vga_text_writestring (buffer);
	vga_text_writestring ("\n");

	vga_text_writestring ("e820: BIOS-provided physical RAM map:\n");

	for (i = 0; entry_count > i; ++i) {
		if (0 == entries[i].length)
			continue;
		if (0 == (E820_3X_XATTRS_DO_NOT_IGNORE & entries[i].xattrs))
			continue;

		vga_text_writestring ("BIOS-e820: [mem ");

		vga_text_writestring ("0x");
		ulltoa (entries[i].base, buffer, 16);
		strpadl (buffer, '0', 16);
		vga_text_writestring (buffer);

		vga_text_writestring ("-0x");
		ulltoa (entries[i].base + entries[i].length - 1, buffer, 16);
		strpadl (buffer, '0', 16);
		vga_text_writestring (buffer);

		vga_text_writestring ("] ");

		switch (entries[i].type) {
			case E820_TYPE_USABLE:
				vga_text_writestring ("usable");
				break;
			case E820_TYPE_RESERVED:
				vga_text_writestring ("reserved");
				break;
			case E820_TYPE_RECLAIM:
				vga_text_writestring ("ACPI data");
				break;
			case E820_TYPE_NVS:
				vga_text_writestring ("ACPI NVS");
				break;
			case E820_TYPE_BAD:
				vga_text_writestring ("bad");
				break;
			default:
				vga_text_writestring ("unknown");
		}

		if (0 != (E820_3X_XATTRS_NON_VOLITALE & entries[i].xattrs))
			vga_text_writestring (" persistent");

		vga_text_writestring ("\n");
	}
}

// vim: set ts=4 sw=4 noet syn=c:
