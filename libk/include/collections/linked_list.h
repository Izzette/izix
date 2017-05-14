// libk/include/collections/linked_list.h

#ifndef IZIX_LIBK_COLLECTIONS_LINKED_LIST_H
#define IZIX_LIBK_COLLECTIONS_LINKED_LIST_H 1

#include <stddef.h>
#include <stdbool.h>

typedef struct linked_list_node_struct linked_list_node_t;
typedef struct linked_list_node_struct {
	linked_list_node_t *prev;
	linked_list_node_t *next;
} linked_list_node_t;

static inline linked_list_node_t new_linked_list_node () {
	linked_list_node_t node = {
		.prev = NULL,
		.next = NULL
	};

	return node;
}

typedef struct linked_list_iterator_struct linked_list_iterator_t;
typedef struct linked_list_iterator_struct {
	linked_list_node_t *node;
	linked_list_node_t *(*cur) (linked_list_iterator_t *);
	linked_list_node_t *(*next) (linked_list_iterator_t *);
	linked_list_node_t *(*prev) (linked_list_iterator_t *);
	void (*reset) (linked_list_iterator_t *);
} linked_list_iterator_t;

linked_list_node_t *linked_list_iterator_cur (linked_list_iterator_t *);
linked_list_node_t *linked_list_iterator_next (linked_list_iterator_t *);
linked_list_node_t *linked_list_iterator_prev (linked_list_iterator_t *);
void linked_list_iterator_reset (linked_list_iterator_t *);

static inline linked_list_iterator_t new_linked_list_iterator (
		linked_list_node_t *node
) {
	linked_list_iterator_t iterator = {
		.node = node,
		.cur = linked_list_iterator_cur,
		.next = linked_list_iterator_next,
		.prev = linked_list_iterator_prev,
		.reset = linked_list_iterator_reset
	};

	return iterator;
}

typedef struct linked_list_struct linked_list_t;
typedef struct linked_list_struct {
	linked_list_node_t *start;
	linked_list_node_t *end;
	size_t (*count) (linked_list_t *);
	linked_list_node_t *(*peek) (linked_list_t *);
	linked_list_node_t *(*peekEnd) (linked_list_t *);
	linked_list_node_t *(*get) (linked_list_t *, size_t);
	void (*push) (linked_list_t *, linked_list_node_t *);
	void (*append) (linked_list_t *, linked_list_node_t *);
	bool (*insert) (linked_list_t *, size_t, linked_list_node_t *);
	linked_list_node_t *(*pop) (linked_list_t *);
	linked_list_node_t *(*popEnd) (linked_list_t *);
	linked_list_node_t *(*remove) (linked_list_t *, size_t);
	void (*removeNode) (linked_list_t *, linked_list_node_t *);
	linked_list_iterator_t (*new_iterator) (linked_list_t *);
} linked_list_t;

size_t linked_list_count (linked_list_t *);
linked_list_node_t *linked_list_peek (linked_list_t *);
linked_list_node_t *linked_list_peekEnd (linked_list_t *);
linked_list_node_t *linked_list_get (linked_list_t *, size_t);
void linked_list_push (linked_list_t *, linked_list_node_t *);
void linked_list_append (linked_list_t *, linked_list_node_t *);
bool linked_list_insert (linked_list_t *, size_t, linked_list_node_t *);
linked_list_node_t *linked_list_pop (linked_list_t *);
linked_list_node_t *linked_list_popEnd (linked_list_t *);
linked_list_node_t *linked_list_remove (linked_list_t *, size_t);
void linked_list_removeNode (linked_list_t *, linked_list_node_t *);
linked_list_iterator_t new_linked_list_iterator_from_list (linked_list_t *);

static inline linked_list_t new_linked_list () {
	linked_list_t list = {
		.start = NULL,
		.end = NULL,
		.count = linked_list_count,
		.peek = linked_list_peek,
		.peekEnd = linked_list_peekEnd,
		.get = linked_list_get,
		.push = linked_list_push,
		.append = linked_list_append,
		.insert = linked_list_insert,
		.pop = linked_list_pop,
		.popEnd = linked_list_popEnd,
		.remove = linked_list_remove,
		.removeNode = linked_list_removeNode,
		.new_iterator = new_linked_list_iterator_from_list
	};

	return list;
}

#define TPL_LINKED_LIST(name, type) \
typedef struct linked_list_##name##_node_struct { \
	linked_list_node_t node; \
	type data; \
} linked_list_##name##_node_t; \
\
static inline linked_list_##name##_node_t new_linked_list_##name##_node (type data) { \
	linked_list_##name##_node_t node = { \
		.node = new_linked_list_node (), \
		.data = data \
	}; \
\
	return node; \
} \
typedef struct linked_list_##name##_iterator_struct linked_list_##name##_iterator_t; \
typedef struct linked_list_##name##_iterator_struct { \
	linked_list_##name##_node_t *node; \
	linked_list_##name##_node_t *(*cur) (linked_list_##name##_iterator_t *); \
	linked_list_##name##_node_t *(*next) (linked_list_##name##_iterator_t *); \
	linked_list_##name##_node_t *(*prev) (linked_list_##name##_iterator_t *); \
	void (*reset) (linked_list_##name##_iterator_t); \
} linked_list_##name##_iterator_t; \
\
static inline linked_list_##name##_iterator_t new_linked_list_##name##_iterator ( \
		linked_list_##name##_node_t *node \
) { \
	linked_list_iterator_t iterator = \
		new_linked_list_iterator ((linked_list_node_t *)node); \
	return *(linked_list_##name##_iterator_t *)&iterator; \
} \
typedef struct linked_list_##name##_struct linked_list_##name##_t; \
typedef struct linked_list_##name##_struct { \
	linked_list_##name##_node_t *start; \
	linked_list_##name##_node_t *end; \
	linked_list_##name##_node_t *(*count) (linked_list_##name##_t *); \
	linked_list_##name##_node_t *(*peek) (linked_list_##name##_t *); \
	linked_list_##name##_node_t *(*peekEnd) (linked_list_##name##_t *); \
	linked_list_##name##_node_t *(*get) (linked_list_##name##_t *, size_t i); \
	void (*push) (linked_list_##name##_t *, linked_list_##name##_node_t *); \
	void (*append) (linked_list_##name##_t *, linked_list_##name##_node_t *); \
	bool (*insert) (linked_list_##name##_t *, size_t i, linked_list_##name##_node_t *); \
	linked_list_##name##_node_t *(*pop) (linked_list_##name##_t *); \
	linked_list_##name##_node_t *(*popEnd) (linked_list_##name##_t *); \
	linked_list_##name##_node_t *(*remove) (linked_list_##name##_t *, size_t); \
	linked_list_##name##_node_t *(*removeNode) ( \
			linked_list_##name##_t *, linked_list_##name##_node_t *); \
	linked_list_##name##_iterator_t (*new_iterator) (linked_list_##name##_t *); \
} linked_list_##name##_t; \
\
static inline linked_list_##name##_t new_linked_list_##name () { \
	linked_list_t list = new_linked_list (); \
	return *(linked_list_##name##_t *)&list; \
}

#endif

// vim: set ts=4 sw=4 noet syn=c:
