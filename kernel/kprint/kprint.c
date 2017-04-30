// kernel/kprint/kprint.c

#include <stdarg.h>

#include <string.h>
#include <format.h>

#include <tty/tty_driver.h>
#include <kprint/kprint.h>

static tty_driver_t kprint_tty_driver;

void set_kprint_tty_driver (tty_driver_t driver) {
	memcpy (&kprint_tty_driver, &driver, sizeof(tty_driver_t));

	kprintf ("kprint: Using TTY driver %s.\n", kprint_tty_driver.term_descriptor);
}

tty_driver_t get_kprint_tty_driver () {
	return kprint_tty_driver;
}

void kputs (const char *str) {
	char c;

	while ((c = *str++))
		kprint_tty_driver.putc (&kprint_tty_driver, c);
}

int kprintf (const char *format, ...) {
	char buf[128];
	va_list ap;

	va_start(ap, format);

	int ret = vsprintf (buf, format, ap);

	va_end(ap);

	kputs (buf);

	return ret;
}

// vim: set ts=4 sw=4 noet syn=c:
