// kernel/include/video/vga/vga_text.h

#ifndef IZIX_VGA_TEXT_H
#define IZIX_VGA_TEXT_H 1

#include <stddef.h>
#include <stdint.h>

#include <attributes.h>

#include <dev/dev_driver.h>

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

#define VGA_COLOR_MIN 0
#define VGA_COLOR_XMAX 16

typedef struct vga_text_size_struct {
	size_t width;
	size_t height;
} vga_text_size_t;

typedef struct PACKED vga_text_color_struct {
	uint8_t fg : 4;
	uint8_t bg : 4;
} vga_text_color_t;

typedef struct PACKED vga_text_entry_struct {
	char c;
	vga_text_color_t color;
} vga_text_entry_t;

void vga_text_init ();
vga_text_size_t vga_text_get_size ();
// WARN: safe inputs are assumed.
void vga_text_put_entry (vga_text_entry_t, size_t, size_t);
void vga_text_scoll_one_line ();
dev_driver_t *vga_text_get_device_driver ();

// WARN: safe inputs are assumed.
static inline vga_text_color_t vga_text_mkcolor (uint8_t fg, uint8_t bg) {
	vga_text_color_t color = {
		.fg = fg,
		.bg = bg
	};

	return color;
}

// WARN: safe inputs are assumed.
static inline vga_text_entry_t vga_text_mkentry (char c, vga_text_color_t color) {
	vga_text_entry_t entry = {
		.c = c,
		.color = color
	};

	return entry;
}

#endif

// vim: set ts=4 sw=4 noet syn=c:
