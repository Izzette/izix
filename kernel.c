/* Surely you will remove the processor conditionals and this comment
 * appropriately depending on whether or not you use C++.
 **/
#include <stddef.h>
#include <stdint.h>
#include <asm/io.h>
#include <izixboot/memmap.h>

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

size_t strlen (const char *);
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
	if (terminal_column == VGA_WIDTH)
		terminal_column = 0;

	if (terminal_row == VGA_HEIGHT)
		terminal_row = 0;
}

char *ulltoa (unsigned long value, char *result, int base) {
	// Worst case senario digits
	static char digits[8 * sizeof(unsigned long)];

	const char *value_map;

	static const char value_map_16[] = {
		'0', '1', '2', '3', '4', '5', '6', '7',
		'8', '9', 'a', 'b', 'c', 'd', 'e', 'f'
	};

	value_map = value_map_16;

	if (0 > base) {
		terminal_writestring("ERROR: ulltoa invalid base, negative!\n");
		return NULL;
	}

	if (16 < (size_t)base) {
		terminal_writestring("ERROR: ulltoa invalid base, greater than 16!\n");
		return NULL;
	}

	size_t result_index = 0;

	if (0 == value) {
		result[result_index++] = '0';
		result[result_index] = '\0';
		return result;
	}

	size_t digit_index = 0, digit_max;

	while (0 != value) {
		size_t digit;

		digit = value % base;
		value /= base;

		digits[digit_index++] = value_map[digit];
	}

	digit_max = digit_index;
	result_index = digit_index;

	result[result_index--] = '\0';

	for (digit_index = 0; digit_max > digit_index; ++digit_index)
		result[result_index--] = digits[digit_index];

	return result;
}

char *strpadl (char *str, char pad, size_t len) {
	size_t len_diff, cur_len, src_index, dst_index;

	cur_len = strlen(str);

	if (cur_len >= len)
		return str;

	len_diff = len - cur_len;

	for (src_index = cur_len - 1, dst_index = len - 1;
			len_diff <= dst_index;
			--src_index, --dst_index)
		str[dst_index] = str[src_index];

	for (dst_index = 0; len_diff > dst_index; ++dst_index)
		str[dst_index] = pad;

	str[len] = '\0';

	return str;
}

char *strcat (char *dest, const char *src) {
	char *cur = dest;

	while ('\0' != *cur)
		++cur;

	while ('\0' != *cur)
		*cur++ = *src++;

	*cur = '\0';

	return dest;
}

size_t strlen (const char *str) {
	size_t len = 0;
	while (str[len])
		len++;
	return len;
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
	if ('\n' == c) {
		terminal_column = 0;
		++terminal_row;

		wrap_console ();

		return;
	}

	terminal_putentryat (c, terminal_color, terminal_column, terminal_row);
	if (terminal_column == VGA_WIDTH) {
		++terminal_row;

		wrap_console ();
	}

	update_cursor ();

	++terminal_column;
}

void terminal_write (const char *data, size_t size) {
	for (size_t i = 0; i < size; i++)
		terminal_putchar (data[i]);
}

void terminal_writestring (const char *data) {
	size_t data_len = strlen (data);

	terminal_write (data, data_len);
}

void kernel_main (const e820_3x_entry_t *entries, e820_3x_entry_t *entries_end) {
	char buffer[9];
	size_t i, entries_length = entries_end - entries;

	terminal_initialize ();

	terminal_writestring ("izix command line: 0x");
	ulltoa ((unsigned long)entries, buffer, 16);
	strpadl (buffer, '0', 8);
	terminal_writestring (buffer);
	terminal_writestring (" 0x");
	ulltoa ((unsigned long)entries_end, buffer, 16);
	strpadl (buffer, '0', 8);
	terminal_writestring (buffer);
	terminal_writestring ("\n");

	terminal_writestring ("BIOS int 15h eax=e820h memory map:\n");

	for (i = 0; entries_length > i; ++i) {
		terminal_writestring (" BASE: 0x");
		ulltoa (entries[i].base, buffer, 16);
		strpadl (buffer, '0', 8);
		terminal_writestring (buffer);

		terminal_writestring (" LENGTH: 0x");
		ulltoa (entries[i].length, buffer, 16);
		strpadl (buffer, '0', 8);
		terminal_writestring (buffer);

		terminal_writestring (" TYPE: ");
		switch (entries[i].type) {
			case E820_TYPE_USABLE:
				terminal_writestring ("Usable");
				break;
			case E820_TYPE_RESERVED:
				terminal_writestring ("Reserved");
				break;
			case E820_TYPE_RECLAIM:
				terminal_writestring ("Reclaim");
				break;
			case E820_TYPE_NVS:
				terminal_writestring ("ACPI NVS");
				break;
			case E820_TYPE_BAD:
				terminal_writestring ("Bad");
				break;
			default:
				terminal_writestring ("Unknown?");
		}

		terminal_writestring (" XATTRS: [");
		if (0 == (E820_3X_XATTRS_DO_NOT_IGNORE & entries[i].xattrs))
			terminal_writestring (" Ignore ");
		else
			terminal_writestring (" Accept ");

		if (0 != (E820_3X_XATTRS_NON_VOLITALE & entries[i].xattrs))
			terminal_writestring (" Non-volitale ");
		else
			terminal_writestring (" Volitale ");

		terminal_writestring ("]\n");
	}
}

// vim: set ts=4 sw=4 noet syn=c:
