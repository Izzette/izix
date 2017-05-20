// libk/include/collections/bintree.h

#ifndef IZIX_LIBK_COLLECTIONS_BINTREE_H
#define IZIX_LIBK_COLLECTIONS_BINTREE_H 1

#include <stddef.h>

typedef struct __attribute__((packed)) bintree_zero_width_struct {
} bintree_zero_width_t;

typedef struct bintree_node_struct bintree_node_t;
typedef struct bintree_node_struct {
	bintree_node_t *low;
	bintree_node_t *high;
	bintree_node_t *parent;
	size_t orderby;
	bintree_zero_width_t data;
} bintree_node_t;

typedef struct bintree_iterator_struct bintree_iterator_t;
typedef struct bintree_iterator_struct {
	bintree_node_t *node;
	bintree_node_t *(*cur) (bintree_iterator_t *);
	bintree_node_t *(*next) (bintree_iterator_t *);
	bintree_node_t *(*prev) (bintree_iterator_t *);
	void (*reset) (bintree_iterator_t *);
} bintree_iterator_t;

typedef struct bintree_struct bintree_t;
typedef struct bintree_sub_struct bintree_sub_t;
typedef struct bintree_sub_struct {
	bintree_node_t *sub_root;
	bintree_node_t *(*min) (bintree_sub_t *);
	bintree_node_t *(*max) (bintree_sub_t *);
	bintree_node_t *(*search) (bintree_sub_t *, size_t);
} bintree_sub_t;

typedef struct bintree_struct bintree_t;
typedef struct bintree_struct {
	bintree_node_t *root;
	unsigned char last_rm : 1;
	bintree_node_t *(*min) (bintree_t *);
	bintree_node_t *(*max) (bintree_t *);
	bintree_node_t *(*search) (bintree_t *, size_t);
	bintree_node_t *(*insert) (bintree_t *, bintree_node_t *);
	void (*remove) (bintree_t *, bintree_node_t *);
	bintree_iterator_t (*new_iterator) (bintree_t *);
} bintree_t;

static inline bintree_node_t new_bintree_node (size_t orderby) {
	bintree_node_t node = {
		.low = NULL,
		.high = NULL,
		.parent = NULL,
		.orderby = orderby
	};

	return node;
}

bintree_iterator_t new_bintree_iterator (bintree_node_t *);
bintree_sub_t new_bintree_sub (bintree_node_t *);
bintree_t new_bintree ();

#define TPL_BINTREE(name, type) \
typedef struct bintree_##name##_node_struct bintree_##name##_node_t; \
typedef struct bintree_##name##_node_struct { \
	bintree_##name##_node_t *low; \
	bintree_##name##_node_t *high; \
	bintree_##name##_node_t *parent; \
	size_t orderby; \
	type data; \
} bintree_##name##_node_t; \
\
static inline bintree_##name##_node_t new_bintree_##name##_node ( \
		type data, \
		size_t orderby \
) { \
	bintree_##name##_node_t node = { \
		.low = NULL, \
		.high = NULL, \
		.parent = NULL, \
		.orderby = orderby, \
		.data = data \
	}; \
\
	return node; \
} \
\
typedef struct bintree_##name##_iterator_struct bintree_##name##_iterator_t; \
typedef struct bintree_##name##_iterator_struct { \
	bintree_##name##_node_t *node; \
	bintree_##name##_node_t *(*cur) (bintree_##name##_iterator_t *); \
	bintree_##name##_node_t *(*next) (bintree_##name##_iterator_t *); \
	bintree_##name##_node_t *(*prev) (bintree_##name##_iterator_t *); \
	void (*reset) (bintree_##name##_iterator_t *); \
} bintree_##name##_iterator_t; \
\
static inline bintree_##name##_iterator_t new_bintree_##name##_iterator ( \
		bintree_##name##_node_t *node \
) { \
	bintree_iterator_t generic_iterator = \
		new_bintree_iterator ((bintree_node_t *)node); \
\
	return *(bintree_##name##_iterator_t *)&generic_iterator; \
} \
\
typedef struct bintree_##name##_struct bintree_##name##_t; \
typedef struct bintree_##name##_sub_struct bintree_##name##_sub_t; \
typedef struct bintree_##name##_sub_struct { \
	bintree_##name##_node_t *sub_root; \
	bintree_##name##_node_t *(*min) (bintree_##name##_sub_t *); \
	bintree_##name##_node_t *(*max) (bintree_##name##_sub_t *); \
	bintree_##name##_node_t *(*search) (bintree_##name##_sub_t *, size_t); \
} bintree_##name##_sub_t; \
\
static inline bintree_##name##_sub_t new_bintree_##name##_sub (bintree_##name##_node_t *node) { \
	bintree_sub_t generic_subtree = new_bintree_sub ((bintree_node_t *)node); \
\
	return *(bintree_##name##_sub_t *)&generic_subtree; \
} \
typedef struct bintree_##name##_struct bintree_##name##_t; \
typedef struct bintree_##name##_struct { \
	bintree_##name##_node_t *root; \
	unsigned char last_rm : 1; \
	bintree_##name##_node_t *(*min) (bintree_##name##_t *); \
	bintree_##name##_node_t *(*max) (bintree_##name##_t *); \
	bintree_##name##_node_t *(*search) (bintree_##name##_t *, size_t); \
	bintree_##name##_node_t *(*insert) (bintree_##name##_t *, bintree_##name##_node_t *); \
	void (*remove) (bintree_##name##_t *, bintree_##name##_node_t *); \
	bintree_##name##_iterator_t (*new_iterator) (bintree_##name##_t *); \
} bintree_##name##_t; \
\
static inline bintree_##name##_t new_bintree_##name () { \
	bintree_t generic_tree = new_bintree (); \
\
	return *(bintree_##name##_t *)&generic_tree; \
}

#endif

// vim: set ts=4 sw=4 noet syn=c:
