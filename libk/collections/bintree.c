// libk/collections/bintree.c

#include <stddef.h>

#include <collections/bintree.h>

static bintree_node_t *bintree_node_next (bintree_node_t *node) {
	bintree_sub_t
		subtree_base,
		*subtree = &subtree_base;

	if (node->high) {
		subtree_base = new_bintree_sub (node->high);

		return subtree->min (subtree);
	}

	bintree_node_t *parent;

	for (parent = node->parent; parent; parent = parent->parent)
		if (node->orderby < parent->orderby)
			return parent;

	return NULL;
}

static bintree_node_t *bintree_node_prev (bintree_node_t *node) {
	bintree_sub_t
		subtree_base,
		*subtree = &subtree_base;

	if (node->low) {
		subtree_base = new_bintree_sub (node->low);

		return subtree->max (subtree);
	}

	bintree_node_t *parent;

	for (parent = node->parent; parent; parent = parent->parent)
		if (node->orderby > parent->orderby)
			return parent;

	return NULL;
}

static void bintree_remove_node_zero (bintree_t *tree, bintree_node_t *node) {
	if (node == tree->root)
		tree->root = NULL;
	else if (node == node->parent->high)
		node->parent->high = NULL;
	else
		node->parent->low = NULL;

	node->parent = NULL;
}

static void bintree_remove_node_one (bintree_t *tree, bintree_node_t *node) {
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

static void bintree_remove_node_two (bintree_t *this, bintree_node_t *node) {
	bintree_iterator_t
		iterator_base,
		*iterator = &iterator_base;
	bintree_node_t *replacement;

	iterator_base = new_bintree_iterator (node);

	if (this->last_rm)
		replacement = iterator->prev (iterator);
	else
		replacement = iterator->next (iterator);

	this->last_rm = (this->last_rm + 1) % 2;

	// "replacment" is guerenteed to have zero or one children.
	if ((!replacement->low) && (!replacement->high))
		bintree_remove_node_zero (this, replacement);
	else
		bintree_remove_node_one (this, replacement);

	if (node == this->root)
		this->root = replacement;
	else if (node == node->parent->high)
		node->parent->high = replacement;
	else
		node->parent->low = replacement;

	replacement->high = node->high;
	if (replacement->high)
		replacement->high->parent = replacement;
	replacement->low = node->low;
	if (replacement->low)
		replacement->low->parent = replacement;
	replacement->parent = node->parent;

	node->high = NULL;
	node->low = NULL;
	node->parent = NULL;
}

static bintree_node_t *bintree_iterator_cur (
		bintree_iterator_t *this
) {
	return this->node;
}

static bintree_node_t *bintree_iterator_next (
		bintree_iterator_t *this
) {
	bintree_node_t *next = (bintree_node_t *)bintree_node_next (
		(bintree_node_t *)this->node);

	if (NULL == next)
		return NULL;

	this->node = next;

	return this->cur (this);
}

static bintree_node_t *bintree_iterator_prev (
		bintree_iterator_t *this
) {
	if (!this->node)
		return NULL;

	bintree_node_t *prev = (bintree_node_t *)bintree_node_prev (
		(bintree_node_t *)this->node);

	if (NULL == prev)
		return NULL;

	this->node = prev;

	return this->cur (this);
}

static void bintree_iterator_reset (
		bintree_iterator_t *this
) {
	while (this->prev (this));
}

bintree_iterator_t new_bintree_iterator (
		bintree_node_t *node
) {
	bintree_iterator_t iterator = {
		.node = node,
		.cur = bintree_iterator_cur,
		.next = bintree_iterator_next,
		.prev = bintree_iterator_prev,
		.reset = bintree_iterator_reset
	};

	return iterator;
}

// Returns smallest/largest node in subtree.
static bintree_node_t *bintree_sub_min (bintree_sub_t *this) {
	bintree_node_t *node = this->sub_root;

	if (!node)
		return NULL;

	while (node->low)
		node = node->low;

	return node;
}

static bintree_node_t *bintree_sub_max (bintree_sub_t *this) {
	bintree_node_t *node = this->sub_root;

	if (!node)
		return NULL;

	while (node->high)
		node = node->high;

	return node;
}

static bintree_node_t *bintree_sub_search (bintree_sub_t *this, size_t orderby) {
	bintree_node_t *parent = this->sub_root;

	if (!parent)
		return NULL;

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

bintree_sub_t new_bintree_sub (
		bintree_node_t *node
) {
	bintree_sub_t subtree = {
		.sub_root = node,
		.min = bintree_sub_min,
		.max = bintree_sub_max,
		.search = bintree_sub_search
	};

	return subtree;
}

// Returns smallest/largest node in tree, NULL if the tree is empty.
static bintree_node_t *bintree_min (bintree_t *this) {
	bintree_sub_t
		subtree_base,
		*subtree = &subtree_base;

	subtree_base = new_bintree_sub (this->root);

	return subtree->min (subtree);
}

static bintree_node_t *bintree_max (bintree_t *this) {
	bintree_sub_t
		subtree_base,
		*subtree = &subtree_base;

	subtree_base = new_bintree_sub (this->root);

	return subtree->max (subtree);
}

static bintree_node_t *bintree_search (bintree_t *this, size_t orderby) {
	bintree_sub_t
		subtree_base,
		*subtree = &subtree_base;

	subtree_base = new_bintree_sub (this->root);

	return subtree->search (subtree, orderby);
}

// Returns NULL if node inserted successfully,
// otherwise returns node with conflicting .orderby field.
static bintree_node_t *bintree_insert (bintree_t *this, bintree_node_t *node) {
	bintree_node_t *parent = this->search (this, node->orderby);

	if (!parent) {
		this->root = node;
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

static void bintree_remove (bintree_t *this, bintree_node_t *node) {
	const unsigned char children_count =
		(node->low  ? 1 : 0) +
		(node->high ? 1 : 0);

	switch (children_count) {
		case 0:
			bintree_remove_node_zero (this, node);
			break;
		case 1:
			bintree_remove_node_one (this, node);
			break;
		case 2:
			bintree_remove_node_two (this, node);
			break;
	}
}

static bintree_iterator_t new_bintree_iterator_from_tree (bintree_t *this) {
	return new_bintree_iterator (this->min (this));
}

bintree_t new_bintree () {
	bintree_t tree = {
		.root = NULL,
		.last_rm = 0,
		.min = bintree_min,
		.max = bintree_max,
		.search = bintree_search,
		.insert = bintree_insert,
		.remove = bintree_remove,
		.new_iterator = new_bintree_iterator_from_tree,
	};

	return tree;
}

// vim: set ts=4 sw=4 noet syn=c:
