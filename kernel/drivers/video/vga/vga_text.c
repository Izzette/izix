// kernel/drivers/video/vga/vga_text.c

#include <stddef.h>

#include <video/vga/vga_text.h>
#include <video/vga/vga_cursor.h>
#include <dev/dev_types.h>
#include <dev/dev_driver.h>
#include <mm/page.h>

static page_t *vga_text_next_page_mapping (dev_driver_t *, page_t *, bool *);
static dev_driver_t vga_text_driver = {
	.pimpl = NULL, // No need for pimpl (yet).
	.dev = {
		.maj = dev_maj_chardev,
		.min = 0 // Only ever one VGA device.
	},
	.next_page_mapping = vga_text_next_page_mapping
};

static vga_text_size_t vga_text_size = {
	.width = 80,
	.height = 25
};

static vga_text_entry_t *vga_text_buffer = (vga_text_entry_t *)0xb8000;

#pragma GCC diagnostic ignored "-Wunused-parameter"
static page_t *vga_text_next_page_mapping (
		dev_driver_t *this,
		page_t *last_page,
		bool *need_write
) {
	// VGA text buffer is 4000 bytes long, so it fits neatly in one page!
	switch ((size_t)last_page) {
		case (size_t)NULL:
			// We need to write this page!
			*need_write = true;
			return (page_t *)vga_text_buffer;
		default:
			return NULL;
	}
}
#pragma GCC diagnostic pop

void vga_text_init () {
	size_t x, y;
	vga_text_color_t color;
	vga_text_entry_t entry;

	color = vga_text_mkcolor (VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
	entry = vga_text_mkentry (' ', color);

	for (y = 0; y < vga_text_size.height; ++y)
		for (x = 0; x < vga_text_size.width; ++x)
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
	const size_t index = vga_text_size.width * y + x;

	vga_text_buffer[index] = entry;
}

void vga_text_scoll_one_line () {
	size_t dx, dy, sx, sy;

	for (dy = 0, sy = 1; sy < vga_text_size.height; ++dy, ++sy) {
		for (dx = 0, sx = 0; sx < vga_text_size.width; ++dx, ++sx) {
			size_t si = vga_text_size.width * sy + sx;
			vga_text_entry_t entry = vga_text_buffer[si];

			vga_text_put_entry (entry, dx, dy);
		}
	}

	vga_text_color_t color = vga_text_mkcolor (VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
	vga_text_entry_t entry = vga_text_mkentry (' ', color);

	for (dx = 0, dy = vga_text_size.height - 1; dx < vga_text_size.width; ++dx)
		vga_text_put_entry (entry, dx, dy);
}

dev_driver_t *vga_text_get_device_driver () {
	return &vga_text_driver;
}


// vim: set ts=4 sw=4 noet syn=c:
