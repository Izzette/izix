// kernel/mm/malloc.c

#include <stddef.h>

#include <string.h>

#include <kpanic/kpanic.h>
#include <kprint/kprint.h>
#include <mm/freemem.h>
#include <mm/malloc.h>

static inline void *malloc_get_internal_ptr (void *ptr) {
	return ptr - MALLOC_ALIGNMENT;
}

static inline void *malloc_get_shared_ptr (void *internal_ptr) {
	return internal_ptr + MALLOC_ALIGNMENT;
}

static inline size_t malloc_get_allocated_size (void *internal_ptr) {
	return *(size_t *)internal_ptr;
}

static inline void malloc_set_allocated_size (void *internal_ptr, size_t internal_size) {
	*(size_t *)internal_ptr = internal_size;
}

static inline size_t malloc_get_internal_size (size_t size) {
	size_t aligned_size = size;
	if (aligned_size % MALLOC_ALIGNMENT)
		aligned_size += MALLOC_ALIGNMENT - size % MALLOC_ALIGNMENT;

	size_t internal_size = aligned_size + MALLOC_ALIGNMENT;

	return internal_size;
}

static inline size_t malloc_get_shared_size (size_t size) {
	return size - MALLOC_ALIGNMENT;
}

void *malloc (size_t size) {
	if (!size)
		return NULL;

	size_t internal_size = malloc_get_internal_size (size);

	// Avoid fragmentation and add room for size_t
	freemem_region_t suggestion = freemem_suggest (internal_size, MALLOC_ALIGNMENT, 0);

	if (!suggestion.length)
		return NULL;  // ENOMEM

	bool remove_success = freemem_remove_region (suggestion);

	if (!remove_success)
		return NULL; // Out of internal freemem entries.

	void *internal_ptr = suggestion.p;

	void *ptr = malloc_get_shared_ptr (internal_ptr);

	malloc_set_allocated_size (internal_ptr, suggestion.length);

	return ptr;
}

void *realloc (void *ptr, size_t size) {
	if (!size)
		return NULL;

	void *internal_ptr = malloc_get_internal_ptr (ptr);
	size_t internal_size = malloc_get_internal_size (size);

	size_t allocated_size = malloc_get_allocated_size (internal_ptr);

	if (internal_size == allocated_size)
		return ptr;

	if (internal_size < allocated_size) {
		size_t extra_size = allocated_size - internal_size;

		freemem_region_t extra_region = new_freemem_region (
				internal_ptr + extra_size, extra_size);

		bool readd_success = freemem_add_region (extra_region);
		if (!readd_success) {
			kputs (
				"mm/malloc: Failed to free supposedly allocated "
				"extra region during realloc!\n");
			kpanic ();
		}

		malloc_set_allocated_size (internal_ptr, internal_size);

		return ptr;
	}

	size_t additional_size = internal_size - allocated_size;
	freemem_region_t next_region = new_freemem_region (
			internal_ptr + allocated_size, additional_size);

	// Attempt to remove.
	bool remove_success = freemem_remove_region (next_region);

	if (remove_success) {
		malloc_set_allocated_size (internal_ptr, internal_size);

		return ptr;
	}

	// Try a new region.
	void *new_ptr = malloc (size);

	if (!new_ptr)
		return NULL;

	memcpy (new_ptr, ptr, malloc_get_shared_size (allocated_size));

	free (ptr);

	ptr = new_ptr;
	internal_ptr = malloc_get_internal_ptr (ptr);

	malloc_set_allocated_size (internal_ptr, internal_size);

	return ptr;
}

void free (void *ptr) {
	void *internal_ptr = malloc_get_internal_ptr (ptr);

	size_t size = malloc_get_allocated_size (internal_ptr);

	freemem_region_t region = new_freemem_region (internal_ptr, size);

	bool add_success = freemem_add_region (region);
	if (!add_success) {
		kputs ("mm/malloc: Failed to free supposedly allocated region!\n");
		kpanic ();
	}
}

// vim: set ts=4 sw=4 noet syn=c:
