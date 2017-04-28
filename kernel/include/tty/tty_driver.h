// kernel/include/tty/tty_driver.h

#ifndef IZIX_TTY_DRIVER_H
#define IZIX_TTY_DRIVER_H 1

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#include <tty/tty.h>

enum tty_driver_error {
	TTY_DRIVER_NOERR = 0, // No error occurred.
	TTY_DRIVER_EINIT = 1  // Failed to intialize chardev for tty.
};

typedef struct tty_driver_struct tty_driver_t;

typedef struct tty_driver_struct {
// Private data.
	void *pimpl;
// Support get_position and set_position.
	const bool support_position;
// Support get_color and set_color.
	const bool support_color;
// Number of bytes greater than 1 in a character that is supported.
	const uint8_t extra_char_width : 2;
// String to human readable description.
	const char *term_descriptor;
// Last error that occurred.
	int errno;
// Initialize the terminal, returns true on success, false on failure.
	bool (*init) (tty_driver_t *this);
// Deinitialize the terminal and all its resources,
// returns true on success, false on failure.
	bool (*release) (tty_driver_t *this);
// Write one charcter, wchar_t will be masked by (1 << (extra_char_width + 2) - 1).
	void (*putc) (tty_driver_t *this, wchar_t c);
// If width or height are 0, there is no width or height respectively.
	tty_size_t (*get_size) ();
// Get the current position, NULL if support_position is false.
	tty_position_t (*get_position) (tty_driver_t *this);
// Set the position, NULL if support_position is false.
	void (*set_position) (tty_driver_t *this, tty_position_t);
// Get the current color, NULL if support_color is false.
	tty_color_t (*get_fg_color) (tty_driver_t *this);
	tty_color_t (*get_bg_color) (tty_driver_t *this);
// Set the color, NULL if support_color is false.
	void (*set_fg_color) (tty_driver_t *this, tty_color_t fg_color);
	void (*set_bg_color) (tty_driver_t *this, tty_color_t bg_color);
} tty_driver_t;

#endif

// vim: set ts=4 sw=4 noet syn=c:
