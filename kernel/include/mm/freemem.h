// kernel/include/mm/freemem.h

#ifndef IZIX_FREEMEM_H
#define IZIX_FREEMEM_H 1

#include <stddef.h>
#include <stdbool.h>

typedef struct freemem_region_struct {
	void *p;
	size_t length;
} freemem_region_t;

static inline freemem_region_t new_freemem_region (void *p, size_t length) {
	freemem_region_t region = {
		.p = p,
		.length = length
	};

	return region;
}

void freemem_init (void *, size_t);
bool freemem_add_region (freemem_region_t);

#endif

// vim: set ts=4 sw=4 noet syn=c:
