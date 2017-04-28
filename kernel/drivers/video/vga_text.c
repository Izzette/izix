// kernel/drivers/video/vga_text.c

#include <stddef.h>

#include <video/vga_text.h>
#include <video/vga_cursor.h>

static vga_text_size_t vga_text_size = {
	.width = 80,
	.height = 25
};

static vga_text_entry_t *vga_text_buffer = (vga_text_entry_t *)0xb8000;

void vga_text_init () {
	size_t x, y;
	vga_text_color_t color;
	vga_text_entry_t entry;

	color = vga_text_mkcolor (VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
	entry = vga_text_mkentry (' ', color);

	for (y = 0; y < vga_text_size.height; y++)
		for (x = 0; x < vga_text_size.width; x++)
			vga_text_put_entry (entry, x, y);

	vga_cursor_set (0, 0);
}

vga_text_size_t vga_text_get_size () {
	return vga_text_size;
}

void vga_text_put_entry (
		vga_text_entry_t entry,
		size_t x, size_t y
) {
	vga_text_size_t size = vga_text_get_size ();

	const size_t index = y * size.width + x;

	vga_text_buffer[index] = entry;
}

// vim: set ts=4 sw=4 noet syn=c:
