// libk/collections/linked_list.c

#include <collections/linked_list.h>

static void linked_list_insert_between (
		linked_list_node_t *,
		linked_list_node_t *,
		linked_list_node_t *
);
static linked_list_node_t *linked_list_get_from (linked_list_node_t *, size_t);

static inline linked_list_node_t *linked_list_next (linked_list_node_t *node) {
	return node->next;
}

static inline linked_list_node_t *linked_list_prev (linked_list_node_t *node) {
	return node->prev;
}

static inline void linked_list_insert_after (
		linked_list_node_t *before,
		linked_list_node_t *node
) {
	linked_list_insert_between (before, node, linked_list_next (before));
}

static inline void linked_list_insert_before (
		linked_list_node_t *after,
		linked_list_node_t *node
) {
	linked_list_insert_between (linked_list_prev (after), node, after);
}

static inline void linked_list_detach (
		linked_list_node_t *prev,
		linked_list_node_t *next
) {
	if (prev)
		prev->next = NULL;
	if (next)
		next->prev = NULL;
}

static inline void linked_list_attach (
		linked_list_node_t *prev,
		linked_list_node_t *next
) {
	if (prev)
		prev->next = next;
	if (next)
		next->prev = prev;
}

static void linked_list_insert_between (
		linked_list_node_t *before,
		linked_list_node_t *between,
		linked_list_node_t *after
) {
	linked_list_attach (before, between);
	linked_list_attach (between, after);
}

static linked_list_node_t *linked_list_insert_at (
		linked_list_node_t *start,
		size_t i,
		linked_list_node_t *node
) {
	linked_list_node_t *before;

	if (!start)
		return node;

	if (0 == i) {
		linked_list_insert_before (start, node);
		return node;
	}

	before = linked_list_get_from (start, i - 1);

	if (before)
		linked_list_insert_after (before, node);

	return start;
}

static void linked_list_remove_node (linked_list_node_t *node) {
	linked_list_node_t *prev, *next;

	prev = linked_list_prev (node);
	next = linked_list_next (node);

	linked_list_detach (prev, node);
	linked_list_detach (node, next);

	linked_list_attach (prev, next);
}

static size_t linked_list_count_from (linked_list_node_t *start) {
	size_t i = 0;

	if (!start)
		return 0;

	do
		i += 1;
	while ((start = linked_list_next (start)));

	return i;
}

static linked_list_node_t *linked_list_get_from (linked_list_node_t *start, size_t index) {
	size_t i;

	for (i = 0; start && index > i; ++i)
		start = linked_list_next (start);

	return start;
}

linked_list_node_t *linked_list_iterator_cur (linked_list_iterator_t *this) {
	return this->node;
}

linked_list_node_t *linked_list_iterator_next (linked_list_iterator_t *this) {
	if (!this->node)
		return NULL;

	if (linked_list_next (this->node))
		this->node = linked_list_next (this->node);
	else
		return NULL;

	return this->node;
}

linked_list_node_t *linked_list_iterator_prev (linked_list_iterator_t *this) {
	if (!this->node)
		return NULL;

	if (linked_list_prev (this->node))
		this->node = linked_list_prev (this->node);
	else
		return NULL;

	return this->node;
}

void linked_list_iterator_reset (linked_list_iterator_t *this) {
	while (linked_list_iterator_prev (this));
}

size_t linked_list_count (linked_list_t *this) {
	if (!this->start)
		return 0;

	return linked_list_count_from (this->start);
}

linked_list_node_t *linked_list_peek (linked_list_t *this) {
	return this->start;
}

linked_list_node_t *linked_list_peekEnd (linked_list_t *this) {
	return this->end;
}

linked_list_node_t *linked_list_get (linked_list_t *this, size_t i) {
	return linked_list_get_from (this->start, i);
}

void linked_list_push (linked_list_t *this, linked_list_node_t *node) {
	this->insert (this, 0, node);
}

void linked_list_append (linked_list_t *this, linked_list_node_t *node) {
	if (!this->start) {
		this->start = node;
		this->end = node;
	} else {
		linked_list_insert_after (this->end, node);
		this->end = node;
	}
}

bool linked_list_insert (linked_list_t *this, size_t i, linked_list_node_t *node) {
	linked_list_node_t *new_start = linked_list_insert_at (this->start, i, node);

	if (!new_start)
		return false;

	if (this->start != new_start) {
		this->start = new_start;
		if (!this->end)
			this->end = new_start;
	}

	return true;
}

linked_list_node_t *linked_list_pop (linked_list_t *this) {
	if (!this->start)
		return NULL;

	linked_list_node_t *node = this->start;

	this->removeNode (this, node);

	return node;
}

linked_list_node_t *linked_list_popEnd (linked_list_t *this) {
	if (!this->end)
		return NULL;

	linked_list_node_t *node = this->end;

	this->removeNode (this, node);

	return node;
}

linked_list_node_t *linked_list_remove (linked_list_t *this, size_t i) {
	linked_list_node_t *node = linked_list_get_from (this->start, i);
	if (!node)
		return NULL;

	this->removeNode (this, node);

	return node;
}

void linked_list_removeNode (linked_list_t *this, linked_list_node_t *node) {
	if (this->start == node)
		this->start = node->next;
	if (this->end == node)
		this->end = node->prev;

	linked_list_remove_node (node);
}

linked_list_iterator_t new_linked_list_iterator_from_list (linked_list_t *this) {
	return new_linked_list_iterator (this->start);
}

// vim: set ts=4 sw=4 noet syn=c:
