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
static inline void *freemem_get_region_end (freemem_region_t region) {
	return region.p + region.length;
}

static inline bool freemem_consecutive_regions (
		freemem_region_t region1,
		freemem_region_t region2
) {
	freemem_region_t low_region, high_region;

	if (region1.p < region2.p) {
		low_region = region1;
		high_region = region2;
	} else {
		low_region = region2;
		high_region = region1;
	}

	return high_region.p == low_region.p + low_region.length;
}

static inline bool freemem_region_subset (
		freemem_region_t set,
		freemem_region_t subset
) {
	if (!(set.p <= subset.p))
		return false;
	if (!(freemem_get_region_end (set) >= freemem_get_region_end (subset)))
		return false;

	return true;
}

static inline freemem_region_t freemem_join_regions (
		freemem_region_t region1,
		freemem_region_t region2
) {
	void *new_p = (region1.p < region2.p) ? region1.p : region2.p;
	const size_t new_length = region1.length + region2.length;

	freemem_region_t joined_region = {
		.p = new_p,
		.length = new_length
	};

	return joined_region;
}

void freemem_init (void *, size_t);

bool freemem_add_region (freemem_region_t);
bool freemem_remove_region (freemem_region_t);

// The absolute value of offset must be less than alignment.
// Will return zeroed if a suggestion cannot be found.
freemem_region_t freemem_suggest (size_t, size_t, int);

#endif

// vim: set ts=4 sw=4 noet syn=c:
