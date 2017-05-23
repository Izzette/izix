// kernel/include/dev/dev_min_types/dev_min_chardev_types.h

#ifndef IZIX_DEV_MIN_CHARDEV_TYPES_H
#define IZIX_DEV_MIN_CHARDEV_TYPES_H 1

typedef enum dev_min_chardev_enum {
	dev_min_chardev_vga_text = 0,
	dev_min_chardev_serial   = 1
} dev_min_chardev_t;

#endif

// vim: set ts=4 sw=4 noet syn=c:
