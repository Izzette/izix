// libk/collections/sparse_collection.c

#include <stddef.h>
#include <stdbool.h>
#include <limits.h>

#include <strings.h>
#include <string.h>
#include <collections/sparse_collection.h>

static int *sparse_collection_get_bit_int (size_t i, int *bitmap) {
	return bitmap + (i - 1) / (8 * sizeof(int));
}

static void sparse_collection_set_bit (size_t i, int *bitmap) {
	*sparse_collection_get_bit_int (i, bitmap) |= 1U << ((i - 1) % (8 * sizeof(int)));
}

static void sparse_collection_unset_bit (size_t i, int *bitmap) {
	*sparse_collection_get_bit_int (i, bitmap) &= ~(1U << ((i - 1) % (8 * sizeof(int))));
}

// Get bitmap length as a number of integers.
static size_t sparse_collection_get_bitmap_length (size_t elm_size, size_t length) {
	// Number of bytes of elements one int can hold.
	const size_t elmbytes_p_int = elm_size * 8 * sizeof(int);
	// Number of bytes that can be mapped by one int (including itself).
	const size_t bytes_p_int = elmbytes_p_int + sizeof(int);

	// The number of ints that will be "full" of elements.
	const size_t full_ints = length / bytes_p_int;
	// The number remaining bytes that can be salvaged.
	const size_t remaining_bytes = length % bytes_p_int;

	// If the (...) then we can't fit any more elements.
	if (remaining_bytes < sizeof(int) + elm_size)
		return full_ints;

	// We can partially utilize one more integer.
	return full_ints + 1;
}

// Get element array length as a number of integers.
static size_t sparse_collection_get_elm_arr_length (
		size_t bitmap_length,
		size_t elm_size,
		size_t length
) {
	// If there is no length to the bitmap, then we can't record whether any of the
	// elements are in use.
	if (0 == bitmap_length)
		return 0;

	// The number of bytes allocatable to elements.
	const size_t allocatable_bytes = length - sizeof(int) * bitmap_length;

	// Number of full elements that can be allocated.
	return allocatable_bytes / elm_size;
}

// Get bitmap pointer.
static int *sparse_collection_get_bitmap (size_t bitmap_length, void *data, size_t length) {
	return (int *)(data + length - sizeof(int) * bitmap_length);
}

// Get element array pointer.
static void *sparse_collection_get_elm_arr (void *data) {
	return data;
}

// Initialize the bitmap
static void sparse_collection_init_bitmap (int *bitmap, size_t elm_arr_length) {
	while (8 * sizeof(int) <= elm_arr_length) {
		*bitmap++ = UINT_MAX;

		elm_arr_length -= 8 * sizeof(int);
	}

	if (elm_arr_length)
		*bitmap = (1 << elm_arr_length) - 1;

	return;
}

// Return index of next set bit (starting at 1), zero if no bit is set.
static size_t sparse_collection_get (const sparse_collection_t *this) {
	size_t i;
	int submap = 0;

	for (i = 0; this->bitmap_length > i; ++i) {
		if (0 != this->bitmap[i]) {
			submap = this->bitmap[i];
			break;
		}
	}

	if (!submap)
		return 0;

	return ffs (submap) + 8 * i * sizeof(int);
}

// Allocated index (starting at 1).
static void *sparse_collection_alloc (sparse_collection_t *this, size_t i) {
	sparse_collection_unset_bit (i, this->bitmap);

	return this->elm_arr + this->elm_size * (i - 1);
}

// Returns true on success.
static bool sparse_collection_insert (sparse_collection_t *this, const void *elm) {
	const size_t i = this->get (this);
	if (!i)
		return false;

	void *const dest = this->alloc (this, i);
	memcpy (dest, elm, this->elm_size);

	return true;
}

static void sparse_collection_free (sparse_collection_t *this, const void *elm) {
	const size_t i = (elm - this->elm_arr) / this->elm_size + 1;

	sparse_collection_set_bit (i, this->bitmap);
}

sparse_collection_t new_sparse_collection (
		void *data, // Data pointer.
		size_t length, // Length of data in bytes.
		size_t elm_size // Size of one element in bytes.
) {
	const size_t bitmap_length =
		sparse_collection_get_bitmap_length (elm_size, length);
	const size_t elm_arr_length =
		sparse_collection_get_elm_arr_length (bitmap_length, elm_size, length);

	sparse_collection_t collection = {
		.bitmap = sparse_collection_get_bitmap (bitmap_length, data, length),
		.bitmap_length = bitmap_length,
		.elm_arr = sparse_collection_get_elm_arr (data),
		.elm_size = elm_size,
		.get = sparse_collection_get,
		.alloc = sparse_collection_alloc,
		.insert = sparse_collection_insert,
		.free = sparse_collection_free
	};

	sparse_collection_init_bitmap (collection.bitmap, elm_arr_length);

	return collection;
}

// vim: set ts=4 sw=4 noet syn=c:
