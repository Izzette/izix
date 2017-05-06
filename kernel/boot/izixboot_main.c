// kernel/boot/izixboot_main.c

#include <stddef.h>
#include <stdint.h>

#include <izixboot/memmap.h>
#include <izixboot/gdt.h>

#include <tty/tty_driver.h>
#include <tty/tty_vga_text.h>
#include <kprint/kprint.h>
#include <mm/freemem.h>
#include <mm/gdt.h>
#include <mm/e820.h>

__attribute__((force_align_arg_pointer))
void kernel_main (
		uint32_t e820_entry_count_u32,
		uint32_t e820_entries_u32,
		uint32_t gdtr_u32
) {
	size_t e820_entry_count = (size_t)e820_entry_count_u32;
#pragma GCC diagnostic ignored "-Wint-to-pointer-cast"
	e820_3x_entry_t *e820_entries = (e820_3x_entry_t *)e820_entries_u32;
	gdt_register_t *gdtr = (gdt_register_t *)gdtr_u32;
#pragma GCC diagnostic pop

	tty_driver_t tty_driver_vga_text = get_tty_vga ();

	tty_driver_vga_text.init (&tty_driver_vga_text);

	set_kprint_tty_driver (tty_driver_vga_text);

	kprintf (
		"boot/izixboot_main: Provided command line:\n"
			"\te820_entry_count=%zd\n"
			"\te820_entries=%p\n"
			"\tgdtr=%p\n",
		e820_entry_count,
		e820_entries,
		gdtr);

	e820_register (e820_entry_count, e820_entries);
	e820_dump_entries ();

	freemem_region_t kernel_region, stack_region, freemem_internal_region;

	// TODO: get the actually start and length of the kernel.
	kernel_region = new_freemem_region (
		(void *)0x8000,
		(size_t)(512 * 127));
	stack_region = new_freemem_region (
		(void *)0x0,
		(size_t)kernel_region.p);
	freemem_internal_region = new_freemem_region (
		freemem_get_region_end (kernel_region), // Exclusive max.
		(size_t)0x1000);

	freemem_init (freemem_internal_region.p, freemem_internal_region.length);

	e820_add_freemem ();

	freemem_remove_region (freemem_internal_region);
	freemem_remove_region (stack_region);
	freemem_remove_region (kernel_region);

	gdt_register (gdtr);
	gdt_dump_entries ();

	tty_driver_vga_text.release (&tty_driver_vga_text);
}

// vim: set ts=4 sw=4 noet syn=c:
