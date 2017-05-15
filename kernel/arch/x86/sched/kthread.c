// kernel/arch/x86/sched/kthread.c

#include <stddef.h>
#include <stdbool.h>

#include <string.h>
#include <collections/bintree.h>
#include <collections/linked_list.h>

#include <mm/freemem.h>
#include <mm/malloc.h>
#include <mm/page.h>
#include <kpanic/kpanic.h>
#include <kprint/kprint.h>
#include <sched/kthread_switch.h>
#include <sched/kthread_bootstrap.h>
#include <sched/kthread.h>

#define KTHREAD_MAX_PROCS 256
#define KTHREAD_STACK_SIZE (8 * PAGE_SIZE)

typedef struct kthread_struct {
	kpid_t kpid;
	kpid_t parent;
	void *stack;
	kthread_registers_t task;
} kthread_t;

typedef struct __attribute__((packed)) zero_width_struct {
} zero_width_t;

static const zero_width_t zero_wide;

#pragma GCC diagnostic ignored "-Wstrict-aliasing"
TPL_LINKED_LIST(kthread, volatile kthread_t);
#pragma GCC diagnostic pop
TPL_BINTREE(kpid, zero_width_t);
TPL_BINTREE(kthread, linked_list_kthread_node_t *);

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

volatile linked_list_kthread_node_t *volatile kthread_running = NULL;
volatile kpid_t kthread_destroy_blocking = -1;

static volatile kthread_registers_t
	kthread_main_task_base,
	*kthread_main_task = &kthread_main_task_base;

static inline kthread_t new_kthread (
		kpid_t kpid,
		kpid_t parent,
		void *stack
) {
	kthread_t kthread = {
		.kpid = kpid,
		.parent = parent,
		.stack = stack,
		.task = new_kthread_task (
			stack + KTHREAD_STACK_SIZE
		)
	};

	return kthread;
}

static inline volatile kthread_registers_t *kthread_get_running_task () {
	if (!kthread_running)
		return kthread_main_task;

	return &kthread_running->data.task;
}

static inline void *kthread_stack_alloc () {
	freemem_region_t stack_region = freemem_suggest (KTHREAD_STACK_SIZE, PAGE_SIZE, 0);
	if (!stack_region.length)
		return NULL;

	const bool remove_success = freemem_remove_region (stack_region);
	if (!remove_success) {
		kputs (
			"sched/kthread: Failed to trivially remove "
			"freemem-suggested stack region!\n");
		kpanic ();
	}

	return stack_region.p;
}

static inline void kthread_stack_free (void *stack) {
	freemem_region_t stack_region = new_freemem_region (stack, KTHREAD_STACK_SIZE);

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
	void *stack = kthread_stack_alloc ();
	if (!stack) {
		kputs ("sched/kthread: Failed to allocate new kthread stack!\n");
		kpanic ();
	}

	kthread_t kthread = new_kthread (kpid, parent, stack);

	linked_list_kthread_node_t *kthread_node =
		malloc (sizeof(linked_list_kthread_node_t));
	if (!kthread_node) {
		kputs ("sched/kthread: Failed to allocate new kthread!\n");
		kpanic ();
	}

	*kthread_node = new_linked_list_kthread_node (kthread);

	return kthread_node;
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
	kthread_stack_free (kthread_node->data.stack);

	kpid_t kpid = kthread_node->data.kpid;

	kthreads_destroy->removeNode ((linked_list_kthread_t *)kthreads_destroy, kthread_node);

	bintree_kpid_node_t *kpid_node = kthread_kpid_node_alloc (kpid);

	kpids_free->insert ((bintree_kpid_t *)kpids_free, kpid_node);
}

static inline void kthread_next_task (volatile kthread_registers_t *this_task) {
	kthread_running = kthreads_active->pop ((linked_list_kthread_t *)kthreads_active);

	if (kthread_running)
		kthread_switch (this_task, kthread_get_running_task ());
	else
		kthread_switch (this_task, kthread_main_task);
}

static void kthread_background_task_destroy () {
	linked_list_kthread_iterator_t
		iterator_base,
		*iterator = &iterator_base;

	for (;;) {
		kthread_destroy_blocking = kthread_get_running ();
		kthread_block ();

		iterator_base = kthreads_destroy->new_iterator (
			(linked_list_kthread_t *)kthreads_destroy);

		linked_list_kthread_node_t *kthread_node;

		kthread_node = iterator->cur (iterator);
		while (kthread_node) {
			// Need to obtain the next node first, because kthread_destroy_thread will
			// deallocate and kill it.
			linked_list_kthread_node_t *kthread_next_node = iterator->next (iterator);

			kthread_destroy_thread (kthread_node);

			kthread_node = kthread_next_node;
		}
	}
}

void kthread_init () {
	kpid_t kpid;

	*kthreads_active = new_linked_list_kthread ();
	*kthreads_destroy = new_linked_list_kthread ();

	*kthreads_blocking = new_bintree_kthread ();

	*kpids_free = new_bintree_kpid ();

	// Skip kernel main and init.
	for (kpid = 2; KTHREAD_MAX_PROCS > kpid; ++kpid) {
		bintree_kpid_node_t kpid_node_base, *kpid_node = &kpid_node_base;
		kpid_node = kthread_kpid_node_alloc (kpid);
		kpids_free->insert ((bintree_kpid_t *)kpids_free, kpid_node);
	}

	// Start the destruction background task.
	kthread_new_task (kthread_background_task_destroy);

	// HACK! Switch tasks back to main.
	kthread_switch (kthread_main_task, kthread_main_task);

	kputs ("sched/kthread: Successfuly initalized kthreads.\n");
}

kpid_t kthread_new_task (void (*task) ()) {
	kpid_t new_kpid = kthread_pop_free_kpid ();
	if (0 > new_kpid)
		return -1;

	linked_list_kthread_node_t *new_kthread_node =
		kthread_create_thread (new_kpid, kthread_get_running ());

	volatile kthread_registers_t *new_task = &new_kthread_node->data.task;

	// Our bootstrap code, the "glue" that keeps the stack backtraceable.
#pragma GCC diagnostic ignored "-Wpointer-to-int-cast"
	kthread_task_pushl ((kthread_registers_t *)new_task, (uint32_t)task);
#pragma GCC diagnostic pop
	kthread_task_pushl ((kthread_registers_t *)new_task, 0);
	kthread_task_push_caller ((kthread_registers_t *)new_task, kthread_bootstrap);

	kthreads_active->append ((linked_list_kthread_t *)kthreads_active, new_kthread_node);

	return new_kpid;
}

void kthread_end_task () {
	kthreads_destroy->append (
		(linked_list_kthread_t *)kthreads_destroy,
		(linked_list_kthread_node_t *)kthread_running);

	if (0 < kthread_destroy_blocking) {
		const bool destroy_wake_success = kthread_wake (kthread_destroy_blocking);
		if (!destroy_wake_success) {
			kputs (
				"sched/kthread: Failed to wake supposedly "
				"blocking kthread-destroy task!\n");
			kpanic ();
		}

		kthread_destroy_blocking = -1;
	}

	kthread_registers_t
		ignored_task_base,
		*ignored_task = &ignored_task_base;

	for (;;)  // Avoid compiler warning about noreturn functions returning.
		kthread_next_task (ignored_task);
}

void kthread_yeild () {
	kthreads_active->append (
		(linked_list_kthread_t *)kthreads_active,
		(linked_list_kthread_node_t *)kthread_running);

	kthread_next_task (kthread_get_running_task ());
}

bool kthread_wake (kpid_t kpid) {
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

	return true;
}

void kthread_block () {
	bintree_kthread_node_t *kthread_blocking_node =
		malloc (sizeof(bintree_kthread_node_t));
	if (!kthread_blocking_node) {
		kputs ("sched/kthread: Failed to allocate new blocking node!\n");
		kpanic ();
	}

	*kthread_blocking_node = new_bintree_kthread_node (
		(linked_list_kthread_node_t *)kthread_running,
		kthread_get_running ());

	kthreads_blocking->insert (
		(bintree_kthread_t *)kthreads_blocking,
		kthread_blocking_node);

	kthread_next_task (kthread_get_running_task ());
}

kpid_t kthread_get_running () {
	if (!kthread_running)
		return 0;

	return kthread_running->data.kpid;
}

// Should only be called from main task.
void kthread_run () {
	kthread_next_task (kthread_main_task);
}

// vim: set ts=4 sw=4 noet syn=c:
