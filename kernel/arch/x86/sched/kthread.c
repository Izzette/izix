// kernel/arch/x86/sched/kthread.c

#include <stddef.h>
#include <stdbool.h>

#include <string.h>
#include <collections/bintree.h>
#include <collections/linked_list.h>

#include <asm/halt.h>
#include <mm/freemem.h>
#include <mm/malloc.h>
#include <mm/page.h>
#include <kpanic/kpanic.h>
#include <kprint/kprint.h>
#include <sched/kthread_switch.h>
#include <sched/kthread_bootstrap.h>
#include <sched/kthread.h>
#include <sched/spinlock.h>

typedef struct kthread_lock_struct {
	spinlock_t spinlock;
	size_t depth;
} kthread_lock_t;

typedef struct kthread_struct {
	kpid_t kpid;
	kpid_t parent;
	freemem_region_t stack_region;
	kthread_registers_t task;
	kthread_lock_t lock;
} kthread_t;

typedef struct __attribute__((packed)) zero_width_struct {
} zero_width_t;

static const zero_width_t zero_wide;

#pragma GCC diagnostic ignored "-Wstrict-aliasing"
TPL_LINKED_LIST(kthread, volatile kthread_t);
#pragma GCC diagnostic pop
TPL_BINTREE(kpid, zero_width_t);
TPL_BINTREE(kthread, linked_list_kthread_node_t *);

static bool kthread_init_record = false;

static volatile linked_list_kthread_t
	kthreads_active_base,
	*kthreads_active = &kthreads_active_base;
static volatile linked_list_kthread_t
	kthreads_destroy_base,
	*kthreads_destroy = &kthreads_destroy_base;

static volatile bintree_kthread_t
	kthreads_blocking_base,
	*kthreads_blocking = &kthreads_blocking_base;

static volatile bintree_kpid_t
	kpids_free_base,
	*kpids_free = &kpids_free_base;

volatile linked_list_kthread_node_t *volatile kthread_running_node = NULL;
volatile kpid_t kthread_destroy_task_kpid = -1;
volatile kpid_t kthread_idle_task_kpid = -1;

static inline kthread_t new_kthread (
		kpid_t kpid,
		kpid_t parent,
		freemem_region_t stack_region
) {
	kthread_t kthread = {
		.kpid = kpid,
		.parent = parent,
		.stack_region = stack_region,
		.task = new_kthread_task (freemem_get_region_end (stack_region)),
		.lock = {
			.spinlock = new_spinlock (),
			.depth = 0
		},
	};

	return kthread;
}

static inline volatile kthread_t *kthread_get_running_thread () {
	return &kthread_running_node->data;
}

static inline volatile kthread_registers_t *kthread_get_running_task () {
	volatile kthread_t *kthread_running_thread = kthread_get_running_thread ();

	return &kthread_running_thread->task;
}

static inline volatile kthread_lock_t *kthread_get_running_lock () {
	volatile kthread_t *kthread_running_thread = kthread_get_running_thread ();

	return &kthread_running_thread->lock;
}

static inline freemem_region_t kthread_stack_alloc () {
	freemem_region_t stack_region = freemem_suggest (KTHREAD_STACK_SIZE, PAGE_SIZE, 0);
	if (!stack_region.length)
		return new_freemem_region (NULL, 0);

	const bool remove_success = freemem_remove_region (stack_region);
	if (!remove_success) {
		kputs (
			"sched/kthread: Failed to trivially remove "
			"freemem-suggested stack region!\n");
		kpanic ();
	}

	return stack_region;
}

static inline void kthread_stack_free (freemem_region_t stack_region) {
	const bool add_success = freemem_add_region (stack_region);
	if (!add_success) {
		kputs ("sched/kthread: Failed to deallocate kthread stack region!\n");
		kpanic ();
	}
}

static inline kpid_t kthread_pop_free_kpid () {
	static kpid_t last_kpid = 1;

	bintree_kpid_node_t *kpid_parent_node = kpids_free->search (
		(bintree_kpid_t *)kpids_free, last_kpid);

	if (!kpid_parent_node)
		return -1;

	if (last_kpid > (kpid_t)kpid_parent_node->node.orderby) {
		bintree_kpid_iterator_t iterator_base, *iterator = &iterator_base;

		kpid_parent_node = iterator->next (iterator);

		if (!kpid_parent_node)
			kpid_parent_node = kpids_free->min ((bintree_kpid_t *)kpids_free);
	}

	bintree_kpid_node_t *kpid_node = kpid_parent_node;
	kpid_t kpid = kpid_node->node.orderby;

	kpids_free->remove ((bintree_kpid_t *)kpids_free, kpid_node);
	free (kpid_node);

	last_kpid = (kpid + 1) % KTHREAD_MAX_PROCS;

	return kpid;
}

static inline linked_list_kthread_node_t *kthread_create_thread (
		kpid_t kpid,
		kpid_t parent
) {
	freemem_region_t stack_region = kthread_stack_alloc ();
	if (!stack_region.length) {
		kputs ("sched/kthread: Failed to allocate new kthread stack!\n");
		kpanic ();
	}

	kthread_t kthread = new_kthread (kpid, parent, stack_region);

	linked_list_kthread_node_t *kthread_node =
		malloc (sizeof(linked_list_kthread_node_t));
	if (!kthread_node) {
		kputs ("sched/kthread: Failed to allocate new kthread!\n");
		kpanic ();
	}

	*kthread_node = new_linked_list_kthread_node (kthread);

	return kthread_node;
}

static inline linked_list_kthread_node_t *kthread_create_main_thread (
		freemem_region_t main_stack_region
) {
	linked_list_kthread_node_t *main_kthread_node =
		malloc (sizeof(linked_list_kthread_node_t));
	if (!main_kthread_node) {
		kputs ("sched/kthread: Failed to allocate main kthread!\n");
		kpanic ();
	}

	kthread_t main_kthread = new_kthread (0, -1, main_stack_region);
	*main_kthread_node = new_linked_list_kthread_node (main_kthread);

	return main_kthread_node;
}

static inline bintree_kpid_node_t *kthread_kpid_node_alloc (kpid_t kpid) {
	bintree_kpid_node_t *kpid_node = malloc (sizeof(bintree_kpid_node_t));
	if (!kpid_node) {
		kputs ("sched/kthread: Failed to allocate free kpid entry!\n");
		kpanic ();
	}

	*kpid_node = new_bintree_kpid_node (zero_wide, kpid);

	return kpid_node;
}

// Must already be out of kthreads_active/blocking queue and into kthreads_destroy queue.
static inline void kthread_destroy_thread (linked_list_kthread_node_t *kthread_node) {
	// Free stack ASAP
	kthread_stack_free (kthread_node->data.stack_region);

	kpid_t kpid = kthread_node->data.kpid;

	kthreads_destroy->removeNode ((linked_list_kthread_t *)kthreads_destroy, kthread_node);

	bintree_kpid_node_t *kpid_node = kthread_kpid_node_alloc (kpid);

	kpids_free->insert ((bintree_kpid_t *)kpids_free, kpid_node);
}

static inline void kthread_next_task (volatile kthread_registers_t *this_task) {
	volatile linked_list_kthread_node_t *next_kthread_node =
		kthreads_active->pop ((linked_list_kthread_t *)kthreads_active);

	if (next_kthread_node) {
		// Increment lock depth on next task before swapping with kthread_running_node.
		volatile kthread_lock_t *next_kthread_lock = &next_kthread_node->data.lock;
		spinlock_try_lock (&next_kthread_lock->spinlock);
		next_kthread_lock->depth += 1;

		kthread_running_node = next_kthread_node;

		kthread_switch (this_task, kthread_get_running_task ());
	} else {
		// If there is nothing to do wake idle task and run it.
		const bool idle_wake_success = kthread_wake (kthread_idle_task_kpid);
		if (!idle_wake_success) {
			kputs (
				"sched/kthread: Failed to wake supposedly "
				"blocking kthread idle bacgkround task!\n");
			kpanic ();
		}

		kthread_next_task (this_task);
	}
}

static inline void kthread_fill_free_kpids () {
	kpid_t kpid;

	// Skip kernel main and init.
	for (kpid = 2; KTHREAD_MAX_PROCS > kpid; ++kpid) {
		bintree_kpid_node_t kpid_node_base, *kpid_node = &kpid_node_base;
		kpid_node = kthread_kpid_node_alloc (kpid);
		kpids_free->insert ((bintree_kpid_t *)kpids_free, kpid_node);
	}

}

static inline void kthread_add_blocking_task (
		volatile linked_list_kthread_node_t *kthread_node
) {
	bintree_kthread_node_t *kthread_blocking_node =
		malloc (sizeof(bintree_kthread_node_t));
	if (!kthread_blocking_node) {
		kputs ("sched/kthread: Failed to allocate new blocking node!\n");
		kpanic ();
	}

	*kthread_blocking_node = new_bintree_kthread_node (
		(linked_list_kthread_node_t *)kthread_node,
		kthread_node->data.kpid);

	kthreads_blocking->insert (
		(bintree_kthread_t *)kthreads_blocking,
		kthread_blocking_node);
}

static void kthread_background_task_idle () {
	kputs ("sched/kthread: Started idle background task.\n");

	for (;;) {
		// Then just halt, and block until needed again.
		halt ();

		kthread_block ();
	}
}

static void kthread_background_task_destroy () {
	linked_list_kthread_iterator_t
		iterator_base,
		*iterator = &iterator_base;

	kputs ("sched/kthread: Started kthread destruction background task.\n");

	// No spontaneous task switching for this task.
	kthread_lock_task ();

	for (;;) {
		// Not safe to use reset here, becuase the iterator does not use volatile links.
		iterator_base = kthreads_destroy->new_iterator (
			(linked_list_kthread_t *)kthreads_destroy);

		linked_list_kthread_node_t *kthread_node;

		kthread_node = iterator->cur (iterator);
		while (kthread_node) {
			// Need to obtain the next node first, because kthread_destroy_thread will
			// deallocate and kill it's links.
			linked_list_kthread_node_t *kthread_next_node = iterator->next (iterator);

			kthread_destroy_thread (kthread_node);

			kthread_node = kthread_next_node;
		}

		kthread_block ();
	}
}

void kthread_init (freemem_region_t main_stack_region) {
	*kthreads_active = new_linked_list_kthread ();
	*kthreads_destroy = new_linked_list_kthread ();

	*kthreads_blocking = new_bintree_kthread ();

	*kpids_free = new_bintree_kpid ();

	kthread_fill_free_kpids ();

	// Create main task
	linked_list_kthread_node_t *main_kthread_node =
		kthread_create_main_thread (main_stack_region);
	kthread_running_node = main_kthread_node;

	// HACK! Switch tasks back to main.
	kthread_lock_task (); // kthread_switch will unlock task.
	kthread_switch (kthread_get_running_task (), kthread_get_running_task ());

	kthread_init_record = true;

	// Start the idle background task in the blocking state.
	kthread_idle_task_kpid =
		kthread_new_blocking_task (kthread_background_task_idle);
	if (0 > kthread_idle_task_kpid) {
		kputs ("sched/kthread: Failed to create idle background task!\n");
		kpanic ();
	}

	// Start the destruction background task in the blocking state.
	kthread_destroy_task_kpid =
		kthread_new_blocking_task (kthread_background_task_destroy);
	if (0 > kthread_destroy_task_kpid) {
		kputs ("sched/kthread: Failed to create destroy background task!\n");
		kpanic ();
	}

	kputs ("sched/kthread: Successfuly initalized kthreads.\n");
}

bool kthread_is_init () {
	return kthread_init_record;
}

kpid_t kthread_new_task (void (*task) ()) {
	kthread_lock_task ();

	kpid_t new_kpid = kthread_pop_free_kpid ();
	if (0 > new_kpid)
		return -1;

	linked_list_kthread_node_t *new_kthread_node =
		kthread_create_thread (new_kpid, kthread_get_running_kpid ());

	volatile kthread_registers_t *new_task = &new_kthread_node->data.task;

	// Our bootstrap code, the "glue" that keeps the stack backtraceable.
#pragma GCC diagnostic ignored "-Wpointer-to-int-cast"
	kthread_task_pushl ((kthread_registers_t *)new_task, (uint32_t)task);
#pragma GCC diagnostic pop
	kthread_task_pushl ((kthread_registers_t *)new_task, 0);
	kthread_task_push_caller ((kthread_registers_t *)new_task, kthread_bootstrap);

	kthreads_active->append ((linked_list_kthread_t *)kthreads_active, new_kthread_node);

	kthread_unlock_task ();

	return new_kpid;
}

// Start task in the blocking state.
kpid_t kthread_new_blocking_task (void (*task) ()) {
	kthread_lock_task ();

	kpid_t new_blocking_kpid = kthread_new_task (task);
	// Gurenteed to be at end of active queue.
	linked_list_kthread_node_t *new_blocking_kthread_node =
		kthreads_active->popEnd ((linked_list_kthread_t *)kthreads_active);

	kthread_add_blocking_task (new_blocking_kthread_node);

	kthread_unlock_task ();

	return new_blocking_kpid;
}

// Task actually can end if locked, because kthread_end_task should never return.
void kthread_end_task () {
	// Lock until task switch.
	kthread_lock_task ();

	kthreads_destroy->append (
		(linked_list_kthread_t *)kthreads_destroy,
		(linked_list_kthread_node_t *)kthread_running_node);

	// We don't care if it was already sleeping or not.
	kthread_wake (kthread_destroy_task_kpid);

	kthread_registers_t
		ignored_task_base,
		*ignored_task = &ignored_task_base;

	for (;;)  // Avoid compiler warning about noreturn functions returning.
		kthread_next_task (ignored_task);
}

void kthread_yeild () {
	kthread_lock_task ();

	kthreads_active->append (
		(linked_list_kthread_t *)kthreads_active,
		(linked_list_kthread_node_t *)kthread_running_node);

	kthread_next_task (kthread_get_running_task ());

	kthread_unlock_task ();
}

bool kthread_wake (kpid_t kpid) {
	kthread_lock_task ();

	bintree_kthread_node_t *kthread_blocking_node =
		kthreads_blocking->search ((bintree_kthread_t *)kthreads_blocking, kpid);

	if (!kthread_blocking_node)
		return false;

	linked_list_kthread_node_t *kthread_node = kthread_blocking_node->data;

	if (kpid != kthread_node->data.kpid)
		return false;

	kthreads_blocking->remove (
		(bintree_kthread_t *)kthreads_blocking,
		kthread_blocking_node);
	free (kthread_blocking_node);

	kthreads_active->append (
		(linked_list_kthread_t *)kthreads_active,
		kthread_node);

	kthread_unlock_task ();

	return true;
}

void kthread_block () {
	kthread_lock_task ();

	kthread_add_blocking_task (kthread_running_node);

	kthread_next_task (kthread_get_running_task ());

	kthread_unlock_task ();
}

void kthread_lock_task () {
	volatile kthread_lock_t *running_lock = kthread_get_running_lock ();

	// We don't care whether or not the lock has been obtained because tasks can't switch
	// if the task lock has been obtained.
	spinlock_try_lock (&running_lock->spinlock);
	running_lock->depth += 1;
}

void kthread_unlock_task () {
	volatile kthread_lock_t *running_lock = kthread_get_running_lock ();

	if (1 >= running_lock->depth) {
		running_lock->depth = 0;
		spinlock_release (&running_lock->spinlock);
		return;
	}

	running_lock->depth -= 1;
}

bool kthread_task_is_locked () {
	volatile kthread_lock_t *running_lock = kthread_get_running_lock ();

	return 0 != running_lock->depth;
}

kpid_t kthread_get_running_kpid () {
	kthread_lock_task ();

	volatile kthread_t *running_thread = kthread_get_running_thread ();

	kpid_t kpid = running_thread->kpid;

	kthread_unlock_task ();

	return kpid;
}

// vim: set ts=4 sw=4 noet syn=c:
