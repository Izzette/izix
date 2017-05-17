// libk/include/collections/packed_list.h

#ifndef IZIX_LIBK_COLLECTIONS_PACKED_LIST_H
#define IZIX_LIBK_COLLECTIONS_PACKED_LIST_H 1

#include <stddef.h>
#include <stdbool.h>

typedef struct packed_list_struct {
	size_t count;
	size_t capacity;
	void *array;
	size_t elm_size;
} packed_list_t;

static inline void *packed_list_get (packed_list_t *list, size_t index) {
	return list->array + list->elm_size * index;
}

static inline void *packed_list_get_first (packed_list_t *list) {
	return packed_list_get (list, 0);
}

static inline void *packed_list_get_last (packed_list_t *list) {
	return packed_list_get (list, list->count - 1);
}

// Returns the current number of elements contained.
static inline size_t packed_list_count (packed_list_t *list) {
	return list->count;
}

// Returns the total number of elements that can be contained.
static inline size_t packed_list_capacity (packed_list_t *list) {
	return list->capacity;
}

// Returns the number of elements that can be added without overrunning.
static inline size_t packed_list_remaining (packed_list_t *list) {
	return packed_list_capacity (list) - packed_list_count (list);
}

// Returns the number of elements dropped, or zero if none.
size_t packed_list_resize (packed_list_t *, size_t);

bool packed_list_append (packed_list_t *, void *);
bool packed_list_remove (packed_list_t *, size_t);

typedef struct packed_list_iterator_struct {
	packed_list_t *list;
	size_t i;
} packed_list_iterator_t;

static inline void *packed_list_iterator_cur (packed_list_iterator_t *iterator) {
	if (iterator->i >= packed_list_count (iterator->list))
		return NULL;

	return iterator->list->array + iterator->i * iterator->list->elm_size;
}

static inline void *packed_list_iterator_next (packed_list_iterator_t *iterator) {
	iterator->i += 1;
	return packed_list_iterator_cur (iterator);
}

static inline void *packed_list_iterator_prev (packed_list_iterator_t *iterator) {
	iterator->i -= 1;
	return packed_list_iterator_cur (iterator);
}

static inline void packed_list_iterator_reset (packed_list_iterator_t *iterator) {
	iterator->i = 0;
}

#define TPL_PACKED_LIST(name, type) \
typedef struct packed_list_##name##_struct packed_list_##name##_t; \
typedef struct packed_list_##name##_struct { \
	packed_list_t internal_list; \
	bool (*pre_remove) (type *); \
	void (*rollback_remove) (type *); \
	bool (*post_add) (type *); \
	type *(*get) (packed_list_##name##_t *, size_t); \
	type *(*get_first) (packed_list_##name##_t *); \
	type *(*get_last) (packed_list_##name##_t *); \
	size_t (*count) (packed_list_##name##_t *); \
	size_t (*capacity) (packed_list_##name##_t *); \
	size_t (*remaining) (packed_list_##name##_t *); \
	size_t (*resize) (packed_list_##name##_t *, size_t); \
	bool (*append) (packed_list_##name##_t *, type); \
	bool (*remove) (packed_list_##name##_t *, size_t); \
	bool (*remove_elm) (packed_list_##name##_t *, type *); \
} packed_list_##name##_t; \
\
static type *packed_list_##name##_get ( \
		packed_list_##name##_t *this, \
		size_t index \
) { \
	return packed_list_get (&this->internal_list, index); \
} \
\
static type *packed_list_##name##_get_first ( \
		packed_list_##name##_t *this \
) { \
	return packed_list_get_first (&this->internal_list); \
} \
\
static type *packed_list_##name##_get_last ( \
		packed_list_##name##_t *this \
) { \
	return packed_list_get_last (&this->internal_list); \
} \
\
static size_t packed_list_##name##_count ( \
		packed_list_##name##_t *this \
) { \
	return packed_list_count (&this->internal_list); \
} \
\
static size_t packed_list_##name##_capacity ( \
		packed_list_##name##_t *this \
) { \
	return packed_list_remaining (&this->internal_list); \
} \
\
static size_t packed_list_##name##_remaining ( \
		packed_list_##name##_t *this \
) { \
	return packed_list_remaining (&this->internal_list); \
} \
\
static size_t packed_list_##name##_resize ( \
		packed_list_##name##_t *this, \
		size_t new_size \
) { \
	return packed_list_resize (&this->internal_list, new_size); \
} \
\
static bool packed_list_##name##_append ( \
		packed_list_##name##_t *this, \
		type elm_base \
) { \
	const bool append_success = packed_list_append (&this->internal_list, &elm_base); \
	if (!append_success) \
		return false; \
\
	return this->post_add (this->get_last (this)); \
} \
\
static bool packed_list_##name##_remove ( \
		packed_list_##name##_t *this, \
		size_t index \
) { \
	const bool is_last = this->get (this, index) == this->get_last (this); \
	const bool pre_remove_last_success = this->pre_remove (this->get_last (this)); \
	if (!pre_remove_last_success) \
		return false; \
\
	if (!is_last) { \
		const bool pre_remove_success = this->pre_remove (this->get (this, index)); \
		if (!pre_remove_success) { \
			this->rollback_remove (this->get_last (this)); \
			return false; \
		} \
	} \
\
	const bool remove_success = packed_list_remove (&this->internal_list, index); \
	if (!remove_success) { \
		this->rollback_remove (this->get_last (this)); \
		return false; \
	} \
\
	if (!is_last) \
		return this->post_add (this->get (this, index)); \
\
	return true; \
} \
\
static bool packed_list_##name##_remove_elm ( \
		packed_list_##name##_t *this, \
		type *elm \
) { \
	return this->remove (this, elm - (type *)this->internal_list.array); \
} \
\
static packed_list_##name##_t new_packed_list_##name ( \
		type *array, \
		size_t capacity, \
		bool (*pre_remove) (type *), \
		void (*rollback_remove) (type *), \
		bool (*post_add) (type *) \
) { \
	packed_list_##name##_t list = { \
		.internal_list = { \
			.count = 0, \
			.capacity = capacity, \
			.array = array, \
			.elm_size = sizeof(type) \
		}, \
		.pre_remove = pre_remove, \
		.rollback_remove = rollback_remove, \
		.post_add = post_add, \
		.get = packed_list_##name##_get, \
		.get_first = packed_list_##name##_get_first, \
		.get_last = packed_list_##name##_get_last, \
		.count = packed_list_##name##_count, \
		.capacity = packed_list_##name##_capacity, \
		.remaining = packed_list_##name##_remaining, \
		.resize = packed_list_##name##_resize, \
		.append = packed_list_##name##_append, \
		.remove = packed_list_##name##_remove, \
		.remove_elm = packed_list_##name##_remove_elm \
	}; \
\
	return list; \
} \
\
typedef struct packed_list_##name##_iterator_struct packed_list_##name##_iterator_t; \
typedef struct packed_list_##name##_iterator_struct { \
	packed_list_iterator_t internal_iterator; \
	type *(*cur) (packed_list_##name##_iterator_t *); \
	type *(*next) (packed_list_##name##_iterator_t *); \
	type *(*prev) (packed_list_##name##_iterator_t *); \
	void (*reset) (packed_list_##name##_iterator_t *); \
} packed_list_##name##_iterator_t; \
\
static type *packed_list_##name##_iterator_cur (packed_list_##name##_iterator_t *this) { \
	return packed_list_iterator_cur (&this->internal_iterator); \
} \
\
static type *packed_list_##name##_iterator_next (packed_list_##name##_iterator_t *this) { \
	return packed_list_iterator_next (&this->internal_iterator); \
} \
\
static type *packed_list_##name##_iterator_prev (packed_list_##name##_iterator_t *this) { \
	return packed_list_iterator_prev (&this->internal_iterator); \
} \
\
static void packed_list_##name##_iterator_reset (packed_list_##name##_iterator_t *this) { \
	return packed_list_iterator_reset (&this->internal_iterator); \
} \
\
static inline packed_list_##name##_iterator_t new_packed_list_##name##_iterator ( \
		packed_list_##name##_t *list \
) { \
	packed_list_##name##_iterator_t iterator = { \
		.internal_iterator = { \
			.list = &list->internal_list, \
			.i = 0 \
		}, \
		.cur = packed_list_##name##_iterator_cur, \
		.next = packed_list_##name##_iterator_next, \
		.prev = packed_list_##name##_iterator_prev, \
		.reset = packed_list_##name##_iterator_reset, \
	}; \
\
	return iterator; \
}

#endif

// vim: set ts=4 sw=4 noet syn=c:
