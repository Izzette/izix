// kernel/include/video/vga_text.h

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

typedef struct vga_text_position_struct {
	size_t x;
	size_t y;
} vga_text_position_t;

typedef struct vga_text_size_struct {
	size_t width;
	size_t height;
} vga_text_size_t;

typedef struct __attribute__((packed)) vga_text_color_struct {
	uint8_t fg : 4;
	uint8_t bg : 4;
} vga_text_color_t;

void vga_text_init ();
vga_text_size_t vga_text_get_size ();
void vga_text_set_position (vga_text_position_t);
vga_text_position_t vga_text_get_position ();
void vga_text_putc (char);
void vga_text_set_color (vga_text_color_t);
vga_text_color_t vga_text_get_color ();

// vim: set ts=4 sw=4 noet syn=c:
