// kernel/mm/malloc.c

#include <stddef.h>

#include <mm/freemem.h>
#include <mm/malloc.h>

void *malloc (size_t size) {
	if (!size)
		return NULL;

	// Avoid fragmentation and add room for size_t
	size_t aligned_size = size + MALLOC_ALIGNMENT;
	if (aligned_size % MALLOC_ALIGNMENT)
		aligned_size += MALLOC_ALIGNMENT - size % MALLOC_ALIGNMENT;

	freemem_region_t suggestion = freemem_suggest (aligned_size, MALLOC_ALIGNMENT, 0);

	if (!suggestion.length)
		return NULL;  // ENOMEM

	bool remove_success = freemem_remove_region (suggestion);

	if (!remove_success)
		return NULL; // Out of internal freemem entries.

	*(size_t *)suggestion.p = suggestion.length;

	return suggestion.p + MALLOC_ALIGNMENT;
}

void free (void *ptr) {
	void *internal_ptr = ptr - MALLOC_ALIGNMENT;

	size_t size = *(size_t *)internal_ptr;

	freemem_region_t region = new_freemem_region (internal_ptr, size);

	freemem_add_region (region); // TODO: panic on failure.
}

// vim: set ts=4 sw=4 noet syn=c:
