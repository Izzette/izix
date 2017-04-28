// kernel/drivers/tty/tty_vga_text.c

#include <stddef.h>
#include <stdint.h>

#include <string.h>

#include <video/vga_text.h>
#include <tty/tty.h>
#include <tty/tty_driver.h>

typedef struct tty_driver_vga_text_privdata_struct {
} tty_driver_vga_text_privdata_t;

static tty_driver_vga_text_privdata_t tty_driver_vga_text_privdata = {};

static bool tty_driver_vga_text_init (tty_driver_t *);
static bool tty_driver_vga_text_release (tty_driver_t *);
static void tty_driver_vga_text_putc (tty_driver_t *, wchar_t);
static tty_size_t tty_driver_vga_text_get_size (tty_driver_t *);
static tty_position_t tty_driver_vga_text_get_position (tty_driver_t *);
static void tty_driver_vga_text_set_position (tty_driver_t *, tty_position_t);
static void tty_driver_vga_text_set_fg_color (tty_driver_t *, tty_color_t);
static void tty_driver_vga_text_set_bg_color (tty_driver_t *, tty_color_t);
static tty_color_t tty_driver_vga_text_get_fg_color (tty_driver_t *);
static tty_color_t tty_driver_vga_text_get_bg_color (tty_driver_t *);

static tty_driver_t tty_driver_vga_text_prototype = {
	.pimpl = NULL,
	.support_position = true,
	.support_color = true,
	.extra_char_width = 0,
	.term_descriptor = "VGA_TEXT",
	.errno = TTY_DRIVER_NOERR,
	.init = tty_driver_vga_text_init,
	.release = tty_driver_vga_text_release,
	.putc = tty_driver_vga_text_putc,
	.get_size = tty_driver_vga_text_get_size,
	.get_position = tty_driver_vga_text_get_position,
	.set_position = tty_driver_vga_text_set_position,
	.get_fg_color = tty_driver_vga_text_get_fg_color,
	.get_bg_color = tty_driver_vga_text_get_bg_color,
	.set_fg_color = tty_driver_vga_text_set_fg_color,
	.set_bg_color = tty_driver_vga_text_set_bg_color
};

#pragma GCC diagnostic ignored "-Wunused-parameter"

static bool tty_driver_vga_text_init (tty_driver_t *this) {
	vga_text_init ();

	return true;
}

static bool tty_driver_vga_text_release (tty_driver_t *this) {
	// nop

	return true;
}

static void tty_driver_vga_text_putc (tty_driver_t *this, wchar_t c) {
	vga_text_putc ((char)c);
}

static tty_size_t tty_driver_vga_text_get_size (tty_driver_t *this) {
	const vga_text_size_t vga_text_size = vga_text_get_size ();

	tty_size_t tty_size = {
		.width = vga_text_size.width,
		.height = vga_text_size.height
	};

	return tty_size;
}

static tty_position_t tty_driver_vga_text_get_position (tty_driver_t *this) {
	const vga_text_position_t vga_text_position = vga_text_get_position ();

	tty_position_t tty_position = {
		.x = vga_text_position.x,
		.y = vga_text_position.y
	};

	return tty_position;
}

static void tty_driver_vga_text_set_position (tty_driver_t *this, tty_position_t tty_position) {
	vga_text_position_t vga_text_position = {
		.x = tty_position.x,
		.y = tty_position.y
	};

	vga_text_set_position (vga_text_position);
}

static void tty_driver_vga_text_set_fg_color (tty_driver_t *this, tty_color_t fg_color) {
	const vga_text_color_t vga_text_current_color = vga_text_get_color ();

	vga_text_color_t vga_text_new_color = {
		.bg = vga_text_current_color.bg,
		.fg = (uint8_t)fg_color
	};

	vga_text_set_color (vga_text_new_color);
}

static void tty_driver_vga_text_set_bg_color (tty_driver_t *this, tty_color_t bg_color) {
	const vga_text_color_t vga_text_current_color = vga_text_get_color ();

	vga_text_color_t vga_text_new_color = {
		.bg = (uint8_t)bg_color,
		.fg = vga_text_current_color.fg
	};

	vga_text_set_color (vga_text_new_color);
}

static tty_color_t tty_driver_vga_text_get_fg_color (tty_driver_t *this) {
	const vga_text_color_t vga_text_current_color = vga_text_get_color ();

	return (tty_color_t)vga_text_current_color.fg;
}

static tty_color_t tty_driver_vga_text_get_bg_color (tty_driver_t *this) {
	const vga_text_color_t vga_text_current_color = vga_text_get_color ();

	return (tty_color_t)vga_text_current_color.bg;
}

tty_driver_t get_tty_driver_vga_text () {
	tty_driver_t tty_driver_vga_text = tty_driver_vga_text_prototype;
	tty_driver_vga_text.pimpl = &tty_driver_vga_text_privdata;

	return tty_driver_vga_text;
}

#pragma GCC diagnostic pop

// vim: set ts=4 sw=4 noet syn=c:
