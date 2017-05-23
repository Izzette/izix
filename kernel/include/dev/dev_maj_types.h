// kernel/include/dev/dev_maj_types.h

#ifndef IZIX_DEV_MAJ_TYPES_H
#define IZIX_DEV_MAJ_TYPES_H 1

typedef enum dev_maj_enum {
	dev_maj_tty     = 1,
	dev_maj_chardev = 2,
	dev_maj_arch    = 127 // Architecture specific devices
} dev_maj_t;

#endif

// vim: set ts=4 sw=4 noet syn=c:
