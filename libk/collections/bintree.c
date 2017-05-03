// libk/collections/bintree.c

#include <stddef.h>

#include <collections.h>

#include <mm/freemem.h>

#define MKBINTREE_NODE_ADJACENT(suffix, hilo, cmp, link) \
bintree_node_t *bintree_node_##suffix (bintree_node_t *node) { \
	if (node->link) \
		return bintree_sub_##hilo (node->link); \
\
	bintree_node_t *parent; \
\
	for (parent = node->parent; parent; parent = parent->parent) \
		if (node->orderby cmp parent->orderby) \
			return parent; \
\
	return NULL; \
}

MKBINTREE_NODE_ADJACENT(prev, max, >, low);
MKBINTREE_NODE_ADJACENT(next, min, <, high);

bintree_node_t *bintree_search (bintree_t *tree, size_t orderby) {
	if (!tree->root)
		return NULL;

	bintree_node_t *parent = tree->root;

	for (;;) {
		if (orderby > parent->orderby && parent->high)
			parent = parent->high;
		else if (orderby < parent->orderby && parent->low)
			parent = parent->low;
		else
			return parent;
	}

	return NULL;
}

bintree_node_t *bintree_insert (bintree_t *tree, bintree_node_t *node) {
	bintree_node_t *parent = bintree_search (tree, node->orderby);

	if (!parent) {
		tree->root = node;
		node->parent = NULL;
		return NULL;
	}

	if (node->orderby == parent->orderby)
		return parent;

	if (node->orderby < parent->orderby)
		parent->low = node;
	else
		parent->high = node;

	node->parent = parent;

	return NULL;
}

void bintree_remove_node_zero (bintree_t *tree, bintree_node_t *node) {
	if (node == tree->root)
		tree->root = NULL;
	else if (node == node->parent->high)
		node->parent->high = NULL;
	else
		node->parent->low = NULL;

	node->parent = NULL;
}

void bintree_remove_node_one (bintree_t *tree, bintree_node_t *node) {
	bintree_node_t *replacement;

	if (node->low) {
		replacement = node->low;
		node->low = NULL;
	} else {
		replacement = node->high;
		node->high = NULL;
	}

	if (node == tree->root)
		tree->root = replacement;
	else if (node == node->parent->high)
		node->parent->high = replacement;
	else
		node->parent->low = replacement;

	replacement->parent = node->parent;

	node->parent = NULL;
}

void bintree_remove_node_two (bintree_t *tree, bintree_node_t *node) {
	bintree_node_t *replacement;

	if (tree->last_rm)
		replacement = bintree_sub_max (node->low);
	else
		replacement = bintree_sub_min (node->high);

	tree->last_rm = (tree->last_rm + 1) % 2;

	// "replacment" is guerenteed to have zero or one children.
	if (!replacement->low && !replacement->high)
		bintree_remove_node_zero (tree, replacement);
	else
		bintree_remove_node_one (tree, replacement);

	if (node == tree->root)
		tree->root = replacement;
	else if (node == node->parent->high)
		node->parent->high = replacement;
	else
		node->parent->low = replacement;

	replacement->high = node->high;
	replacement->low = node->low;
	replacement->parent = node->parent;

	node->high = NULL;
	node->low = NULL;
	node->parent = NULL;
}

// vim: set ts=4 sw=4 noet syn=c:
