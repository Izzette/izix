// kernel/mm/freemem.c

#include <stdbool.h>

#include <collections.h>

#include <kprint/kprint.h>
#include <kpanic/kpanic.h>
#include <mm/freemem.h>

typedef struct freemem_entry_struct {
	freemem_region_t region;
	bintree_node_t node_base;
} freemem_entry_t;

static bintree_t freemem_tree_base;
static bintree_t *freemem_tree = &freemem_tree_base;
static freemem_entry_t *freemem_entries;
static size_t freemem_entry_max_length;
static size_t freemem_entry_count = 0;

static inline freemem_entry_t new_freemem_entry (
		freemem_region_t region,
		bintree_node_t node_base
) {
	freemem_entry_t entry = {
		.region = region,
		.node_base = node_base
	};

	return entry;
}

static inline freemem_entry_t *freemem_get_last_entry () {
	return freemem_entries + freemem_entry_count - 1;
}

static inline bool freemem_is_last_entry (freemem_entry_t *entry) {
	return entry == freemem_get_last_entry ();
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

static inline void freemem_move_entry (
		freemem_entry_t *entry,
		freemem_entry_t *new_location
) {
	// Remove it from the old location.
	bintree_remove_node (freemem_tree, freemem_get_node (entry));
	// Then move the data.
	new_location->region = entry->region;
	// Fix the moved entry.
	freemem_fix_entry (new_location);
	// Then insert it back into the tree.
	bintree_node_t *conflict = bintree_insert_node (
			freemem_tree, freemem_get_node (new_location));
	if (conflict) {
		kputs ("mm/freemem: Failed to do trivial reinsert while moving an entry!\n");
		kpanic ();
	}
}

static inline void freemem_move_last_entry (freemem_entry_t *new_location) {
	freemem_move_entry (freemem_get_last_entry (), new_location);
}

static inline freemem_entry_t *freemem_join_entries (
		freemem_entry_t *entry1,
		freemem_entry_t *entry2
) {
	freemem_region_t joined_region =
		freemem_join_regions (entry1->region, entry2->region);

	freemem_region_t *new_region;
	freemem_entry_t *new_entry;
	bintree_node_t *node1, *node2, *new_node;

	node1 = freemem_get_node (entry1);
	node2 = freemem_get_node (entry2);

	bintree_remove_node (freemem_tree, node1);
	bintree_remove_node (freemem_tree, node2);

	unsigned char last_entry_facts =
		(freemem_is_last_entry (entry1) ? 1 : 0) |
		(freemem_is_last_entry (entry2) ? 1 : 0) << 1;

	switch (last_entry_facts) {
		default: // Neither were last
			freemem_move_last_entry (entry1);
		case 0b01: // Entry 1 was last.
			new_entry = entry2;
			break;
		case 0b10: // Entry 2 was last.
			new_entry = entry1;
			break;
	}

	new_node = freemem_get_node (new_entry);
	new_region = &new_entry->region;

	*new_region = joined_region;
	*new_node = new_bintree_node ((size_t)new_region->p, new_entry);

	bintree_node_t *conflict = bintree_insert_node (freemem_tree, new_node);
	if (conflict) {
		kputs ("mm/freemem: Failed to insert joined node!\n");
		kpanic ();
	}

	// We have removed two then inserting one, so our count decrements by one.
	freemem_entry_count -= 1;

	return new_entry;
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
	freemem_entries = internal;
	freemem_entry_max_length = internal_length / sizeof(freemem_region_t);
}

bool freemem_add_region (freemem_region_t region) {
	if (freemem_entry_max_length < freemem_entry_count)
		return false;

	freemem_entry_t *entry = freemem_entries + freemem_entry_count;

	bintree_node_t node_base = new_bintree_node ((size_t)region.p, entry);
	freemem_entry_t entry_base = new_freemem_entry (region, node_base);

	*entry = entry_base;

	bintree_node_t *node, *conflict;

	node = freemem_get_node (entry);

	conflict = bintree_insert_node (freemem_tree, node);
	if (conflict)
		return false;

	// commit
	freemem_entry_count += 1;

	freemem_defrag_entry (entry);

	return true;
}

bool freemem_remove_region (freemem_region_t region) {
	if (0 == freemem_entry_count)
		return false;

	// Parent is guaranteed to be non-NULL because freemem_entry_count is non-zero.
	bintree_node_t *parent = bintree_search (freemem_tree, (size_t)region.p);
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
			bintree_remove_node (freemem_tree, parent);
			freemem_move_last_entry (superset_entry);
			freemem_entry_count -= 1;
	}

	return true;
}

freemem_region_t freemem_suggest (size_t length, size_t alignment, int offset) {
	size_t i = freemem_entry_count;

	while (i--) {
		size_t align_inc;

		if ((size_t)freemem_entries[i].region.p % alignment)
			align_inc = alignment - (size_t)freemem_entries[i].region.p % alignment;
		else
			align_inc = 0;

		if (0 <= offset) {
			align_inc += offset;
		} else {
			if ((size_t)(-1 * offset) > align_inc)
				align_inc += alignment;
			align_inc -= -1 * offset;
		}

		if (length + align_inc > freemem_entries[i].region.length)
			continue;

		return new_freemem_region (
			freemem_entries[i].region.p + align_inc,
			length);
	}

	return new_freemem_region ((void *)0x0, 0);
}

// vim: set ts=4 sw=4 noet syn=c:
