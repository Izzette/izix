// kernel/kprint/kprint.c

#include <stdarg.h>

#include <string.h>
#include <format.h>

#include <tty/tty_driver.h>
#include <kprint/kprint.h>
#include <sched/kthread.h>

static char kprint_buffer[1024];

static volatile tty_driver_t kprint_tty_driver;

void set_kprint_tty_driver (tty_driver_t driver) {
	kthread_lock_task ();

	memcpy ((void *)&kprint_tty_driver, &driver, sizeof(tty_driver_t));

	kprintf ("kprint: Using TTY driver %s.\n", kprint_tty_driver.term_descriptor);

	kthread_unlock_task ();
}

tty_driver_t get_kprint_tty_driver () {
	kthread_lock_task ();

	tty_driver_t driver = kprint_tty_driver;

	kthread_unlock_task ();

	return driver;
}

void kputs (const char *str) {
	kthread_lock_task ();

	char c;

	while ((c = *str++))
		kprint_tty_driver.putc ((tty_driver_t *)&kprint_tty_driver, c);

	kthread_unlock_task ();
}

int kprintf (const char *format, ...) {
	kthread_lock_task ();

	va_list ap;

	va_start(ap, format);

	int ret = vsprintf (kprint_buffer, format, ap);

	va_end(ap);

	kputs (kprint_buffer);

	kthread_unlock_task ();

	return ret;
}

// vim: set ts=4 sw=4 noet syn=c:
