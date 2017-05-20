// kernel/mm/freemem.c

#include <stdbool.h>

#include <collections/bintree.h>
#include <collections/sparse_collection.h>

#include <kprint/kprint.h>
#include <kpanic/kpanic.h>
#include <mm/freemem.h>
#include <sched/kthread.h>

// TODO: By limitation of size_t and the choice to use the exclusive maximum of regions
//       the last byte of the addressable memory space  cannot be used, as the exclusive
//       maximum will read as zero.  This will likely never be an issue, because there is
//       (almost?) always a reserved memory region leading up to that byte, but if it were
//       to occur, it would realistically result of the loss of a complete page of memory.
//       Perhaps, this should be solved of sooner rather than later ...

TPL_BINTREE (freemem, freemem_region_t)
TPL_SPARSE_COLLECTION(freemem, bintree_freemem_node_t)

static bintree_freemem_t freemem_tree_base;
static bintree_freemem_t *freemem_tree = &freemem_tree_base;
static sparse_collection_freemem_t freemem_entries_base;
static sparse_collection_freemem_t *freemem_entries = &freemem_entries_base;

static bool freemem_consecutive_regions (
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

static bool freemem_region_subset (
		freemem_region_t set,
		freemem_region_t subset
) {
	if (!(set.p <= subset.p))
		return false;
	if (!(freemem_get_region_end (set) >= freemem_get_region_end (subset)))
		return false;

	return true;
}

static bintree_freemem_node_t new_freemem_entry (
		freemem_region_t region
) {
	bintree_freemem_node_t entry = new_bintree_freemem_node (
		region,
		(size_t)region.p
	);

	return entry;
}

static void freemem_fix_entry (bintree_freemem_node_t *entry) {
	entry->orderby = (size_t)entry->data.p;
}

static bintree_freemem_node_t *freemem_join_entries (
		bintree_freemem_node_t *entry1,
		bintree_freemem_node_t *entry2
) {
	const size_t new_length = entry1->data.length + entry2->data.length;
	bintree_freemem_node_t *low_entry, *high_entry;

	if (entry1->data.p < entry2->data.p) {
		low_entry = entry1;
		high_entry = entry2;
	} else {
		high_entry = entry1;
		low_entry = entry2;
	}

	freemem_tree->remove (freemem_tree, high_entry);
	freemem_entries->free (freemem_entries, high_entry);

	low_entry->data.length = new_length;

	return low_entry;
}

// returns joined entry, NULL if not joined.
static bintree_freemem_node_t *freemem_maybe_join (
		bintree_freemem_node_t *entry1,
		bintree_freemem_node_t *entry2
) {
	if (!freemem_consecutive_regions (entry1->data, entry2->data))
		return NULL;

	return freemem_join_entries (entry1, entry2);;
}

// Returns the entry that contains the region in the suppied entry,
// which may be the same pointer and may be the same entry.
static bintree_freemem_node_t *freemem_defrag_entry (bintree_freemem_node_t *entry) {
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

static bool freemem_add_region_internal (freemem_region_t region) {
	bintree_freemem_node_t entry_base = new_freemem_entry (region);

	const size_t i = freemem_entries->get (freemem_entries);
	if (!i)
		return false;

	bintree_freemem_node_t *node = freemem_entries->alloc (freemem_entries, i);
	*node = entry_base;

	bintree_freemem_node_t *conflict = freemem_tree->insert (freemem_tree, node);
	if (conflict)
		return false;

	freemem_defrag_entry (node);

	return true;
}

static bool freemem_remove_region_internal (freemem_region_t region) {
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
			region_add_success = freemem_add_region_internal (
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
			freemem_tree->remove (freemem_tree, parent);
			freemem_entries->free (freemem_entries, parent);
	}

	return true;
}

static freemem_region_t freemem_suggest_internal (
		size_t length,
		size_t alignment,
		int offset
) {
	bintree_freemem_iterator_t iterator = freemem_tree->new_iterator (freemem_tree);
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

void freemem_init (void *internal, size_t internal_length) {
	kthread_lock_task ();

	freemem_tree_base = new_bintree_freemem ();
	freemem_entries_base = new_sparse_collection_freemem (internal, internal_length);

	kthread_unlock_task ();

}

bool freemem_add_region (freemem_region_t region) {
	kthread_lock_task ();

	const bool ret = freemem_add_region_internal (region);

	kthread_unlock_task ();

	return ret;
}

bool freemem_remove_region (freemem_region_t region) {
	kthread_lock_task ();

	const bool ret = freemem_remove_region_internal (region);

	kthread_unlock_task ();

	return ret;
}

freemem_region_t freemem_suggest (size_t length, size_t alignment, int offset) {
	kthread_lock_task ();

	const freemem_region_t ret = freemem_suggest_internal (length, alignment, offset);

	kthread_unlock_task ();

	return ret;
}

// vim: set ts=4 sw=4 noet syn=c:
