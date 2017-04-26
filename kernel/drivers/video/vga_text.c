// kernel/drivers/video/vga_text.c

#include <asm/io.h>
#include <string.h>
#include <stddef.h>
#include <stdint.h>

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

static size_t VGA_WIDTH = 80;
static size_t VGA_HEIGHT = 25;

static size_t vga_text_row;
static size_t vga_text_column;
static uint8_t vga_text_color;
static uint16_t *vga_text_buffer;

static inline uint8_t vga_entry_color (enum vga_color fg, enum vga_color bg) {
	return fg | bg << 4;
}

static inline uint16_t vga_entry (unsigned char uc, uint8_t color) {
	return (uint16_t) uc | (uint16_t) color << 8;
}

static inline void update_cursor () {
	size_t position = (vga_text_row * 80) + vga_text_column;

	// cursor LOW port to vga INDEX register
	outb (0x0F, 0x3D4);
	outb ((unsigned char)(position & 0xFF), 0x3D5);
	// cursor HIGH port to vga INDEX register
	outb (0x0E, 0x3D4);
	outb ((unsigned char)((position >> 8) & 0xFF), 0x3D5);
}

static inline void wrap_console () {
	if (vga_text_column >= VGA_WIDTH)
		vga_text_column = 0;

	if (vga_text_row >= VGA_HEIGHT)
		vga_text_row = 0;
}

void vga_text_initialize () {
  size_t x, y;

	vga_text_row = 0;
	vga_text_column = 0;
	vga_text_color = vga_entry_color (VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
	vga_text_buffer = (uint16_t *)0xb8000;

	for (y = 0; y < VGA_HEIGHT; y++) {
		for (x = 0; x < VGA_WIDTH; x++) {
			const size_t index = y * VGA_WIDTH + x;

			vga_text_buffer[index] = vga_entry (' ', vga_text_color);
		}
	}

	update_cursor ();
}

void vga_text_setcolor (uint8_t color) {
	vga_text_color = color;
}

void vga_text_putentryat (char c, uint8_t color, size_t x, size_t y) {
	const size_t index = y * VGA_WIDTH + x;

	vga_text_buffer[index] = vga_entry (c, color);
}

void vga_text_putchar (char c) {
	static size_t virtual_length = 0;

	if ('\n' == c) {
		if (vga_text_column || 0 == virtual_length) {
			vga_text_column = 0;
			++vga_text_row;

			wrap_console ();
		}

		virtual_length = 0;

		update_cursor ();

		return;
	}

	vga_text_putentryat (c, vga_text_color, vga_text_column, vga_text_row);

	++virtual_length;

	if (++vga_text_column >= VGA_WIDTH) {
		++vga_text_row;

		wrap_console ();
	}

	update_cursor ();
}

void vga_text_write (const char *data, size_t size) {
  size_t i;

	for (i = 0; i < size; i++)
		vga_text_putchar (data[i]);
}

void vga_text_writestring (const char *data) {
  char c;

	while ((c = *data++))
		vga_text_putchar (c);
}

// vim: set ts=4 sw=4 noet syn=c:
