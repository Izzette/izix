// kernel/mm/freemem.c

#include <stdbool.h>

#include <collections/bintree.h>
#include <collections/packed_list.h>

#include <kprint/kprint.h>
#include <kpanic/kpanic.h>
#include <mm/freemem.h>

// TODO: By limitation of size_t and the choice to use the exclusive maximum of regions
//       the last byte of the addressable memory space  cannot be used, as the exclusive
//       maximum will read as zero.  This will likely never be an issue, because there is
//       (almost?) always a reserved memory region leading up to that byte, but if it were
//       to occur, it would realistically result of the loss of a complete page of memory.
//       Perhaps, this should be solved of sooner rather than later ...

TPL_BINTREE (freemem, freemem_region_t)
TPL_PACKED_LIST(freemem, bintree_freemem_node_t)

static bintree_freemem_t freemem_tree_base;
static bintree_freemem_t *freemem_tree = &freemem_tree_base;
static packed_list_freemem_t freemem_entry_list_base;
static packed_list_freemem_t *freemem_entry_list = &freemem_entry_list_base;

static inline bintree_freemem_node_t new_freemem_entry (
		freemem_region_t region
) {
	bintree_freemem_node_t entry = new_bintree_freemem_node (
		region,
		(size_t)region.p
	);

	return entry;
}

static inline void freemem_fix_entry (bintree_freemem_node_t *entry) {
	entry->node.orderby = (size_t)entry->data.p;
}

static bool freemem_pre_remove (bintree_freemem_node_t *entry) {
	freemem_tree->remove (freemem_tree, entry);

	return true;
}

static bool freemem_post_add (bintree_freemem_node_t *entry) {
	// Fix the moved entry.
	freemem_fix_entry (entry);
	// Then insert it back into the tree.
	bintree_freemem_node_t *conflict = freemem_tree->insert (freemem_tree, entry);
	if (conflict) {
		kputs ("mm/freemem: Failed to do trivial insert while adding entry!\n");
		kpanic ();
	}

	return true;
}

static void freemem_rollback_remove (bintree_freemem_node_t *entry) {
	// Fix the moved entry.
	freemem_fix_entry (entry);
	// Then insert it back into the tree.
	bintree_freemem_node_t *conflict = freemem_tree->insert (freemem_tree, entry);
	if (conflict) {
		kputs (
			"mm/freemem: Failed to do trivial reinsert "
			"while rolling back removal of an entry!\n");
		kpanic ();
	}
}

static inline bintree_freemem_node_t *freemem_join_entries (
		bintree_freemem_node_t *entry1,
		bintree_freemem_node_t *entry2
) {
	bintree_freemem_node_t joined_entry_base =
		new_freemem_entry (freemem_join_regions (entry1->data, entry2->data));

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
static inline bintree_freemem_node_t *freemem_maybe_join (
		bintree_freemem_node_t *entry1,
		bintree_freemem_node_t *entry2
) {
	if (!freemem_consecutive_regions (entry1->data, entry2->data))
		return NULL;

	return freemem_join_entries (entry1, entry2);;
}

// Returns the entry that contains the region in the suppied entry,
// which may be the same pointer and may be the same entry.
static inline bintree_freemem_node_t *freemem_defrag_entry (bintree_freemem_node_t *entry) {
	bintree_freemem_iterator_t iterator_base, *iterator = &iterator_base;
	bintree_freemem_node_t *prev_entry, *next_entry, *new_entry;

	iterator_base = new_bintree_freemem_iterator (entry);
	if (iterator->prev (iterator)) {
		prev_entry = iterator->cur (iterator);

		new_entry = freemem_maybe_join (entry, prev_entry);
		if (new_entry)
			entry = new_entry;
	}

	iterator_base = new_bintree_freemem_iterator (entry);
	if (iterator->next (iterator)) {
		next_entry = iterator->cur (iterator);

		new_entry = freemem_maybe_join (entry, next_entry);
		if (new_entry)
			entry = new_entry;
	}

	return entry;
}

void freemem_init (void *internal, size_t internal_length) {
	freemem_tree_base = new_bintree_freemem ();
	freemem_entry_list_base = new_packed_list_freemem (
		internal,
		internal_length / sizeof(bintree_freemem_node_t),
		freemem_pre_remove,
		freemem_rollback_remove,
		freemem_post_add);
}

bool freemem_add_region (freemem_region_t region) {
	bintree_freemem_node_t entry_base = new_freemem_entry (region);

	const bool add_success = freemem_entry_list->append (freemem_entry_list, entry_base);
	if (!add_success)
		return false;

	freemem_defrag_entry (freemem_entry_list->get_last (freemem_entry_list));

	return true;
}

bool freemem_remove_region (freemem_region_t region) {
	bintree_freemem_iterator_t iterator_base, *iterator = &iterator_base;
	bintree_freemem_node_t *parent;
	void *region_end, *parent_region_end;

	parent = freemem_tree->search (freemem_tree, (size_t)region.p);

	if (!parent)
		return false;

	iterator_base = new_bintree_freemem_iterator (parent);

	if (region.p < parent->data.p) {
		while (parent && region.p < parent->data.p)
			parent = iterator->prev (iterator);

		if (!parent)
			return false;

		region_end = freemem_get_region_end (region);
		parent_region_end = freemem_get_region_end (parent->data);
	} else {
		while (parent) {
			region_end = freemem_get_region_end (region);
			parent_region_end = freemem_get_region_end (parent->data);

			if (region_end <= parent_region_end)
				break;

			parent = iterator->next (iterator);
		}

		if (!parent)
			return false;
	}

	if (!freemem_region_subset (parent->data, region))
		return false;

	unsigned char region_facts =
		(region.p   == parent->data.p    ? 1 : 0) |
		(region_end == parent_region_end ? 1 : 0) << 1;

	bool region_add_success;
	size_t old_length;
	bintree_freemem_node_t *conflict;

	switch (region_facts) {
		case 0b00: // region shares no edges with superset.
			old_length = parent->data.length;
			parent->data.length = region.p - parent->data.p;
			region_add_success = freemem_add_region (
				new_freemem_region (
					region_end, (size_t)(parent_region_end - region_end)));
			if (!region_add_success) {
				// Rollback.
				parent->data.length = old_length;
				return false;
			}
			break;
		case 0b01: // region shares the start with superset region.
			freemem_tree->remove (freemem_tree, parent);
			parent->data.p += region.length;
			parent->data.length -= region.length;
			freemem_fix_entry (parent);
			conflict = freemem_tree->insert (freemem_tree, parent);
			if (conflict) {
				kputs (
					"mm/freemem: Failed to do trivial reinsert while increasing "
					"start of free region as part of a remove region operation!\n");
				kpanic ();
			}
			break;
		case 0b10: // region shares the end with superset region.
			parent->data.length -= region.length;
			break;
		default: // region is superset region.
			// This will remove the element from the bintree in the pre_remove hook.
			freemem_entry_list->remove_elm (freemem_entry_list, parent);
	}

	return true;
}

freemem_region_t freemem_suggest (size_t length, size_t alignment, int offset) {
	packed_list_freemem_iterator_t iterator =
		new_packed_list_freemem_iterator (freemem_entry_list);
	bintree_freemem_node_t *entry;

	for (entry = iterator.cur (&iterator); entry; entry = iterator.next (&iterator)) {
		size_t align_inc;

		if ((size_t)entry->data.p % alignment)
			align_inc = alignment - (size_t)entry->data.p % alignment;
		else
			align_inc = 0;

		if (0 <= offset) {
			align_inc += offset;
		} else {
			if ((size_t)(-1 * offset) > align_inc)
				align_inc += alignment;
			align_inc -= -1 * offset;
		}

		if (length + align_inc > entry->data.length)
			continue;

		return new_freemem_region (
			entry->data.p + align_inc,
			length);
	}

	return new_freemem_region ((void *)0x0, 0);
}

// vim: set ts=4 sw=4 noet syn=c:
