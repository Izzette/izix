// kernel/arch/x86/sched/kthreads.c

#include <stddef.h>
#include <stdbool.h>

#include <string.h>
#include <collections.h>

#include <mm/malloc.h>
#include <kpanic/kpanic.h>
#include <kprint/kprint.h>
#include <sched/switch_kthread.h>
#include <sched/kthreads.h>

#define KTHREAD_MAX_PROCS 256
#define KTHREAD_STACK_SIZE 8 * 4096

typedef volatile struct kthread_process_struct {
	bool in_use;
	kthread_registers_t task;
} kthread_process_t;

volatile kthread_registers_t *volatile kthread_running_task;
static volatile kthread_process_t *kthread_processes;
static volatile kpid_t kthread_next_free_id;
static volatile kthread_registers_t *kthread_main_task;

static inline void kthread_find_next_free () {
	kpid_t id;

	for (
			id = kthread_next_free_id + 1;
			kthread_next_free_id != id;
			++id, id %= KTHREAD_MAX_PROCS)
		if (!kthread_processes[id].in_use) {
			kthread_next_free_id = id;
			return;
		}

	kthread_next_free_id = -1;
}

static inline bool kthread_has_free () {
	return 0 < kthread_next_free_id;
}

static inline void kthread_free (kpid_t id) {
	kthread_processes[id].in_use = false;

	if (kthread_has_free ())
		return;

	kthread_next_free_id = id;
}

void kthread_init () {
	kthread_processes = malloc (KTHREAD_MAX_PROCS * sizeof(kthread_process_t));
	if (!kthread_processes) {
		kputs ("sched/kthreads: Failed to allocate process table!\n");
		kpanic ();
	}

	memset (
		(void *)kthread_processes, '\0',
		KTHREAD_MAX_PROCS * sizeof(kthread_process_t));

	kthread_next_free_id = 1;
	kthread_processes->in_use = true;
	kthread_main_task = &kthread_processes->task;

	// HACK! Switch tasks back to main.
	switch_kthread (kthread_main_task, kthread_main_task);

	kputs ("sched/kthread: Successfuly initalized kthreads.\n");
}

kpid_t kthread_new_task (void (*task) ()) {
	if (!kthread_has_free ())
		return -1;

	void *new_stack = malloc (KTHREAD_STACK_SIZE);
	if (!new_stack)
		return -1;

	kpid_t new_kpid = kthread_next_free_id;
	kthread_process_t *new_proc = kthread_processes + new_kpid;
	new_proc->in_use = true;
	kthread_registers_t *new_task = &new_proc->task;

	kthread_find_next_free ();

#pragma GCC diagnostic ignored "-Wpointer-to-int-cast"
	new_task->esp = (uint32_t)(new_stack + KTHREAD_STACK_SIZE);
#pragma GCC diagnostic pop
	new_task->ebp = new_task->esp;
	new_task->eflags = kthread_running_task->eflags;

	new_task->esp -= sizeof(void (*) ());
#pragma GCC diagnostic ignored "-Wint-to-pointer-cast"
	*((void (**) ())new_task->esp) = kthread_end_task;
#pragma GCC diagnostic pop

	new_task->esp -= sizeof(void (*) ());
#pragma GCC diagnostic ignored "-Wint-to-pointer-cast"
	*((void (**) ())new_task->esp) = task;
#pragma GCC diagnostic pop

	switch_kthread (kthread_running_task, new_task);

	return new_kpid;
}

void kthread_end_task () {
	kthread_free (kthread_get_running ());

	kthread_registers_t nil_task;

	switch_kthread (&nil_task, kthread_main_task);
}

void kthread_yeild () {
	switch_kthread (kthread_running_task, kthread_main_task);
}

kpid_t kthread_get_running () {
	kthread_process_t *kthread_running_process =
		(void *)kthread_running_task - offsetof(kthread_process_t, task);

	return kthread_running_process - kthread_processes;
}

// Should only be called from main task.
void kthread_run () {
	kpid_t id;

	// Skip main.
	for (id = 1; KTHREAD_MAX_PROCS > id; ++id) {
		kthread_process_t *proc = kthread_processes + id;

		if (!proc->in_use)
			continue;

		switch_kthread (kthread_main_task, &proc->task);
	}
}

// vim: set ts=4 sw=4 noet syn=c:
