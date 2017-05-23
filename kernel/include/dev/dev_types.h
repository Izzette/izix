// kernel/include/dev/dev_types.h

#ifndef IZIX_DEV_TYPES_H
#define IZIX_DEV_TYPES_H 1

#include <dev/dev_maj_types.h>
#include <dev/dev_min_types.h>

typedef struct dev_struct {
	dev_maj_t maj;
	dev_min_t min;
} dev_t;

#endif

// vim: set ts=4 sw=4 noet syn=c:
