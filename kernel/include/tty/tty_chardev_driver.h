// kernel/include/tty/tty_chardev_driver.h

#ifndef IZIX_TTY_DRIVER_H
#define IZIX_TTY_DRIVER_H 1

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#include <tty/tty.h>
#include <sched/mutex.h>
#include <dev/dev_driver.h>

enum tty_chardev_driver_error {
	TTY_CHARDEV_DRIVER_NOERR = 0, // No error occurred.
	TTY_CHARDEV_DRIVER_EINIT = 1  // Failed to intialize chardev for tty.
};

// TTY chardev driver.
typedef struct tty_chardev_driver_struct tty_chardev_driver_t;
typedef struct tty_chardev_driver_struct {
// Private data.
	void *pimpl;
// Device driver
	dev_driver_t *dev_driver;
// Supports position.
	bool support_position;
// Supports cursor position.
	bool support_cursor_position;
// Support colors.
	bool support_color;
// Number of bytes greater than 1 in a character that is supported.
	uint8_t extra_char_width : 2;
// String to human readable description.
	const char *term_descriptor;
// Last error that occurred.
	int errno;
// Current position.
	tty_position_t position;
// Current colors.
	tty_color_t fg_color;
	tty_color_t bg_color;
// Lock must be held by caller for every function except init and throughout
// color/position modification to update.
	mutex_t mutex_base;
// Initialize the terminal, returns true on success, false on failure.
	bool (*init) (tty_chardev_driver_t *this);
// De-initialize the terminal and all its resources,
// returns true on success, false on failure.
	bool (*release) (tty_chardev_driver_t *this);
// Write one character, wchar_t will be masked by (1 << (extra_char_width + 2) - 1).
	void (*putc) (tty_chardev_driver_t *this, wchar_t c);
// If width or height are 0, there is no width or height respectively.
	tty_size_t (*get_size) (tty_chardev_driver_t *this);
// Do all position related updates, should be called after changing position.
	void (*position_updates) (tty_chardev_driver_t *this);
// Do all color related updates, should be called after changing fg_color or bg_color.
	void (*color_updates) (tty_chardev_driver_t *this);
} tty_chardev_driver_t;

void tty_chardev_safe_position_use_size (tty_chardev_driver_t *driver, tty_size_t size);
bool tty_chardev_wrap_console_use_size (tty_chardev_driver_t *driver, tty_size_t size);

static inline void tty_chardev_safe_position (tty_chardev_driver_t *driver) {
	tty_size_t size = driver->get_size (driver);

	tty_chardev_safe_position_use_size (driver, size);
}

static inline void tty_chardev_wrap_console (tty_chardev_driver_t *driver) {
	tty_size_t size = driver->get_size (driver);

	tty_chardev_wrap_console_use_size (driver, size);
}

__attribute__((optimize("O3")))
static inline mutex_t *tty_chardev_driver_get_mutex (volatile tty_chardev_driver_t *driver) {
	return &driver->mutex_base;
}

#endif

// vim: set ts=4 sw=4 noet syn=c:
