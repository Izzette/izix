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

// Exclusive
static inline void *freemem_region_end (freemem_region_t region) {
	return region.p + region.length;
}

void freemem_init (void *, size_t);

bool freemem_add_region (freemem_region_t);
bool freemem_remove_region (freemem_region_t);
// The absolute value of offset must be less than alignment.
// Will return zeroed if a region cannot be found.
freemem_region_t freemem_alloc (size_t, size_t, int);

#endif

// vim: set ts=4 sw=4 noet syn=c:
