// kernel/dev/dev.c

#include <collections/bintree.h>

#include <sched/mutex.h>
#include <mm/malloc.h>
#include <kprint/kprint.h>
#include <kpanic/kpanic.h>
#include <dev/dev.h>
#include <dev/dev_types.h>
#include <dev/dev_driver.h>
#include <mm/page.h>
#include <mm/paging.h>

// Many of these routines are unessesarily large, using __attribute__((optimize("Os")))
// to reduce bloat.

TPL_BINTREE(min, dev_driver_t *)
TPL_BINTREE(maj, bintree_min_fields_t)

static mutex_t
	dev_mutex_base,
	*dev_mutex = &dev_mutex_base;

static bintree_maj_t
	dev_maj_tree_base,
	*dev_maj_tree = &dev_maj_tree_base;

static bintree_min_node_t *dev_get (dev_t dev, bintree_maj_node_t **maj_node_ptr) {
	*maj_node_ptr = dev_maj_tree->search (dev_maj_tree, dev.maj);
	if (!*maj_node_ptr || (size_t)dev.maj != (*maj_node_ptr)->orderby) {
		*maj_node_ptr = NULL;
		return NULL;
	}

	bintree_min_t
		min_tree_base,
		*min_tree = &min_tree_base;

	min_tree_base = new_bintree_min_from_fields ((*maj_node_ptr)->data);

	bintree_min_node_t *min_node = min_tree->search (min_tree, dev.min);
	if (!min_node || (size_t)dev.min != min_node->orderby)
		return NULL;

	return min_node;
}

__attribute__((optimize("Os")))
static bintree_maj_node_t *dev_maj_add (dev_maj_t maj) {
	bintree_maj_node_t *maj_node = malloc (sizeof(bintree_maj_node_t));
	if (!maj_node) {
		kputs ("dev/dev_driver: Failed to allocate new major node!\n");
		kpanic ();
	}

	bintree_min_fields_t min_fields = new_bintree_min_fields ();
	*maj_node = new_bintree_maj_node (min_fields, maj);

	bintree_maj_node_t *conflict = dev_maj_tree->insert (dev_maj_tree, maj_node);
	if (conflict) {
		kputs ("dev/dev_driver: Failed to insert supposedly missing major node!\n");
		kpanic ();
	}

	return maj_node;
}

__attribute__((optimize("Os")))
static void dev_maj_remove (bintree_maj_node_t *maj_node) {
	dev_maj_tree->remove (dev_maj_tree, maj_node);
	free (maj_node);
}

__attribute__((optimize("Os")))
static bintree_min_node_t *dev_min_add (bintree_min_t *min_tree, dev_driver_t *driver) {
	bintree_min_node_t *min_node = malloc (sizeof(bintree_min_node_t));
	if (!min_node) {
		kputs ("dev/dev_driver: Failed to allocate new minor node!\n");
		kpanic ();
	}

	*min_node = new_bintree_min_node (driver, driver->dev.min);

	bintree_min_node_t *conflict = min_tree->insert (min_tree, min_node);
	if (conflict) {
		kputs ("dev/dev_driver: Failed to insert supposedly missing minor node!\n");
		kpanic ();
	}

	return min_node;
}

__attribute__((optimize("Os")))
static void dev_min_remove (bintree_min_t *min_tree, bintree_min_node_t *min_node) {
	min_tree->remove (min_tree, min_node);
	free (min_node);
}

__attribute__((constructor))
void dev_construct () {
	dev_mutex_base = new_mutex ();
	dev_maj_tree_base = new_bintree_maj ();
}

__attribute__((optimize("Os")))
void dev_add (dev_driver_t *dev_driver) {
	mutex_lock (dev_mutex);

	bintree_maj_node_t *maj_node;
	bintree_min_node_t *min_node = dev_get (dev_driver->dev, &maj_node);
	if (!maj_node)
		maj_node = dev_maj_add (dev_driver->dev.maj);
	if (min_node) {
		kputs ("dev/dev_driver: Attempted to add already present device!\n");
		kpanic ();
	}

	bintree_min_t
		min_tree_base,
		*min_tree = &min_tree_base;

	min_tree_base = new_bintree_min_from_fields (maj_node->data);
	dev_min_add (min_tree, dev_driver);
	maj_node->data = min_tree->get_fields (min_tree);

	mutex_release (dev_mutex);
}

__attribute__((optimize("Os")))
void dev_remove (dev_t dev) {
	mutex_lock (dev_mutex);

	bintree_maj_node_t *maj_node;
	bintree_min_node_t *min_node = dev_get (dev, &maj_node);
	if (!maj_node) {
		kputs (
			"dev/dev_driver: Failed to find coresponding major node while removing"
			"a dev!\n");
		kpanic ();
	}
	if (min_node) {
		kputs ("dev/dev_driver: Attempted to remove already absent device!\n");
		kpanic ();
	}

	bintree_min_t
		min_tree_base,
		*min_tree = &min_tree_base;

	min_tree_base = new_bintree_min_from_fields (maj_node->data);

	dev_min_remove (min_tree, min_node);

	if (min_tree->root)
		maj_node->data = min_tree->get_fields (min_tree);
	else
		dev_maj_remove (maj_node);

	mutex_release (dev_mutex);
}

__attribute__((optimize("Os")))
void dev_map (dev_t dev, paging_data_t *paging_data) {
	mutex_lock (dev_mutex);

	bintree_maj_node_t *maj_node;
	bintree_min_node_t *min_node = dev_get (dev, &maj_node);
	if (!maj_node) {
		kputs (
			"dev/dev_driver: Failed to find coresponding major node while mapping"
			"a device!\n");
		kpanic ();
	}
	if (!min_node) {
		kputs ("dev/dev_driver: Attempt to map absent device!\n");
		kpanic ();
	}

	dev_driver_t *driver = min_node->data;

	page_t *pg = NULL;
	bool need_write;
	while ((pg = driver->next_page_mapping (driver, pg, &need_write))) {
#if !defined(IZIX) || defined(ARCH_X86)
		page_attrs_t compat_attrs = {
			.present = true,
			.writable = need_write,
			.user = false,
			.write_through = true,
			.cache_disabled = true,
			.accessed = false,
			.dirty = false,
			.global = false
		};
#endif

		if (paging_table_present (pg, paging_data)) {
			page_attrs_t old_attrs = paging_get_attrs (pg, paging_data);
			compat_attrs = paging_compatable_attrs (compat_attrs, old_attrs);
		}

		paging_set_map (pg, pg, paging_data);
		paging_set_attrs (pg, compat_attrs, paging_data);
	}

	mutex_release (dev_mutex);
}

__attribute__((optimize("Os")))
void dev_map_all (paging_data_t *paging_data) {
	bintree_maj_iterator_t
		maj_iterator_base,
		*maj_iterator = &maj_iterator_base;

	maj_iterator_base = dev_maj_tree->new_iterator (dev_maj_tree);

	bintree_maj_node_t *maj_node = maj_iterator->cur (maj_iterator);
	while (maj_node) {
		bintree_min_t
			min_tree_base,
			*min_tree = &min_tree_base;
		bintree_min_iterator_t
			min_iterator_base,
			*min_iterator = &min_iterator_base;

		min_tree_base = new_bintree_min_from_fields (maj_node->data);

		min_iterator_base = min_tree->new_iterator (min_tree);

		bintree_min_node_t *min_node = min_iterator->cur (min_iterator);
		while (min_node) {
			dev_t dev = {
				.maj = maj_node->orderby,
				.min = min_node->orderby
			};

			dev_map (dev, paging_data);

			min_node = min_iterator->next (min_iterator);
		}

		maj_node = maj_iterator->next (maj_iterator);
	}
}

// vim: set ts=4 sw=4 noet syn=c:
