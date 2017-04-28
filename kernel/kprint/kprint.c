// kernel/kprint/kprint.c

#include <string.h>

#include <tty/tty_driver.h>

static tty_driver_t kprint_tty_driver;

void set_kprint_tty_driver (tty_driver_t driver) {
	memcpy (&kprint_tty_driver, &driver, sizeof(tty_driver_t));
}

tty_driver_t get_kprint_tty_driver () {
	return kprint_tty_driver;
}

void kputs (const char *str) {
	char c;

	while ((c = *str++))
		kprint_tty_driver.putc (&kprint_tty_driver, c);
}

// vim: set ts=4 sw=4 noet syn=c:
