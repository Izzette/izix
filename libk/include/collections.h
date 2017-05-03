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

// Returns smallest/largest node in subtree.
bintree_node_t *bintree_sub_min (bintree_node_t *);
bintree_node_t *bintree_sub_max (bintree_node_t *);

// Returns smallest/largest node in tree, NULL if the tree is empty.
bintree_node_t *bintree_min (bintree_t *);
bintree_node_t *bintree_max (bintree_t *);

// Returns NULL if there is no adjacent node.
bintree_node_t *bintree_node_prev (bintree_node_t *);
bintree_node_t *bintree_node_next (bintree_node_t *);

// Returns a pointer to the node that matches orderby if it is found,
// Returns NULL if a matching node is not found.
bintree_node_t *bintree_search (bintree_t *, size_t);

// Returns NULL if node inserted successfully,
// otherwise returns node with conflicting .orderby field.
bintree_node_t *bintree_insert (bintree_t *, bintree_node_t *);

void bintree_remove_node (bintree_t *, bintree_node_t *);

// Returns address of removed node if found,
// otherwise NULL if node can not be found.
bintree_node_t *bintree_remove (bintree_t *, size_t);

#endif

// vim: set ts=4 sw=4 noet syn=c:
