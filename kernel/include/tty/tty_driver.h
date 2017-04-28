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
// Supports position.
	const bool support_position;
// Supports cursor position.
	const bool support_cursor_position;
// Support colors.
	const bool support_color;
// Number of bytes greater than 1 in a character that is supported.
	const uint8_t extra_char_width : 2;
// String to human readable description.
	const char *term_descriptor;
// Last error that occurred.
	int errno;
// Current position.
	tty_position_t position;
// Current colors.
	tty_color_t fg_color;
	tty_color_t bg_color;
// Initialize the terminal, returns true on success, false on failure.
	bool (*init) (tty_driver_t *this);
// Deinitialize the terminal and all its resources,
// returns true on success, false on failure.
	bool (*release) (tty_driver_t *this);
// Write one charcter, wchar_t will be masked by (1 << (extra_char_width + 2) - 1).
	void (*putc) (tty_driver_t *this, wchar_t c);
// If width or height are 0, there is no width or height respectively.
	tty_size_t (*get_size) (tty_driver_t *this);
// Do all position related updates, should be called after changing position.
	void (*position_updates) (tty_driver_t *this);
// Do all color related updates, should be called after changing fg_color or bg_color.
	void (*color_updates) (tty_driver_t *this);
} tty_driver_t;

void tty_safe_position_use_size (tty_driver_t *driver, tty_size_t size);
bool tty_wrap_console_use_size (tty_driver_t *driver, tty_size_t size);

static inline void tty_safe_position (tty_driver_t *driver) {
	tty_size_t size = driver->get_size (driver);

	tty_safe_position_use_size (driver, size);
}

static inline void tty_wrap_console (tty_driver_t *driver) {
	tty_size_t size = driver->get_size (driver);

	tty_wrap_console_use_size (driver, size);
}

#endif

// vim: set ts=4 sw=4 noet syn=c:
