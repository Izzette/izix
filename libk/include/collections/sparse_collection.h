// libk/include/collections/sparse_collection.h

#ifndef IZIX_LIBK_COLLECTIONS_SPARSE_COLLECTION_H
#define IZIX_LIBK_COLLECTIONS_SPARSE_COLLECTION_H 1

#include <stddef.h>
#include <stdbool.h>

#include <attributes.h>

typedef struct sparse_collection_struct sparse_collection_t;
typedef struct sparse_collection_struct {
	int *bitmap;
	size_t bitmap_length; // Length of bitmap in sizeof(int).
	void *elm_arr;
	size_t elm_size; // Size of element.
	size_t (*get) (const sparse_collection_t *);
	void *(*alloc) (sparse_collection_t *, size_t);
	bool (*insert) (sparse_collection_t *, const void *);
	void (*free) (sparse_collection_t *, const void *);
} sparse_collection_t;

sparse_collection_t new_sparse_collection (void *, size_t, size_t);

#define TPL_SPARSE_COLLECTION(name, type) \
typedef struct sparse_collection_##name##_struct sparse_collection_##name##_t; \
typedef struct sparse_collection_##name##_struct { \
	int *bitmap; \
	size_t bitmap_length; \
	type *elm_arr; \
	size_t elm_size; \
	size_t (*get) (const sparse_collection_##name##_t *); \
	void *(*alloc) (sparse_collection_##name##_t *, size_t); \
	bool (*insert) (sparse_collection_##name##_t *, const type *); \
	void (*free) (sparse_collection_##name##_t *, const void *); \
} MAY_ALIAS sparse_collection_##name##_t; \
\
static inline sparse_collection_##name##_t new_sparse_collection_##name ( \
	void *data, \
	size_t length \
) { \
	sparse_collection_t generic_collection = new_sparse_collection ( \
		data, \
		length, \
		sizeof(type)); \
\
	return *(sparse_collection_##name##_t *)&generic_collection; \
}

#endif

// vim: set ts=4 sw=4 noet syn=c:
