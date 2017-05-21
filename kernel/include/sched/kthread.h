// kernel/include/sched/kthread.h

#ifndef IZIX_KTHREAD_H
#define IZIX_KTHREAD_H 1

#include <stdbool.h>

#include <mm/freemem.h>
#include <mm/paging.h>
#include <sched/spinlock.h>
#include <sched/kthread_kpid.h>
#include <sched/kthread_task.h>

#define KTHREAD_MAX_PROCS 256
#define KTHREAD_STACK_SIZE (8 * PAGE_SIZE)

void kthread_init (freemem_region_t);
bool kthread_is_init ();
void kthread_end_task ()
	__attribute__((noreturn));
void kthread_yield ();
// Return is true if kpid was blocking, false otherwise (even if kpid is free).
bool kthread_wake (kpid_t);
void kthread_block ();
kpid_t kthread_new_task (void (*) ());
kpid_t kthread_new_blocking_task (void (*) ());
kpid_t kthread_new_main_task ();

/* Locking manipulates a semaphore that increments every time a lock is called decremented
 * for every time unlock is call.  So, for every kthread_lock_task there must be an unlock
 * in order to unlock the task.
 */
/* Return is true if the task was previously unlocked, false if already locked, but you
 * still must call kthread_unlock_task.
 */
bool kthread_lock_task ();
void kthread_unlock_task ();

kpid_t kthread_get_running_kpid ();

#endif

// vim: set ts=4 sw=4 noet syn=c:
