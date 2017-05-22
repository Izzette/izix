// kernel/mm/freemem.c

#include <stdbool.h>

#include <collections/bintree.h>
#include <collections/sparse_collection.h>

#include <kprint/kprint.h>
#include <kpanic/kpanic.h>
#include <mm/freemem.h>
#include <sched/spinlock.h>
#include <sched/kthread.h>

// TODO: By limitation of size_t and the choice to use the exclusive maximum of regions
//       the last byte of the addressable memory space  cannot be used, as the exclusive
//       maximum will read as zero.  This will likely never be an issue, because there is
//       (almost?) always a reserved memory region leading up to that byte, but if it were
//       to occur, it would realistically result of the loss of a complete page of memory.
//       Perhaps, this should be solved of sooner rather than later ...

typedef struct __attribute__((packed)) freemem_zero_width_struct {
} freemem_zero_width_t;

TPL_BINTREE (region, size_t)
TPL_BINTREE (p, freemem_zero_width_t)
TPL_BINTREE (length, bintree_p_fields_t)

TPL_SPARSE_COLLECTION(region_node, bintree_region_node_t)
TPL_SPARSE_COLLECTION(p_node, bintree_p_node_t)
TPL_SPARSE_COLLECTION(length_node, bintree_length_node_t)

static bintree_region_t
	region_tree_base,
	*region_tree = &region_tree_base;
static bintree_length_t
	length_tree_base,
	*length_tree = &length_tree_base;

static sparse_collection_region_node_t
	region_nodes_base,
	*region_nodes = &region_nodes_base;
static sparse_collection_p_node_t
	p_nodes_base,
	*p_nodes = &p_nodes_base;
static sparse_collection_length_node_t
	length_nodes_base,
	*length_nodes = &length_nodes_base;

static spinlock_t
	freemem_lock_base,
	*freemem_lock = &freemem_lock_base;

// Panics if perfect matches cannot be found.
static bintree_region_node_t *freemem_get_nodes (
		freemem_region_t region,
		bintree_length_node_t **length_node_ptr,
		bintree_p_node_t **p_node_ptr) {
	bintree_region_node_t *region_node;
	bintree_p_t p_tree_base, *p_tree = &p_tree_base;

	region_node = region_tree->search (region_tree, (size_t)region.p);
	*length_node_ptr = length_tree->search (length_tree, region.length);

	if (!region_node ||
			(size_t)region.p != region_node->orderby ||
			region.length != region_node->data) {
		kputs ("mm/freemem: Failed to find matching region node!\n");
		kpanic ();
	}

	if (!*length_node_ptr ||
			region.length != (*length_node_ptr)->orderby) {
		kputs ("mm/freemem: Failed to find matching length node!\n");
		kpanic ();
	}

	p_tree_base = new_bintree_p_from_fields ((*length_node_ptr)->data);

	*p_node_ptr = p_tree->search (p_tree, (size_t)region.p);

	if (!*p_node_ptr ||
			(size_t)region.p != (*p_node_ptr)->orderby) {
		kputs ("mm/freemem: Failed to find matching p node!\n");
		kpanic ();
	}

	return region_node;
}

static void freemem_delete (freemem_region_t region) {
	bintree_region_node_t *region_node;
	bintree_length_node_t *length_node;
	bintree_p_node_t *p_node;
	bintree_p_t p_tree_base, *p_tree = &p_tree_base;

	region_node = freemem_get_nodes (region, &length_node, &p_node);

	p_tree_base = new_bintree_p_from_fields (length_node->data);

	region_tree->remove (region_tree, region_node);
	region_nodes->free (region_nodes, region_node);
	p_tree->remove (p_tree, p_node);
	p_nodes->free (p_nodes, p_node);
	if (!p_tree->root) {
		length_tree->remove (length_tree, length_node);
		length_nodes->free (length_nodes, length_node);
	} else {
		length_node->data = p_tree->get_fields (p_tree);
	}
}

static void freemem_insert (freemem_region_t region) {
	bintree_region_node_t *region_node, *region_conflict;
	bintree_length_node_t *length_node, *length_conflict;
	bintree_p_node_t *p_node, *p_conflict;
	bintree_p_t p_tree_base, *p_tree = &p_tree_base;
	size_t i;

	i = region_nodes->get (region_nodes);
	if (!i) {
		kputs ("mm/freemem: failed to allocate new region node!\n");
		kpanic ();
	}
	region_node = region_nodes->alloc (region_nodes, i);

	i = p_nodes->get (p_nodes);
	if (!i) {
		kputs ("mm/freemem: failed to allocate new p node!\n");
		kpanic ();
	}
	p_node = p_nodes->alloc (p_nodes, i);

	*region_node = new_bintree_region_node (region.length, (size_t)region.p);
	*p_node = new_bintree_p_node ((freemem_zero_width_t){}, (size_t)region.p);

	// Find or allocate length node
	length_node = length_tree->search (length_tree, region.length);
	if (!length_node ||
			region.length != length_node->orderby) {
		i = length_nodes->get (length_nodes);
		if (!i) {
			kputs ("mm/freemem: failed to allocate new length node!\n");
			kpanic ();
		}
		length_node = length_nodes->alloc (length_nodes, i);

		*length_node = new_bintree_length_node (new_bintree_p_fields (), region.length);

		length_conflict = length_tree->insert (length_tree, length_node);
		if (length_conflict) {
			kputs ("mm/freemem: failed to insert new length node!\n");
			kputs ("\tbintree bug?\n");
			kpanic ();
		}
	}

	region_conflict = region_tree->insert (region_tree, region_node);
	if (region_conflict) {
		kputs ("mm/freemem: failed to insert new region node!\n");
		kpanic ();
	}

	p_tree_base = new_bintree_p_from_fields (length_node->data);
	p_conflict = p_tree->insert (p_tree, p_node);
	if (p_conflict) {
		kputs ("mm/freemem: failed to insert new p node!\n");
		kpanic ();
	}
	length_node->data = p_tree->get_fields (p_tree);
}

static bool freemem_is_consecutive (
		freemem_region_t region1,
		freemem_region_t region2
) {
	freemem_region_t low_region, high_region;

	if (region1.p < region2.p) {
		low_region = region1;
		high_region = region2;
	} else {
		low_region = region2;
		high_region = region1;
	}

	return high_region.p == low_region.p + low_region.length;
}

static bool freemem_is_subset (
		freemem_region_t set,
		freemem_region_t subset
) {
	if (!(set.p <= subset.p))
		return false;
	if (!(freemem_region_end (set) >= freemem_region_end (subset)))
		return false;

	return true;
}

// Assumed consecutive.
static freemem_region_t freemem_join (
		freemem_region_t region1,
		freemem_region_t region2
) {
	size_t new_length;
	void *new_p;
	freemem_region_t new_region;

	new_length = region1.length + region2.length;
	if (region1.p < region2.p)
		new_p = region1.p;
	else
		new_p = region2.p;
	new_region = new_freemem_region (new_p, new_length);

	freemem_delete (region1);
	freemem_delete (region2);

	freemem_insert (new_region);

	return new_region;
}

// returns joined entry, or new_freemem_region (NULL, 0).
static freemem_region_t freemem_maybe_join (
		freemem_region_t region1,
		freemem_region_t region2
) {
	if (!freemem_is_consecutive (region1, region2))
		return new_freemem_region (NULL, 0);

	return freemem_join (region1, region2);
}

static void freemem_defrag (freemem_region_t region) {
	bintree_region_iterator_t iterator_base, *iterator = &iterator_base;
	bintree_region_node_t *region_node, *prev_region_node, *next_region_node;
	freemem_region_t new_region, prev_region, next_region;

	region_node = region_tree->search (region_tree, (size_t)region.p);
	if (!region_node) {
		kputs ("mm/freemem: Failed to find region node while defragmenting!\n");
		kpanic ();
	}

	iterator_base = new_bintree_region_iterator (region_node);
	if (iterator->prev (iterator)) {
		prev_region_node = iterator->cur (iterator);
		prev_region = new_freemem_region (
			(void *)prev_region_node->orderby,
			prev_region_node->data);

		new_region = freemem_maybe_join (region, prev_region);
		if (new_region.length) {
			region = new_region;
			region_node = region_tree->search (region_tree, (size_t)region.p);
			if (!region_node) {
				kputs ("mm/freemem: Failed to find new region while defragmenting!\n");
				kpanic ();
			}
		}
	}

	iterator_base = new_bintree_region_iterator (region_node);
	if (iterator->next (iterator)) {
		next_region_node = iterator->cur (iterator);
		next_region = new_freemem_region (
			(void *)next_region_node->orderby,
			next_region_node->data);

		new_region = freemem_maybe_join (region, next_region);
	}
}

static freemem_region_t freemem_suggest (
		size_t length,
		size_t alignment,
		int offset
) {
	bintree_p_t
		p_length_tree_base,
		*p_length_tree = &p_length_tree_base;
	bintree_length_node_t *length_entry;
	bintree_p_node_t *p_entry;
	bintree_length_iterator_t
		length_iterator_base,
		*length_iterator = &length_iterator_base;
	bintree_p_iterator_t
		p_iterator_base,
		*p_iterator = &p_iterator_base;

	length_entry = length_tree->search (length_tree, length);
	if (!length_entry)
		return new_freemem_region (NULL, 0);

	length_iterator_base = new_bintree_length_iterator (length_entry);

	if (length > length_entry->orderby) {
		length_entry = length_iterator->next (length_iterator);
		if (!length_entry)
			return new_freemem_region (NULL, 0);
	}

	do {
		size_t align_inc;

		p_length_tree_base = new_bintree_p_from_fields (length_entry->data);
		p_iterator_base = p_length_tree->new_iterator (p_length_tree);

		p_entry = p_iterator->cur (p_iterator);
		while (p_entry) {
			if ((size_t)p_entry->orderby % alignment)
				align_inc = alignment - (size_t)p_entry->orderby % alignment;
			else
				align_inc = 0;

			if (0 <= offset) {
				align_inc += offset;
			} else {
				if ((size_t)(-1 * offset) > align_inc)
					align_inc += alignment;
				align_inc -= -1 * offset;
			}

			if (length + align_inc <= length_entry->orderby)
				return new_freemem_region (
					(void *)p_entry->orderby + align_inc,
					length);

			p_entry = p_iterator->next (p_iterator);
		}

		length_entry = length_iterator->next (length_iterator);
	} while (length_entry);

	return new_freemem_region (NULL, 0);
}

static bool freemem_add_region_internal (freemem_region_t region) {
	freemem_insert (region);
	freemem_defrag (region);

	return true;
}

static bool freemem_remove_region_internal (freemem_region_t region) {
	bintree_region_iterator_t iterator_base, *iterator = &iterator_base;
	bintree_region_node_t *parent;
	freemem_region_t parent_region;

	parent = region_tree->search (region_tree, (size_t)region.p);

	if (!parent)
		return false;

	iterator_base = new_bintree_region_iterator (parent);
	if ((size_t)region.p < parent->orderby)
		parent = iterator->prev (iterator);
	else {
		parent_region = new_freemem_region ((void *)parent->orderby, parent->data);

		if (freemem_region_end (region) > freemem_region_end (parent_region))
			parent = iterator->next (iterator);
	}
	if (!parent)
		return false;
	parent_region = new_freemem_region ((void *)parent->orderby, parent->data);

	if (!freemem_is_subset (parent_region, region))
		return false;

	unsigned char region_facts =
		((region.p == parent_region.p)
			? 1 : 0) |
		((freemem_region_end (region) == freemem_region_end (parent_region))
			? 1 : 0) << 1;

	switch (region_facts) {
		case 0b00: // region shares no edges with superset.
			freemem_delete (parent_region);
			freemem_insert (
				new_freemem_region (
					parent_region.p,
					region.p - parent_region.p));
			freemem_insert (
				new_freemem_region (
					freemem_region_end (region),
					freemem_region_end (parent_region) - freemem_region_end (region)));
			break;
		case 0b01: // region shares the start with superset region.
			freemem_delete (parent_region);
			freemem_insert (
				new_freemem_region (
					freemem_region_end (region),
					parent_region.length - region.length));

			break;
		case 0b10: // region shares the end with superset region.
			freemem_delete (parent_region);
			freemem_insert (
				new_freemem_region (
					parent_region.p,
					parent_region.length - region.length));
			break;
		default: // region is superset region.
			freemem_delete (parent_region);
	}

	return true;
}

static freemem_region_t freemem_alloc_internal (
		size_t length,
		size_t alignment,
		int offset
) {
	const freemem_region_t suggestion = freemem_suggest (length, alignment, offset);
	if (!suggestion.length)
		return suggestion;

	const bool remove_success = freemem_remove_region_internal (suggestion);
	if (!remove_success)
		return new_freemem_region (NULL, 0);

	return suggestion;
}

// Must be called before initializing kthreads
void freemem_init (void *internal, size_t internal_length) {
	freemem_lock_base = new_spinlock ();

	region_tree_base = new_bintree_region ();
	length_tree_base = new_bintree_length ();

	size_t total_size =
		4 * sizeof(bintree_region_node_t) +
		sizeof(bintree_length_node_t) +
		4 * sizeof(bintree_p_node_t);

	void *next_data = internal;
	size_t next_size = 0;

	next_data += next_size;
	next_size = internal_length * 4 * sizeof(bintree_region_node_t) / total_size;
	region_nodes_base = new_sparse_collection_region_node (next_data, next_size);

	next_data += next_size;
	next_size = internal_length * sizeof(bintree_length_node_t) / total_size;
	length_nodes_base = new_sparse_collection_length_node (next_data, next_size);

	next_data += next_size;
	next_size = internal_length * 4 * sizeof(bintree_p_node_t) / total_size;
	p_nodes_base = new_sparse_collection_p_node (next_data, next_size);
}

// We can't use an ordinary mutex here, because it requires memory allocation.
static void freemem_obtain_lock () {
	while (!spinlock_try_lock (freemem_lock))
		kthread_yield ();
}

static void freemem_release_lock () {
	spinlock_release (freemem_lock);
}

bool freemem_add_region (freemem_region_t region) {
	freemem_obtain_lock ();

	const bool ret = freemem_add_region_internal (region);

	freemem_release_lock ();

	return ret;
}

bool freemem_remove_region (freemem_region_t region) {
	freemem_obtain_lock ();

	const bool ret = freemem_remove_region_internal (region);

	freemem_release_lock ();

	return ret;
}

freemem_region_t freemem_alloc (size_t length, size_t alignment, int offset) {
	freemem_obtain_lock ();

	const freemem_region_t ret = freemem_alloc_internal (length, alignment, offset);

	freemem_release_lock ();

	return ret;
}

// vim: set ts=4 sw=4 noet syn=c:
