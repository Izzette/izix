// kernel/dev/dev.c

#include <collections/bintree.h>

#include <sched/mutex.h>
#include <mm/malloc.h>
#include <kprint/kprint.h>
#include <kpanic/kpanic.h>
#include <dev/dev.h>
#include <dev/dev_types.h>
#include <dev/dev_driver.h>

TPL_BINTREE(min, dev_driver_t *)
TPL_BINTREE(maj, bintree_min_fields_t)

static mutex_t
	dev_mutex_base,
	*dev_mutex = &dev_mutex_base;

static bintree_maj_t
	dev_maj_tree_base,
	*dev_maj_tree = &dev_maj_tree_base;

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

static void dev_maj_remove (dev_maj_t maj) {
	bintree_maj_node_t *maj_node = dev_maj_tree->search (dev_maj_tree, maj);
	if (!maj_node) {
		kputs ("dev/dev_driver: Failed to find major node while removing!\n");
		kpanic ();
	}

	free (maj_node);
}

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

static void dev_min_remove (bintree_min_t *min_tree, dev_min_t min) {
	bintree_min_node_t *min_node = min_tree->search (min_tree, min);
	if (!min_node) {
		kputs ("dev/dev_driver: Failed to find minor node while removing!\n");
		kpanic ();
	}

	free (min_node);
}

__attribute__((constructor))
void dev_construct () {
	dev_mutex_base = new_mutex ();
	dev_maj_tree_base = new_bintree_maj ();
}

void dev_add (dev_driver_t *dev_driver) {
	mutex_lock (dev_mutex);

	bintree_maj_node_t *maj_node =
		dev_maj_tree->search (dev_maj_tree, dev_driver->dev.maj);
	if (!maj_node || dev_driver->dev.maj != maj_node->orderby)
		maj_node = dev_maj_add (dev_driver->dev.maj);

	bintree_min_t
		min_tree_base,
		*min_tree = &min_tree_base;

	min_tree_base = new_bintree_min_from_fields (maj_node->data);
	dev_min_add (min_tree, dev_driver);
	maj_node->data = min_tree->get_fields (min_tree);

	mutex_release (dev_mutex);
}

void dev_remove (dev_t dev) {
	mutex_lock (dev_mutex);

	bintree_maj_node_t *maj_node =
		dev_maj_tree->search (dev_maj_tree, dev.maj);
	if (!maj_node) {
		kputs (
			"dev/dev_driver: Failed to find coresponding major node while removing"
			"a dev!\n");
		kpanic ();
	}

	bintree_min_t
		min_tree_base,
		*min_tree = &min_tree_base;

	min_tree_base = new_bintree_min_from_fields (maj_node->data);
	dev_min_remove (min_tree, dev.min);

	if (min_tree->root)
		maj_node->data = min_tree->get_fields (min_tree);
	else
		dev_maj_remove (dev.maj);

	mutex_release (dev_mutex);
}

// vim: set ts=4 sw=4 noet syn=c:
