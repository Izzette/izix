// kernel/include/video/vga_text.h

void vga_text_initialize ();
void vga_text_setcolor (uint8_t);
void vga_text_putentryat (char, uint8_t, size_t, size_t);
void vga_text_putchar (char);
void vga_text_write (const char *, size_t);
void vga_text_writestring (const char *);

// vim: set ts=4 sw=4 noet syn=c:
