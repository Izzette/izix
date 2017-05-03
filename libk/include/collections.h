// libk/include/collections.h

#ifndef IZIX_LIBK_COLLECTIONS_H
#define IZIX_LIBK_COLLECTIONS_H 1

#include <stddef.h>

typedef struct bintree_node_struct bintree_node_t;

typedef struct bintree_node_struct {
	bintree_node_t *low;
	bintree_node_t *high;
	bintree_node_t *parent;
	size_t orderby;
	void *data;
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

static inline bintree_node_t new_bintree_node (size_t orderby, void *data) {
	bintree_node_t node = {
		.low = NULL,
		.high = NULL,
		.parent = NULL,
		.orderby = orderby,
		.data = data
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
bintree_node_t *bintree_search (bintree_t *, size_t);

// Returns NULL if node inserted successfully,
// otherwise returns node with conflicting .orderby field.
bintree_node_t *bintree_insert (bintree_t *, bintree_node_t *);

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


#endif

// vim: set ts=4 sw=4 noet syn=c:
