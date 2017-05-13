// libk/include/collections/bintree.h

#ifndef IZIX_LIBK_COLLECTIONS_BINTREE_H
#define IZIX_LIBK_COLLECTIONS_BINTREE_H 1

#include <stddef.h>

typedef struct bintree_node_struct bintree_node_t;
typedef struct bintree_node_struct {
	bintree_node_t *low;
	bintree_node_t *high;
	bintree_node_t *parent;
	size_t orderby;
} bintree_node_t;

typedef struct bintree_struct {
	bintree_node_t *root;
	unsigned char last_rm : 1;
} bintree_t;

static inline bintree_t new_bintree () {
	bintree_t tree = {
		.root = NULL,
		.last_rm = 0
	};

	return tree;
}

static inline bintree_node_t new_bintree_node (size_t orderby) {
	bintree_node_t node = {
		.low = NULL,
		.high = NULL,
		.parent = NULL,
		.orderby = orderby
	};

	return node;
}

// Returns smallest/largest node in subtree.
#define MKBINTREE_SUB_HILO(suffix, link) \
static inline bintree_node_t *bintree_sub_##suffix (bintree_node_t *node) { \
	while (node->link) \
		node = node->link; \
\
	return node; \
}

MKBINTREE_SUB_HILO(min, low);
MKBINTREE_SUB_HILO(max, high);


// Returns smallest/largest node in tree, NULL if the tree is empty.
static inline bintree_node_t *bintree_min (bintree_t *tree) {
	if (!tree->root)
		return NULL;

	return bintree_sub_min (tree->root);
}
static inline bintree_node_t *bintree_max (bintree_t *tree) {
	if (!tree->root)
		return NULL;

	return bintree_sub_max (tree->root);
}

// Returns NULL if there is no adjacent node.
bintree_node_t *bintree_node_prev (bintree_node_t *);
bintree_node_t *bintree_node_next (bintree_node_t *);

// Returns a pointer to the node that matches orderby if it is found,
// Returns NULL if a matching node is not found.
bintree_node_t *bintree_sub_search (bintree_node_t *, size_t);

static inline bintree_node_t *bintree_search (bintree_t *tree, size_t orderby) {
	if (!tree->root)
		return NULL;

	return bintree_sub_search (tree->root, orderby);
}

// Returns NULL if node inserted successfully,
// otherwise returns node with conflicting .orderby field.
bintree_node_t *bintree_insert_node (bintree_t *, bintree_node_t *);

void bintree_remove_node_zero (bintree_t *, bintree_node_t *);
void bintree_remove_node_one (bintree_t *, bintree_node_t *);
void bintree_remove_node_two (bintree_t *, bintree_node_t *);

static inline void bintree_remove_node (bintree_t *tree, bintree_node_t *node) {
	const unsigned char children_count =
		(node->low  ? 1 : 0) +
		(node->high ? 1 : 0);

	switch (children_count) {
		case 0:
			bintree_remove_node_zero (tree, node);
			break;
		case 1:
			bintree_remove_node_one (tree, node);
			break;
		case 2:
			bintree_remove_node_two (tree, node);
			break;
	}
}

// Returns address of removed node if found,
// otherwise NULL if node can not be found.
static inline bintree_node_t *bintree_remove (bintree_t *tree, size_t orderby) {
	bintree_node_t *node = bintree_search (tree, orderby);

	if (!node || orderby != node->orderby)
		return NULL;

	bintree_remove_node (tree, node);

	return node;
}

#define TPL_BINTREE(name, type) \
typedef struct bintree_##name##_node_struct { \
	bintree_node_t node; \
	type data; \
} bintree_##name##_node_t; \
\
static inline bintree_##name##_node_t new_bintree_##name##_node ( \
		type data, \
		size_t orderby \
) { \
	bintree_##name##_node_t node = { \
		.node = new_bintree_node (orderby), \
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
static bintree_##name##_node_t *bintree_##name##_iterator_cur ( \
		bintree_##name##_iterator_t *this \
) { \
	return this->node; \
} \
\
static bintree_##name##_node_t *bintree_##name##_iterator_next ( \
		bintree_##name##_iterator_t *this \
) { \
	bintree_##name##_node_t *next = (bintree_##name##_node_t *)bintree_node_next ( \
		(bintree_node_t *)this->node); \
\
	if (NULL == next) \
		return NULL; \
\
	this->node = next; \
\
	return this->cur (this); \
} \
\
static bintree_##name##_node_t *bintree_##name##_iterator_prev ( \
		bintree_##name##_iterator_t *this \
) { \
	if (!this->node) \
		return NULL; \
\
	bintree_##name##_node_t *prev = (bintree_##name##_node_t *)bintree_node_prev ( \
		(bintree_node_t *)this->node); \
\
	if (NULL == prev) \
		return NULL; \
\
	this->node = prev; \
\
	return this->cur (this); \
} \
\
static void bintree_##name##_iterator_reset ( \
		bintree_##name##_iterator_t *this \
) { \
	while (this->prev (this)); \
} \
static inline bintree_##name##_iterator_t new_bintree_##name##_iterator ( \
		bintree_##name##_node_t *node \
) { \
	bintree_##name##_iterator_t iterator = { \
		.node = node, \
		.cur = bintree_##name##_iterator_cur, \
		.next = bintree_##name##_iterator_next, \
		.prev = bintree_##name##_iterator_prev, \
		.reset = bintree_##name##_iterator_reset \
	}; \
\
	return iterator; \
} \
\
typedef struct bintree_##name##_struct bintree_##name##_t; \
typedef struct bintree_##name##_sub_struct bintree_##name##_sub_t; \
typedef struct bintree_##name##_sub_struct { \
	bintree_##name##_t *tree; \
	bintree_##name##_node_t *sub_root; \
	bintree_##name##_node_t *(*min) (bintree_##name##_sub_t *); \
	bintree_##name##_node_t *(*max) (bintree_##name##_sub_t *); \
	bintree_##name##_node_t *(*search) (bintree_##name##_sub_t *, size_t); \
} bintree_##name##_sub_t; \
\
static bintree_##name##_node_t *bintree_##name##_sub_min ( \
		bintree_##name##_sub_t *this \
) { \
	if (!this->sub_root) \
		return NULL; \
\
	return (bintree_##name##_node_t *) \
		bintree_sub_min ((bintree_node_t *)this->sub_root); \
} \
\
static bintree_##name##_node_t *bintree_##name##_sub_max ( \
		bintree_##name##_sub_t *this \
) { \
	if (!this->sub_root) \
		return NULL; \
\
	return (bintree_##name##_node_t *) \
		bintree_sub_max ((bintree_node_t *)this->sub_root); \
} \
\
static bintree_##name##_node_t *bintree_##name##_sub_search ( \
		bintree_##name##_sub_t *this, \
		size_t orderby \
) { \
	if (!this->sub_root) \
		return NULL; \
\
	return (bintree_##name##_node_t *) \
		bintree_sub_search ((bintree_node_t *)this->sub_root, orderby); \
} \
\
static bintree_##name##_sub_t new_bintree_##name##_sub ( \
		bintree_##name##_t *this, \
		bintree_##name##_node_t *node \
) { \
	bintree_##name##_sub_t subtree = { \
		.tree = this, \
		.sub_root = node, \
		.min = bintree_##name##_sub_min, \
		.max = bintree_##name##_sub_max, \
		.search = bintree_##name##_sub_search \
	}; \
\
	return subtree; \
} \
typedef struct bintree_##name##_struct { \
	bintree_t tree; \
	bintree_##name##_node_t *(*min) (bintree_##name##_t *); \
	bintree_##name##_node_t *(*max) (bintree_##name##_t *); \
	bintree_##name##_node_t *(*search) (bintree_##name##_t *, size_t); \
	bintree_##name##_node_t *(*insert) (bintree_##name##_t *, bintree_##name##_node_t *); \
	void (*remove) (bintree_##name##_t *, bintree_##name##_node_t *); \
	bintree_##name##_iterator_t (*new_iterator) (bintree_##name##_t *); \
	bintree_##name##_sub_t (*new_subtree) (bintree_##name##_t *, bintree_##name##_node_t *); \
} bintree_##name##_t; \
\
\
static bintree_##name##_node_t *bintree_##name##_min ( \
		bintree_##name##_t *this \
) { \
	return (bintree_##name##_node_t *)bintree_min (&this->tree); \
} \
\
static bintree_##name##_node_t *bintree_##name##_max ( \
		bintree_##name##_t *this \
) { \
	return (bintree_##name##_node_t *) \
		bintree_max (&this->tree); \
} \
\
static bintree_##name##_node_t *bintree_##name##_search ( \
		bintree_##name##_t *this, \
		size_t orderby \
) { \
	return (bintree_##name##_node_t *) \
		bintree_search (&this->tree, orderby); \
} \
\
static bintree_##name##_node_t *bintree_##name##_insert ( \
		bintree_##name##_t *this, \
		bintree_##name##_node_t *node \
) { \
	return (bintree_##name##_node_t *) \
		bintree_insert_node (&this->tree, (bintree_node_t *)node); \
} \
\
static void bintree_##name##_remove ( \
		bintree_##name##_t *this, \
		bintree_##name##_node_t *node \
) { \
	bintree_remove_node (&this->tree, (bintree_node_t *)node); \
} \
\
static bintree_##name##_iterator_t new_bintree_##name##_iterator_from_tree ( \
		bintree_##name##_t *this \
) { \
	return new_bintree_##name##_iterator (this->min (this)); \
} \
static inline bintree_##name##_t new_bintree_##name () { \
	bintree_##name##_t tree = { \
		.tree = new_bintree (), \
		.min = bintree_##name##_min, \
		.max = bintree_##name##_max, \
		.search = bintree_##name##_search, \
		.insert = bintree_##name##_insert, \
		.remove = bintree_##name##_remove, \
		.new_iterator = new_bintree_##name##_iterator_from_tree, \
		.new_subtree = new_bintree_##name##_sub \
	}; \
\
	return tree; \
}

#endif

// vim: set ts=4 sw=4 noet syn=c:
