// kernel/arch/x86/include/sched/spinlock.h

#ifndef IZIX_SPINLOCK_H
#define IZIX_SPINLOCK_H 1

#include <stdbool.h>

typedef volatile int spinlock_t;

#pragma GCC diagnostic ignored "-Wignored-qualifiers"
static inline volatile spinlock_t new_spinlock () {
	spinlock_t lock = 0;

	return lock;
}
#pragma GCC diagnostic pop

static inline bool spinlock_try_lock (spinlock_t *lock) {
	return !(__sync_lock_test_and_set (lock, 1));
}

static inline void spinlock_lock (spinlock_t *lock) {
	while (!spinlock_try_lock (lock))
		while (*lock);
}

static inline void spinlock_release (spinlock_t *lock) {
	__sync_lock_release (lock);
}

static inline bool spinlock_is_locked (spinlock_t *lock) {
	if (spinlock_try_lock (lock)) {
		spinlock_release (lock);
		return false;
	}

	return true;
}

#endif

// vim: set ts=4 sw=4 noet syn=c:
