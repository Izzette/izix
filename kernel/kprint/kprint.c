// kernel/kprint/kprint.c

#include <stdarg.h>

#include <string.h>
#include <format.h>

#include <tty/tty_chardev_driver.h>
#include <kprint/kprint.h>
#include <sched/kthread.h>
#include <sched/mutex.h>

static mutex_t
	kprint_mutex_base,
	*kprint_mutex = &kprint_mutex_base;

static char kprint_buffer[1024];

static volatile tty_chardev_driver_t *kprint_tty_chardev_driver;

void kprint_init () {
	kprint_mutex_base = new_mutex ();
}

void set_kprint_tty_chardev_driver (volatile tty_chardev_driver_t *driver) {
	// Wrap assignment and retrival of tty_chardev_driver.
	mutex_lock (kprint_mutex);

	kprint_tty_chardev_driver = driver;

	mutex_lock(tty_chardev_driver_get_mutex (kprint_tty_chardev_driver));
	const char *term_descriptor = kprint_tty_chardev_driver->term_descriptor;
	mutex_release(tty_chardev_driver_get_mutex (kprint_tty_chardev_driver));

	mutex_release (kprint_mutex);

	kprintf ("kprint: Using TTY driver %s.\n", term_descriptor);
}

volatile tty_chardev_driver_t *get_kprint_tty_chardev_driver () {
	mutex_lock (kprint_mutex);
	volatile tty_chardev_driver_t *driver = kprint_tty_chardev_driver;
	mutex_release (kprint_mutex);

	return driver;
}

void kputs (const char *str) {
	char c;

	while ((c = *str++)) {
		mutex_lock(tty_chardev_driver_get_mutex (kprint_tty_chardev_driver));
		kprint_tty_chardev_driver->putc (
				(tty_chardev_driver_t *)kprint_tty_chardev_driver, c);
		mutex_release(tty_chardev_driver_get_mutex (kprint_tty_chardev_driver));
	}
}

int kprintf (const char *format, ...) {
	va_list ap;

	va_start(ap, format);

	// Wrap usage of kprint_buffer.
	mutex_lock (kprint_mutex);

	int ret = vsprintf (kprint_buffer, format, ap);

	va_end(ap);

	kputs (kprint_buffer);

	mutex_release (kprint_mutex);

	return ret;
}

// vim: set ts=4 sw=4 noet syn=c:
