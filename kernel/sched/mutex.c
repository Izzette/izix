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

	while (!native_lock_try_lock (mutex_get_native_lock (mutex))) {
		volatile mutex_kpid_list_t *waiting_kpids;
		mutex_kpid_list_node_t *kpid_node;

		waiting_kpids = mutex_get_waiting_kpids (mutex);

		kpid_node = malloc (sizeof(mutex_kpid_list_node_t));
		if (!kpid_node) {
			kputs ("sched/mutex: Failed to allocate new kpid waiting node!\n");
			kpanic ();
		}

		*kpid_node = new_mutex_kpid_list_node (kthread_get_running_kpid ());

		kthread_lock_task ();

		// It would be unfair if we obtained the lock in this next attempt, as we would get
		// it before threads who have been waiting longer, so let's give them a chance.
		kthread_yield ();

		// In case the lock has been released since our last attempt, we don't want
		// to be waiting around for a lock that's already been released.
		if (native_lock_try_lock (mutex_get_native_lock (mutex))) {
			free (kpid_node);

			break;
		}

		waiting_kpids->append (
			(mutex_kpid_list_t *)waiting_kpids,
			kpid_node);

		kthread_block ();

		kthread_unlock_task ();
	}
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
	kthread_lock_task ();

	kpid_node = waiting_kpids->pop ((mutex_kpid_list_t *)waiting_kpids);
	if (kpid_node) {
		kthread_wake (kpid_node->data);
		free (kpid_node);
	}

	native_lock_release (mutex_get_native_lock (mutex));

	kthread_unlock_task ();
}

// vim: set ts=4 sw=4 noet syn=c:
