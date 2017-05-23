// kernel/drivers/video/vga/vga_cursor.c

#include <stddef.h>

#include <asm/io.h>

#include <video/vga/vga_text.h>

void vga_cursor_set (size_t x, size_t y) {
	vga_text_size_t size = vga_text_get_size ();

	size_t cursor_offset = size.width * y + x;

	// cursor LOW port to vga INDEX register
	outb (0x0F, 0x3D4);
	outb ((unsigned char)(cursor_offset & 0xFF), 0x3D5);
	// cursor HIGH port to vga INDEX register
	outb (0x0E, 0x3D4);
	outb ((unsigned char)((cursor_offset >> 8) & 0xFF), 0x3D5);
}

// vim: set ts=4 sw=4 noet syn=c:
