// kernel/drivers/tty/tty_vga_text.c

#include <stddef.h>

#include <video/vga_text.h>
#include <video/vga_cursor.h>
#include <tty/tty.h>
#include <tty/tty_driver.h>

#define VGA_COLOR_FG_DEFAULT VGA_COLOR_LIGHT_GREY
#define VGA_COLOR_BG_DEFAULT VGA_COLOR_BLACK

typedef struct tty_vga_privdata_struct {
	vga_text_color_t vga_color;
	size_t virtual_length;
} tty_vga_privdata_t;

static tty_vga_privdata_t tty_vga_privdata = {
	.vga_color = {
		.fg = VGA_COLOR_FG_DEFAULT,
		.bg = VGA_COLOR_BG_DEFAULT
	},
	.virtual_length = 0
};

static bool tty_vga_init (tty_driver_t *);
static bool tty_vga_release (tty_driver_t *);
static void tty_vga_putc (tty_driver_t *, wchar_t);
static tty_size_t tty_vga_get_size (tty_driver_t *);
static void tty_vga_position_updates (tty_driver_t *);
static void tty_vga_color_updates (tty_driver_t *);

static tty_driver_t tty_vga_prototype = {
	.pimpl = NULL,  // Uninitialized
	.support_position = true,
	.support_color = true,
	.extra_char_width = 0,
	.term_descriptor = "VGA_TEXT",
	.errno = TTY_DRIVER_NOERR,
	.position = { .x = 0, .y = 0 },
	.fg_color = VGA_COLOR_FG_DEFAULT,
	.bg_color = VGA_COLOR_BG_DEFAULT,
	.init = tty_vga_init,
	.release = tty_vga_release,
	.putc = tty_vga_putc,
	.get_size = tty_vga_get_size,
	.position_updates = tty_vga_position_updates,
	.color_updates = tty_vga_color_updates
};

#pragma GCC diagnostic ignored "-Wunused-parameter"

static bool tty_vga_init (tty_driver_t *this) {
	vga_text_init ();

	return true;
}

static bool tty_vga_release (tty_driver_t *this) {
	// nop

	return true;
}

static void tty_vga_putc (tty_driver_t *this, wchar_t c) {
	tty_vga_privdata_t *privdata_ptr = (tty_vga_privdata_t *)this->pimpl;
	tty_size_t size = this->get_size (this);

	if ('\n' == c) {
		if (this->position.x || 0 == privdata_ptr->virtual_length) {
			this->position.x = 0;
			this->position.y += 1;
		}

		privdata_ptr->virtual_length = 0;

		tty_wrap_console_use_size (this, size);
		vga_cursor_set (this->position.x, this->position.y);

		return;
	}

	vga_text_entry_t entry = vga_text_mkentry (c, privdata_ptr->vga_color);

	vga_text_put_entry (entry, this->position.x, this->position.y);

	privdata_ptr->virtual_length += 1;

	this->position.x += 1;

	if (this->position.x >= size.width) {
		this->position.y += 1;

		tty_wrap_console_use_size (this, size);
	}

	vga_cursor_set (this->position.x, this->position.y);
}

static tty_size_t tty_vga_get_size (tty_driver_t *this) {
	const vga_text_size_t vga_text_size = vga_text_get_size ();

	tty_size_t tty_size = {
		.width = vga_text_size.width,
		.height = vga_text_size.height
	};

	return tty_size;
}

static void tty_vga_position_updates (tty_driver_t *this) {
	tty_safe_position (this);

	vga_cursor_set (this->position.x, this->position.y);
}

static void tty_vga_color_updates (tty_driver_t *this) {
	tty_vga_privdata_t *privdata_ptr = (tty_vga_privdata_t *)this->pimpl;

	if (VGA_COLOR_MIN > this->fg_color || VGA_COLOR_XMAX <= this->fg_color)
		this->fg_color = VGA_COLOR_FG_DEFAULT;

	if (VGA_COLOR_MIN > this->bg_color || VGA_COLOR_XMAX <= this->bg_color)
		this->bg_color = VGA_COLOR_BG_DEFAULT;

	privdata_ptr->vga_color.fg = this->fg_color;
	privdata_ptr->vga_color.bg = this->bg_color;
}

tty_driver_t get_tty_vga () {
	tty_driver_t tty_vga = tty_vga_prototype;
	tty_vga.pimpl = &tty_vga_privdata;

	return tty_vga;
}

#pragma GCC diagnostic pop

// vim: set ts=4 sw=4 noet syn=c:
