// kernel/sched/spinlock.c

#include <stdbool.h>

#include <sched/native_lock.h>
#include <sched/spinlock.h>
#include <sched/kthread.h>

__attribute__((optimize("O3")))
bool spinlock_try_lock (spinlock_t *lock) {
	return native_lock_try_lock ((native_lock_t *)lock);
}

__attribute__((optimize("O3")))
void spinlock_lock (spinlock_t *lock) {
	while (!spinlock_try_lock (lock))
		kthread_yield ();
}

__attribute__((optimize("O3")))
void spinlock_release (spinlock_t *lock) {
	native_lock_release ((native_lock_t *)lock);
}

__attribute__((optimize("O3")))
bool spinlock_is_locked (spinlock_t *lock) {
	return native_lock_is_locked ((native_lock_t *)lock);
}

// // vim: set ts=4 sw=4 noet syn=c:
