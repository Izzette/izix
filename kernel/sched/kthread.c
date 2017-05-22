// kernel/sched/kthread.c

#include <collections/linked_list.h>
#include <collections/bintree.h>

#include <mm/malloc.h>
#include <kprint/kprint.h>
#include <kpanic/kpanic.h>
#include <asm/halt.h>
#include <sched/spinlock.h>
#include <sched/kthread_kpid.h>
#include <sched/kthread_task.h>
#include <sched/kthread_preempt.h>

// We've added a lot of __attribute__((optimize("O3"))) here because it's very important
// for preemptive multitasking that task switches themselves be very very fast.

#define KTHREAD_MAIN_KPID 2

typedef struct kthread_lock_struct {
	spinlock_t spinlock;
	size_t depth;
} kthread_lock_t;

typedef struct bintree_kthread_node_struct bintree_kthread_node_t;
typedef struct kthread_struct {
	kpid_t kpid;
	kpid_t parent;
	freemem_region_t stack_region;
	kthread_task_t task;
	kthread_lock_t lock;
	bintree_kthread_node_t *blocking_node;
} kthread_t;

static kthread_lock_t new_kthread_lock () {
	kthread_lock_t lock = {
		.spinlock = new_spinlock (),
		.depth = 0
	};

	return lock;
}

static kthread_t new_kthread (
		kpid_t kpid,
		kpid_t parent,
		freemem_region_t stack_region,
		kthread_task_t task
) {
	kthread_t kthread = {
		.kpid = kpid,
		.parent = parent,
		.stack_region = stack_region,
		.task = task,
		.lock = new_kthread_lock (),
		.blocking_node = NULL
	};

	return kthread;
}

typedef struct __attribute__((packed)) zero_width_struct {
} zero_width_t;

static const zero_width_t zero_wide;

TPL_LINKED_LIST(kthread, volatile kthread_t);
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

__attribute__((optimize("O3")))
static volatile kthread_t *kthread_get_running_thread () {
	return &kthread_running_node->data;
}

__attribute__((optimize("O3")))
static volatile kthread_task_t *kthread_get_running_task () {
	volatile kthread_t *kthread_running_thread = kthread_get_running_thread ();

	return &kthread_running_thread->task;
}

__attribute__((optimize("O3")))
static volatile kthread_lock_t *kthread_get_running_lock () {
	volatile kthread_t *kthread_running_thread = kthread_get_running_thread ();

	return &kthread_running_thread->lock;
}

static freemem_region_t kthread_stack_alloc () {
	freemem_region_t stack_region = freemem_alloc (KTHREAD_STACK_SIZE, PAGE_SIZE, 0);
	if (!stack_region.length) {
		kputs ("sched/kthread: Failed to allocate new kthread stack!\n");
		kpanic ();
	}

	return stack_region;
}

static void kthread_stack_free (freemem_region_t stack_region) {
	const bool add_success = freemem_add_region (stack_region);
	if (!add_success) {
		kputs ("sched/kthread: Failed to deallocate kthread stack region!\n");
		kpanic ();
	}
}

static linked_list_kthread_node_t *kthread_node_alloc (kthread_t kthread) {
	linked_list_kthread_node_t *kthread_node =
		malloc (sizeof(linked_list_kthread_node_t));
	if (!kthread_node) {
		kputs ("sched/kthread: Failed to allocate new kthread!\n");
		kpanic ();
	}

	kthread.blocking_node =
		malloc (sizeof(bintree_kthread_node_t));
	if (!kthread.blocking_node) {
		kputs ("sched/kthread: Failed to allocate new blocking node!\n");
		kpanic ();
	}

	*kthread_node = new_linked_list_kthread_node (kthread);
	*kthread_node->data.blocking_node = new_bintree_kthread_node (
		(linked_list_kthread_node_t *)kthread_node,
		kthread_node->data.kpid);

	return kthread_node;
}

static kpid_t kthread_pop_free_kpid () {
	static volatile kpid_t last_kpid = 1;

	// Lock task until kpid_node is removed and last_kpid incremented.
	kthread_lock_task ();

	bintree_kpid_node_t *kpid_parent_node = kpids_free->search (
		(bintree_kpid_t *)kpids_free, last_kpid);

	if (!kpid_parent_node) {
		kthread_unlock_task ();
		return -1;
	}

	if (last_kpid > (kpid_t)kpid_parent_node->orderby) {
		bintree_kpid_iterator_t iterator_base, *iterator = &iterator_base;

		iterator_base = kpids_free->new_iterator ((bintree_kpid_t *)kpids_free);
		kpid_parent_node = iterator->next (iterator);
		if (!kpid_parent_node)
			kpid_parent_node = kpids_free->min ((bintree_kpid_t *)kpids_free);
	}
	bintree_kpid_node_t *kpid_node = kpid_parent_node;
	kpid_t kpid = kpid_node->orderby;

	last_kpid = (kpid + 1) % KTHREAD_MAX_PROCS;

	kpids_free->remove ((bintree_kpid_t *)kpids_free, kpid_node);

	kthread_unlock_task ();

	free (kpid_node);

	return kpid;
}

static linked_list_kthread_node_t *kthread_create_thread (
		kpid_t kpid,
		kpid_t parent,
		void (*entry) ()
) {
	freemem_region_t stack_region = kthread_stack_alloc ();
	kthread_task_t task = new_kthread_task (entry, freemem_region_end (stack_region));
	kthread_t kthread = new_kthread (kpid, parent, stack_region, task);

	linked_list_kthread_node_t *kthread_node = kthread_node_alloc (kthread);

	return kthread_node;
}

static linked_list_kthread_node_t *kthread_create_main_thread (
		freemem_region_t main_stack_region
) {
	kthread_task_t main_task = new_kthread_task_from_running ();
	kthread_t kthread = new_kthread (KTHREAD_MAIN_KPID, 0, main_stack_region, main_task);

	linked_list_kthread_node_t *kthread_node = kthread_node_alloc (kthread);

	return kthread_node;
}

static bintree_kpid_node_t *kthread_kpid_node_alloc (kpid_t kpid) {
	bintree_kpid_node_t *kpid_node = malloc (sizeof(bintree_kpid_node_t));
	if (!kpid_node) {
		kputs ("sched/kthread: Failed to allocate free kpid entry!\n");
		kpanic ();
	}

	*kpid_node = new_bintree_kpid_node (zero_wide, kpid);

	return kpid_node;
}

// Must already be out of kthreads_active/blocking queue and into kthreads_destroy queue.
static void kthread_destroy_thread (linked_list_kthread_node_t *kthread_node) {
	// Free stack ASAP
	kthread_stack_free (kthread_node->data.stack_region);

	kpid_t kpid = kthread_node->data.kpid;

	free (kthread_node->data.blocking_node);
	free (kthread_node);

	bintree_kpid_node_t *kpid_node = kthread_kpid_node_alloc (kpid);

	kthread_lock_task ();
	bintree_kpid_node_t *conflict =
		kpids_free->insert ((bintree_kpid_t *)kpids_free, kpid_node);
	kthread_unlock_task ();

	if (conflict) {
		kputs ("sched/kthread: Attempt to destroy thread with KPID not allocated!\n");
		kpanic ();
	}
}

// Task must already be locked!
__attribute__((optimize("O3")))
static void kthread_next_task (volatile kthread_task_t *this_task) {
	volatile linked_list_kthread_node_t *next_kthread_node =
		kthreads_active->pop ((linked_list_kthread_t *)kthreads_active);

	if (next_kthread_node) {
		if (kthread_get_running_kpid () == next_kthread_node->data.kpid)
			// Just return back to the task rather than switching back to the same task.
			return;

		// Increment lock depth on next task before swapping with kthread_running_node.
		volatile kthread_lock_t *next_kthread_lock = &next_kthread_node->data.lock;
		spinlock_try_lock (&next_kthread_lock->spinlock);
		next_kthread_lock->depth += 1;

		kthread_running_node = next_kthread_node;

		kthread_switch (this_task, kthread_get_running_task ());
	} else {
		// If there is nothing to do wake idle task and run it.
		kthread_wake (kthread_idle_task_kpid);

		kthread_next_task (this_task);
	}
}

static void kthread_fill_free_kpids () {
	kpid_t kpid;

	// Skip kernel main and init.
	for (kpid = 3; KTHREAD_MAX_PROCS > kpid; ++kpid) {
		bintree_kpid_node_t kpid_node_base, *kpid_node = &kpid_node_base;
		kpid_node = kthread_kpid_node_alloc (kpid);

		kthread_lock_task ();
		bintree_kpid_node_t *conflict =
			kpids_free->insert ((bintree_kpid_t *)kpids_free, kpid_node);
		kthread_unlock_task ();

		if (conflict) {
			kputs ("sched/kthread: Error inserting new free kpid!\n");
			kputs ("\tbintree bug?\n");
			kpanic ();
		}
	}
}

static void kthread_add_blocking_node (
		volatile linked_list_kthread_node_t *kthread_node
) {
	kthread_lock_task ();
	bintree_kthread_node_t *conflict =
		kthreads_blocking->insert (
			(bintree_kthread_t *)kthreads_blocking,
			kthread_node->data.blocking_node);
	kthread_unlock_task ();

	if (conflict) {
		kputs ("sched/kthread: Attempt to double-add blocking node!\n");
		kpanic ();
	}
}

static void kthread_background_task_idle () {
	kputs ("sched/kthread: Started idle background task.\n");

	// Spontaneous task switching would result in this thread "kthread_yield"ing which
	// would add it back to the active queue, and thus execute it again even if threads
	// are active and pending execution.
	kthread_lock_task ();

	for (;;) {
		// Then just halt, and block until needed again.
		halt ();

		// Block if there are any new kthreads active.
		if (kthreads_active->start)
			kthread_block ();
	}
}

static void kthread_background_task_destroy () {
	kputs ("sched/kthread: Started kthread destruction background task.\n");

	for (;;) {
		kthread_lock_task ();
		linked_list_kthread_node_t *kthread_node = kthreads_destroy->pop (
			(linked_list_kthread_t *)kthreads_destroy);
		kthread_unlock_task ();

		if (!kthread_node) {
			kthread_block ();
			continue;
		}

		kthread_destroy_thread (kthread_node);
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

	// Must wait to initialize until after kthread_running_node has been assigned.
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

	// Delay preempt until idle and destroy task have been created, task switching isn't
	// safe until those pids have been filled.
	kthread_preempt_enable ();

	kputs ("sched/kthread: Successfully initialized kthreads.\n");
}

__attribute__((optimize("O3")))
bool kthread_is_init () {
	return kthread_init_record;
}

kpid_t kthread_new_task (void (*entry) ()) {
	kpid_t new_kpid = kthread_pop_free_kpid ();
	if (0 > new_kpid)
		return -1;

	linked_list_kthread_node_t *new_kthread_node =
		kthread_create_thread (new_kpid, kthread_get_running_kpid (), entry);

	kthread_lock_task ();
	kthreads_active->append (
		(linked_list_kthread_t *)kthreads_active,
		new_kthread_node);
	kthread_unlock_task ();

	return new_kpid;
}

// Start task in the blocking state.
kpid_t kthread_new_blocking_task (void (*entry) ()) {
	kpid_t new_kpid = kthread_pop_free_kpid ();
	if (0 > new_kpid)
		return -1;

	linked_list_kthread_node_t *new_blocking_kthread_node =
		kthread_create_thread (new_kpid, kthread_get_running_kpid (), entry);

	kthread_add_blocking_node (new_blocking_kthread_node);

	return new_kpid;
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

	kthread_task_t
		ignored_task_base,
		*ignored_task = &ignored_task_base;

	for (;;)  // Avoid compiler warning about "noreturn" functions returning.
		kthread_next_task (ignored_task);
}

__attribute__((optimize("O3")))
void kthread_yield () {
	// Lock must be held through switch or else we could end up in kthreads active twice.
	kthread_lock_task ();

	kthreads_active->append (
		(linked_list_kthread_t *)kthreads_active,
		(linked_list_kthread_node_t *)kthread_running_node);

	kthread_next_task (kthread_get_running_task ());

	kthread_unlock_task ();
}

__attribute__((optimize("O3")))
bool kthread_wake (kpid_t kpid) {
	// Lock atleast until removal from blocking tree.
	kthread_lock_task ();

	bintree_kthread_node_t *kthread_blocking_node =
		kthreads_blocking->search ((bintree_kthread_t *)kthreads_blocking, kpid);
	if (!kthread_blocking_node) {
		kthread_unlock_task ();
		return false;
	}

	linked_list_kthread_node_t *kthread_node = kthread_blocking_node->data;
	if (kpid != kthread_node->data.kpid) {
		kthread_unlock_task ();
		return false;
	}

	kthreads_blocking->remove (
		(bintree_kthread_t *)kthreads_blocking,
		kthread_blocking_node);

	kthreads_active->append (
		(linked_list_kthread_t *)kthreads_active,
		kthread_node);

	kthread_unlock_task ();

	return true;
}

void kthread_block () {
	kthread_add_blocking_node (kthread_running_node);

	kthread_lock_task ();
	kthread_next_task (kthread_get_running_task ());
	kthread_unlock_task ();
}

__attribute__((optimize("O3")))
bool kthread_lock_task () {
	// Can in all likely-hood consider a call to lock task the first lock so long as
	// kthreads aren't initialized.
	if (!kthread_is_init ())
		return true;

	volatile kthread_lock_t *running_lock = kthread_get_running_lock ();

	const bool first_lock = spinlock_try_lock (&running_lock->spinlock);
	running_lock->depth += 1;

	return first_lock;
}

__attribute__((optimize("O3")))
void kthread_unlock_task () {
	if (!kthread_is_init ())
		return;

	volatile kthread_lock_t *running_lock = kthread_get_running_lock ();

	if (1 >= running_lock->depth) {
		running_lock->depth = 0;
		spinlock_release (&running_lock->spinlock);
		return;
	}

	running_lock->depth -= 1;
}

__attribute__((optimize("O3")))
kpid_t kthread_get_running_kpid () {
	volatile kthread_t *running_thread = kthread_get_running_thread ();

	kpid_t kpid = running_thread->kpid;

	return kpid;
}

// vim: set ts=4 sw=4 noet syn=c:
