// kernel/include/kprint/kprint.h

#ifndef IZIX_KPRINT_H
#define IZIX_KPRINT_H 1

#include <attributes.h>

#include <tty/tty_chardev_driver.h>

void set_kprint_tty_chardev_driver (volatile tty_chardev_driver_t *);
volatile tty_chardev_driver_t *get_kprint_tty_chardev_driver ();

void kputs (const char *);
int kprintf (const char *, ...)
	FORMAT(printf, 1, 2);

#endif

// vim: set ts=4 sw=4 noet syn=c:
