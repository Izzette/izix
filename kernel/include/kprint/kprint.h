// kernel/include/kprint/kprint.h

#ifndef IZIX_KPRINT_H
#define IZIX_KPRINT_H 1

#include <tty/tty_driver.h>

void set_kprint_tty_driver (tty_driver_t);
tty_driver_t get_kprint_tty_driver ();

void kputs (const char *);
int kprintf (const char *, ...)
	__attribute__ ((format (printf, 1, 2)));

#endif

// vim: set ts=4 sw=4 noet syn=c:
