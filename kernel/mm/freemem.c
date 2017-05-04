// kernel/mm/freemem.c

#include <stdbool.h>

#include <collections.h>

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

static inline freemem_entry_t new_freemem_entry (freemem_region_t region) {
	freemem_entry_t entry = {
		.region = region,
		.node_base = new_bintree_node (0, 0)
	};

	return entry;
}

static inline bintree_node_t *freemem_get_node (freemem_entry_t *entry) {
	return &entry->node_base;
}

static inline freemem_entry_t *freemem_get_last_entry () {
	return freemem_entries + freemem_entry_count - 1;
}

static inline bool freemem_is_last_entry (freemem_entry_t *entry) {
	return entry == freemem_get_last_entry ();
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
	bintree_insert_node (freemem_tree, freemem_get_node (new_location));
}

static inline void freemem_move_last_entry (freemem_entry_t *new_location) {
	freemem_move_entry (freemem_get_last_entry (), new_location);
}

static inline bool freemem_consecutive_regions (
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

static inline freemem_region_t freemem_join_regions (
		freemem_region_t region1,
		freemem_region_t region2
) {
	void *new_p = (region1.p < region2.p) ? region1.p : region2.p;
	const size_t new_length = region1.length + region2.length;

	freemem_region_t joined_region = {
		.p = new_p,
		.length = new_length
	};

	return joined_region;
}

static inline freemem_entry_t *freemem_join_entries (
		freemem_entry_t *entry1,
		freemem_entry_t *entry2
) {

	// We will be removing two then inserting one, so our count decrements by one.
	freemem_entry_count -= 1;

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

	bintree_insert_node (freemem_tree, new_node);

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

	freemem_entry_t entry_base = {
		.region = region,
		.node_base = node_base
	};

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

// vim: set ts=4 sw=4 noet syn=c:
