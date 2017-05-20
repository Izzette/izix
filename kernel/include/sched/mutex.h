// kernel/include/sched/mutex.h

#ifndef IZIX_MUTEX_H
#define IZIX_MUTEX_H 1

#include <stdbool.h>

#include <collections/linked_list.h>

#include <sched/spinlock.h>
#include <sched/kthread.h>

TPL_LINKED_LIST(mutex_kpid, kpid_t)

typedef linked_list_mutex_kpid_t mutex_kpid_list_t;
typedef linked_list_mutex_kpid_node_t mutex_kpid_list_node_t;
typedef linked_list_mutex_kpid_iterator_t mutex_kpid_list_iterator_t;
#define new_mutex_kpid_list new_linked_list_mutex_kpid
#define new_mutex_kpid_list_node new_linked_list_mutex_kpid_node

typedef struct mutex_struct {
	spinlock_t spinlock_base;
	volatile linked_list_mutex_kpid_t waiting_kpids_base;
} mutex_t;

static inline mutex_t new_mutex () {
	mutex_t mutex = {
		.spinlock_base = new_spinlock (),
		.waiting_kpids_base = new_mutex_kpid_list ()
	};

	return mutex;
}

static inline spinlock_t *mutex_get_spinlock (mutex_t *mutex) {
	return &mutex->spinlock_base;
}

static inline volatile mutex_kpid_list_t *mutex_get_waiting_kpids (mutex_t *mutex) {
	return &mutex->waiting_kpids_base;
}

static inline bool mutex_try_lock (mutex_t *mutex) {
	if (!kthread_is_init ())
		return true;

	return spinlock_try_lock (mutex_get_spinlock (mutex));
}

void mutex_lock (mutex_t *);
void mutex_release (mutex_t *);

#endif

// vim: set ts=4 sw=4 noet syn=c: