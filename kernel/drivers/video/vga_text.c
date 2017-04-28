// kernel/drivers/video/vga_text.c

#include <stddef.h>
#include <stdint.h>

#include <string.h>

#include <asm/io.h>
#include <video/vga_text.h>

typedef struct __attribute__((packed)) vga_text_entry_struct {
	char c;
	vga_text_color_t color;
} vga_text_entry_t;

static vga_text_size_t vga_text_size = {
	.width = 80,
	.height = 25
};
static vga_text_position_t vga_text_position = {
	.x = 0,
	.y = 0
};

static vga_text_color_t vga_text_color = {
	.fg = VGA_COLOR_LIGHT_GREY,
	.bg = VGA_COLOR_BLACK
};

static vga_text_entry_t *vga_text_buffer = (vga_text_entry_t *)0xb8000;

static inline void update_cursor () {
	size_t cursor_offset = (vga_text_position.y * 80) + vga_text_position.x;

	// cursor LOW port to vga INDEX register
	outb (0x0F, 0x3D4);
	outb ((unsigned char)(cursor_offset & 0xFF), 0x3D5);
	// cursor HIGH port to vga INDEX register
	outb (0x0E, 0x3D4);
	outb ((unsigned char)((cursor_offset>> 8) & 0xFF), 0x3D5);
}

static inline void wrap_console () {
	if (vga_text_position.x >= vga_text_size.width)
		vga_text_position.x = 0;

	if (vga_text_position.y >= vga_text_size.height)
		vga_text_position.y = 0;
}

static inline void vga_text_put_entry (
		char c,
		vga_text_color_t color,
		vga_text_position_t position
) {
	const size_t index = position.y * vga_text_size.width + position.x;

	const vga_text_entry_t entry = {
		.color = color,
		.c = c
	};

	vga_text_buffer[index] = entry;
}

void vga_text_init () {
	size_t x, y;

	for (y = 0; y < vga_text_size.height; y++) {
		for (x = 0; x < vga_text_size.width; x++) {
			const vga_text_position_t position = {
				.x = x,
				.y = y
			};

			vga_text_put_entry (' ', vga_text_color, position);
		}
	}

	update_cursor ();
}

vga_text_size_t vga_text_get_size () {
	return vga_text_size;
}

void vga_text_set_position (vga_text_position_t position) {
	position.x %= vga_text_size.width;
	position.y %= vga_text_size.height;
}

vga_text_position_t vga_text_get_position () {
	return vga_text_position;
}

void vga_text_putc (char c) {
	static size_t virtual_length = 0;

	if ('\n' == c) {
		if (vga_text_position.x || 0 == virtual_length) {
			vga_text_position.x = 0;
			++vga_text_position.y;

			wrap_console ();
		}

		virtual_length = 0;

		update_cursor ();

		return;
	}

	vga_text_put_entry (c, vga_text_color, vga_text_position);

	++virtual_length;

	if (++vga_text_position.x >= vga_text_size.width) {
		++vga_text_position.y;

		wrap_console ();
	}

	update_cursor ();
}

void vga_text_set_color (vga_text_color_t color) {
	vga_text_color = color;
}

vga_text_color_t vga_text_get_color () {
	return vga_text_color;
}

// vim: set ts=4 sw=4 noet syn=c:
