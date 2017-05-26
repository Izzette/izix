// kernel/sched/spinlock.c

#include <stdbool.h>

#include <attributes.h>

#include <sched/native_lock.h>
#include <sched/spinlock.h>
#include <sched/kthread.h>

FASTCALL FAST
bool spinlock_try_lock (spinlock_t *lock) {
	return native_lock_try_lock ((native_lock_t *)lock);
}

FASTCALL FAST
void spinlock_lock (spinlock_t *lock) {
	while (!spinlock_try_lock (lock))
		kthread_yield ();
}

FASTCALL FAST
void spinlock_release (spinlock_t *lock) {
	native_lock_release ((native_lock_t *)lock);
}

FASTCALL FAST
bool spinlock_is_locked (spinlock_t *lock) {
	return native_lock_is_locked ((native_lock_t *)lock);
}

// // vim: set ts=4 sw=4 noet syn=c:
