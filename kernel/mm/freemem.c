// kernel/mm/freemem.c

#include <stdbool.h>

#include <collections.h>

#include <kprint/kprint.h>
#include <kpanic/kpanic.h>
#include <mm/freemem.h>

// TODO: By limitation of size_t and the choice to use the exclusive maximum of regions
//       the last byte of the addressable memory space  cannot be used, as the exclusive
//       maximum will read as zero.  This will likely never be an issue, because there is
//       (almost?) always a reserved memory region leading up to that byte, but if it were
//       to occur, it would realistically result of the loss of a complete page of memory.
//       Perhaps, this should be solved of sooner rather than later ...

typedef struct freemem_entry_struct {
	freemem_region_t region;
	bintree_node_t node_base;
} freemem_entry_t;

TPL_PACKED_LIST(freemem_entry, freemem_entry_t)

static bintree_t freemem_tree_base;
static bintree_t *freemem_tree = &freemem_tree_base;
static packed_list_freemem_entry_t freemem_entry_list_base;
static packed_list_freemem_entry_t *freemem_entry_list = &freemem_entry_list_base;

static inline freemem_entry_t new_freemem_entry (
		freemem_region_t region
) {
	freemem_entry_t entry = {
		.region = region,
		.node_base = new_bintree_node (0, NULL)
	};

	return entry;
}

static inline bintree_node_t *freemem_get_node (freemem_entry_t *entry) {
	return &entry->node_base;
}

static inline freemem_entry_t *freemem_get_node_data (bintree_node_t *node) {
	return (freemem_entry_t *)node->data;
}

static inline void freemem_fix_entry (freemem_entry_t *entry) {
	bintree_node_t *node = freemem_get_node (entry);
	void *data = entry;
	size_t orderby = (size_t)entry->region.p;

	*node = new_bintree_node (orderby, data);
}

static bool freemem_pre_remove (freemem_entry_t *entry) {
	bintree_remove_node (freemem_tree, freemem_get_node (entry));

	return true;
}

static bool freemem_post_add (freemem_entry_t *entry) {
	// Fix the moved entry.
	freemem_fix_entry (entry);
	// Then insert it back into the tree.
	bintree_node_t *conflict = bintree_insert_node (
			freemem_tree, freemem_get_node (entry));
	if (conflict) {
		kputs ("mm/freemem: Failed to do trivial insert while adding entry!\n");
		kpanic ();
	}

	return true;
}

static void freemem_rollback_remove (freemem_entry_t *entry) {
	// Fix the moved entry.
	freemem_fix_entry (entry);
	// Then insert it back into the tree.
	bintree_node_t *conflict = bintree_insert_node (
			freemem_tree, freemem_get_node (entry));
	if (conflict) {
		kputs (
			"mm/freemem: Failed to do trivial reinsert "
			"while rolling back removal of an entry!\n");
		kpanic ();
	}
}

static inline freemem_entry_t *freemem_join_entries (
		freemem_entry_t *entry1,
		freemem_entry_t *entry2
) {
	freemem_entry_t joined_entry_base =
		new_freemem_entry (freemem_join_regions (entry1->region, entry2->region));

	const bool remove_first_success =
		freemem_entry_list->remove_elm (freemem_entry_list, entry1);
	const bool remove_second_success =
		freemem_entry_list->remove_elm (freemem_entry_list, entry2);
	if (!remove_first_success || !remove_second_success) {
		kputs ("mm/freemem: Failed to do trivial entry removal!\n");
		kpanic ();
	}

	freemem_entry_list->append (freemem_entry_list, joined_entry_base);

	return freemem_entry_list->get_last (freemem_entry_list);
}

// returns joined entry, NULL if not joined.
static inline freemem_entry_t *freemem_maybe_join (
		freemem_entry_t *entry1,
		freemem_entry_t *entry2
) {
	if (!freemem_consecutive_regions (entry1->region, entry2->region))
		return NULL;

	return freemem_join_entries (entry1, entry2);;
}

// Returns the entry that contains the region in the suppied entry,
// which may be the same pointer and may be the same entry.
static inline freemem_entry_t *freemem_defrag_entry (freemem_entry_t *entry) {
	bintree_node_t *node, *prev_node, *next_node;
	freemem_entry_t *prev_entry, *next_entry, *new_entry;

	node = freemem_get_node (entry);

	prev_node = bintree_node_prev (node);
	next_node = bintree_node_next (node);

	if (prev_node) {
		prev_entry = prev_node->data;

		new_entry = freemem_maybe_join (entry, prev_entry);
		if (new_entry)
			entry = new_entry;
	}

	if (next_node) {
		next_entry = next_node->data;

		new_entry = freemem_maybe_join (entry, next_entry);
		if (new_entry)
			entry = new_entry;
	}

	return entry;
}

void freemem_init (void *internal, size_t internal_length) {
	freemem_tree_base = new_bintree ();
	freemem_entry_list_base = new_packed_list_freemem_entry (
		internal,
		internal_length,
		freemem_pre_remove,
		freemem_rollback_remove,
		freemem_post_add);
}

bool freemem_add_region (freemem_region_t region) {
	freemem_entry_t entry_base = new_freemem_entry (region);

	const bool add_success = freemem_entry_list->append (freemem_entry_list, entry_base);
	if (!add_success)
		return false;

	freemem_defrag_entry (freemem_entry_list->get_last (freemem_entry_list));

	return true;
}

bool freemem_remove_region (freemem_region_t region) {
	bintree_node_t *parent = bintree_search (freemem_tree, (size_t)region.p);

	if (!parent)
		return false;

	freemem_region_t parent_region = freemem_get_node_data (parent)->region;

	if (region.p < parent_region.p) do {
		parent_region = freemem_get_node_data (parent)->region;

		if (region.p >= parent_region.p)
			break;

		parent = bintree_node_prev (parent);
	} while (parent); else do {
		parent_region = freemem_get_node_data (parent)->region;

		if (freemem_get_region_end (region) <= freemem_get_region_end (parent_region))
			break;

		parent = bintree_node_next (parent);
	} while (parent);

	if (!parent)
		return false;

	if (!freemem_region_subset (parent_region, region))
		return false;

	bintree_node_t *superset_node;
	freemem_entry_t *superset_entry;
	void *region_p, *superset_region_p;
	void *region_end, *superset_region_end;

	superset_node = parent;

	superset_entry = freemem_get_node_data (superset_node);

	region_p = region.p;
	superset_region_p = parent_region.p;

	region_end = freemem_get_region_end (region);
	superset_region_end = freemem_get_region_end (parent_region);

	unsigned char region_facts =
		(region_p   == superset_region_p   ? 1 : 0) |
		(region_end == superset_region_end ? 1 : 0) << 1;

	bool region_add_success;
	size_t old_length;
	bintree_node_t *conflict;

	switch (region_facts) {
		case 0b00: // region shares no edges with superset.
			old_length = superset_entry->region.length;
			superset_entry->region.length = region.p - superset_entry->region.p;
			region_add_success = freemem_add_region (
				new_freemem_region (
					region_end, (size_t)(superset_region_end - region_end)));
			if (!region_add_success) {
				// Rollback.
				superset_entry->region.length = old_length;
				return false;
			}
			break;
		case 0b01: // region shares the start with superset region.
			bintree_remove_node (freemem_tree, superset_node);
			superset_entry->region.p += region.length;
			superset_entry->region.length -= region.length;
			superset_node->orderby = (size_t)superset_entry->region.p;
			conflict = bintree_insert_node (freemem_tree, superset_node);
			if (conflict) {
				kputs (
					"mm/freemem: Failed to do trivial reinsert while increasing "
					"start of free region as part of a remove region operation!\n");
				kpanic ();
			}
			break;
		case 0b10: // region shares the end with superset region.
			superset_entry->region.length -= region.length;
			break;
		default: // region is superset region.
			freemem_entry_list->remove_elm (freemem_entry_list, superset_entry);
	}

	return true;
}

freemem_region_t freemem_suggest (size_t length, size_t alignment, int offset) {
	packed_list_freemem_entry_iterator_t iterator =
		new_packed_list_freemem_entry_iterator (freemem_entry_list);
	freemem_entry_t *entry;

	for (entry = iterator.cur (&iterator); entry; entry = iterator.next (&iterator)) {
		size_t align_inc;

		if ((size_t)entry->region.p % alignment)
			align_inc = alignment - (size_t)entry->region.p % alignment;
		else
			align_inc = 0;

		if (0 <= offset) {
			align_inc += offset;
		} else {
			if ((size_t)(-1 * offset) > align_inc)
				align_inc += alignment;
			align_inc -= -1 * offset;
		}

		if (length + align_inc > entry->region.length)
			continue;

		return new_freemem_region (
			entry->region.p + align_inc,
			length);
	}

	return new_freemem_region ((void *)0x0, 0);
}

// vim: set ts=4 sw=4 noet syn=c:
