// kernel/sched/mutex.c

#include <attributes.h>
#include <collections/linked_list.h>

#include <mm/malloc.h>
#include <kprint/kprint.h>
#include <kpanic/kpanic.h>
#include <sched/native_lock.h>
#include <sched/mutex.h>
#include <sched/kthread.h>

FASTCALL FAST
void mutex_lock (mutex_t *mutex) {
	if (!kthread_is_init ())
		return;

	const volatile mutex_kpid_list_t *waiting_kpids = mutex_get_waiting_kpids (mutex);

	mutex_kpid_list_node_t *kpid_node = NULL;

	while (!native_lock_try_lock (mutex_get_native_lock (mutex))) {
		// kpid_node may already be allocated.
		if (!kpid_node) {
			kpid_node = malloc (sizeof(mutex_kpid_list_node_t));
			if (!kpid_node) {
				kputs ("sched/mutex: Failed to allocate new kpid waiting node!\n");
				kpanic ();
			}
			*kpid_node = new_mutex_kpid_list_node (kthread_get_running_kpid ());
		}

		// Lock through second lock obtain attempt, to adding to waiting kpids, to
		// locking the task.
		spinlock_lock (mutex_get_spinlock (mutex));

		// In case the lock has been released since our last attempt, we don't want
		// to be waiting around for a lock that's already been released.
		if (native_lock_try_lock (mutex_get_native_lock (mutex))) {
			// It would be unfair if we obtained the lock in this next attempt, as we
			// would get it before threads who have been waiting longer.
			spinlock_release (mutex_get_spinlock (mutex));
			mutex_release (mutex);
			kthread_yield ();
			continue;
		}

		waiting_kpids->append (
			(mutex_kpid_list_t *)waiting_kpids,
			kpid_node);

		// Task does need to be locked through kthread_block, or else the wake-up event
		// wouldn't wake this task up (because it isn't blocking yet).
		kthread_lock_task ();

		spinlock_release (mutex_get_spinlock (mutex));

		kthread_block ();

		kthread_unlock_task ();
	}

	if (kpid_node)
		free (kpid_node);
}

FASTCALL FAST
void mutex_release (mutex_t *mutex) {
	if (!kthread_is_init ())
		return;

	volatile mutex_kpid_list_t *waiting_kpids;
	mutex_kpid_list_node_t *kpid_node;

	waiting_kpids = mutex_get_waiting_kpids (mutex);

	// We don't want any threads trying to add themselves as waiting while we are working
	// with the list and waking the next one up.
	spinlock_lock (mutex_get_spinlock (mutex));

	kpid_node = waiting_kpids->pop ((mutex_kpid_list_t *)waiting_kpids);
	if (kpid_node)
		kthread_wake (kpid_node->data);

	native_lock_release (mutex_get_native_lock (mutex));

	spinlock_release (mutex_get_spinlock (mutex));
}

// vim: set ts=4 sw=4 noet syn=c:
