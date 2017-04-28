// kernel/include/tty/tty.h

#ifndef IZIX_TTY_H
#define IZIX_TTY_H 1

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

typedef int tty_color_t;

typedef struct tty_position_struct {
	size_t x;
	size_t y;
} tty_position_t;

typedef struct tty_size_struct {
	size_t width;
	size_t height;
} tty_size_t;

#endif

// vim: set ts=4 sw=4 noet syn=c:
