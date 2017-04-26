/* Surely you will remove the processor conditionals and this comment
 * appropriately depending on whether or not you use C++.
 **/
#include <stddef.h>
#include <stdint.h>
#include <asm/io.h>
#include <string.h>
#include <format.h>
#include <izixboot/memmap.h>
#include <izixboot/gdt.h>
#include <izixboot/gdt32.h>

/* Hardware text mode color constants. */
enum vga_color {
	VGA_COLOR_BLACK = 0,
	VGA_COLOR_BLUE = 1,
	VGA_COLOR_GREEN = 2,
	VGA_COLOR_CYAN = 3,
	VGA_COLOR_RED = 4,
	VGA_COLOR_MAGENTA = 5,
	VGA_COLOR_BROWN = 6,
	VGA_COLOR_LIGHT_GREY = 7,
	VGA_COLOR_DARK_GREY = 8,
	VGA_COLOR_LIGHT_BLUE = 9,
	VGA_COLOR_LIGHT_GREEN = 10,
	VGA_COLOR_LIGHT_CYAN = 11,
	VGA_COLOR_LIGHT_RED = 12,
	VGA_COLOR_LIGHT_MAGENTA = 13,
	VGA_COLOR_LIGHT_BROWN = 14,
	VGA_COLOR_WHITE = 15,
};

static const size_t VGA_WIDTH = 80;
static const size_t VGA_HEIGHT = 25;

size_t terminal_row;
size_t terminal_column;
uint8_t terminal_color;
uint16_t *terminal_buffer;

void terminal_writestring (const char *);

static inline uint8_t vga_entry_color (enum vga_color fg, enum vga_color bg) {
	return fg | bg << 4;
}

static inline uint16_t vga_entry (unsigned char uc, uint8_t color) {
	return (uint16_t) uc | (uint16_t) color << 8;
}

static inline void update_cursor () {
	size_t position = (terminal_row * 80) + terminal_column;

	// cursor LOW port to vga INDEX register
	outb (0x0F, 0x3D4);
	outb ((unsigned char)(position & 0xFF), 0x3D5);
	// cursor HIGH port to vga INDEX register
	outb (0x0E, 0x3D4);
	outb ((unsigned char)((position >> 8) & 0xFF), 0x3D5);
}

static inline void wrap_console () {
	if (terminal_column >= VGA_WIDTH)
		terminal_column = 0;

	if (terminal_row >= VGA_HEIGHT)
		terminal_row = 0;
}

void terminal_initialize (void) {
	terminal_row = 0;
	terminal_column = 0;
	terminal_color = vga_entry_color (VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
	terminal_buffer = (uint16_t*) 0xB8000;
	for (size_t y = 0; y < VGA_HEIGHT; y++) {
		for (size_t x = 0; x < VGA_WIDTH; x++) {
			const size_t index = y * VGA_WIDTH + x;
			terminal_buffer[index] = vga_entry (' ', terminal_color);
		}
	}

	update_cursor ();
}

void terminal_setcolor (uint8_t color) {
	terminal_color = color;
}

void terminal_putentryat (char c, uint8_t color, size_t x, size_t y) {
	const size_t index = y * VGA_WIDTH + x;
	terminal_buffer[index] = vga_entry (c, color);
}

void terminal_putchar (char c) {
	static size_t virtual_length = 0;

	if ('\n' == c) {
		if (terminal_column || 0 == virtual_length) {
			terminal_column = 0;
			++terminal_row;

			wrap_console ();
		}

		virtual_length = 0;

		update_cursor ();

		return;
	}

	terminal_putentryat (c, terminal_color, terminal_column, terminal_row);

	++virtual_length;

	if (++terminal_column >= VGA_WIDTH) {
		++terminal_row;

		wrap_console ();
	}

	update_cursor ();
}

void terminal_write (const char *data, size_t size) {
	for (size_t i = 0; i < size; i++)
		terminal_putchar (data[i]);
}

void terminal_writestring (const char *data) {
	size_t data_len = strlen (data);

	terminal_write (data, data_len);
}

__attribute__((force_align_arg_pointer))
void kernel_main (const uint32_t entry_count_u32, const uint32_t entries_u32, const uint32_t gdtr_u32) {
#pragma GCC diagnostic ignored "-Wint-to-pointer-cast"
	const e820_3x_entry_t *entries = (e820_3x_entry_t *)entries_u32;
#pragma GCC diagnostic pop
	const size_t entry_count = (size_t)entry_count_u32;
#pragma GCC diagnostic ignored "-Wint-to-pointer-cast"
	const gdt_register_t *gdtr = (gdt_register_t *)gdtr_u32;
#pragma GCC diagnostic pop

	char buffer[17];
	size_t i;

	terminal_initialize ();

	terminal_writestring ("Kernel command line: entry_count=");
	ulltoa ((unsigned long)entry_count_u32, buffer, 10);
	terminal_writestring (buffer);
	terminal_writestring (" entries=0x");
	ulltoa ((unsigned long)entries_u32, buffer, 16);
	strpadl (buffer, '0', 8);
	terminal_writestring (buffer);
	terminal_writestring (" gdtr=0x");
	ulltoa ((unsigned long)gdtr_u32, buffer, 16);
	strpadl (buffer, '0', 8);
	terminal_writestring (buffer);
	terminal_writestring ("\n");

	terminal_writestring ("e820: BIOS-provided physical RAM map:\n");

	for (i = 0; entry_count > i; ++i) {
		if (0 == entries[i].length)
			continue;
		if (0 == (E820_3X_XATTRS_DO_NOT_IGNORE & entries[i].xattrs))
			continue;

		terminal_writestring ("BIOS-e820: [mem ");

		terminal_writestring ("0x");
		ulltoa (entries[i].base, buffer, 16);
		strpadl (buffer, '0', 16);
		terminal_writestring (buffer);

		terminal_writestring ("-0x");
		ulltoa (entries[i].base + entries[i].length - 1, buffer, 16);
		strpadl (buffer, '0', 16);
		terminal_writestring (buffer);

		terminal_writestring ("] ");

		switch (entries[i].type) {
			case E820_TYPE_USABLE:
				terminal_writestring ("usable");
				break;
			case E820_TYPE_RESERVED:
				terminal_writestring ("reserved");
				break;
			case E820_TYPE_RECLAIM:
				terminal_writestring ("ACPI data");
				break;
			case E820_TYPE_NVS:
				terminal_writestring ("ACPI NVS");
				break;
			case E820_TYPE_BAD:
				terminal_writestring ("bad");
				break;
			default:
				terminal_writestring ("unknown");
		}

		if (0 != (E820_3X_XATTRS_NON_VOLITALE & entries[i].xattrs))
			terminal_writestring (" persistent");

		terminal_writestring ("\n");
	}
}

// vim: set ts=4 sw=4 noet syn=c:
