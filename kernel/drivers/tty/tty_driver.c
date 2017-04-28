// kernel/drivers/tty/tty_driver.c

#include <stdbool.h>

#include <tty/tty.h>
#include <tty/tty_driver.h>

void tty_safe_position_use_size (
		tty_driver_t *driver,
		tty_size_t size
) {
	driver->position.x %= size.width;
	driver->position.y %= size.height;
}

// Returns true if scroll is necessary.
bool tty_wrap_console_use_size (
		tty_driver_t *driver,
		tty_size_t size
) {
	if (driver->position.x >= size.width)
		driver->position.x = 0;

	if (driver->position.y >= size.height) {
		driver->position.y = 0;

		return true;
	}

	return false;
}

// vim: set ts=4 sw=4 noet syn=c:
