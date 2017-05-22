// kernel/arch/x86/include/sched/native_lock.h

#ifndef IZIX_NATIVE_LOCK_H
#define IZIX_NATIVE_LOCK_H 1

#include <stdbool.h>

typedef volatile int native_lock_t;

#pragma GCC diagnostic ignored "-Wignored-qualifiers"
static inline volatile native_lock_t new_native_lock () {
	native_lock_t lock = 0;

	return lock;
}
#pragma GCC diagnostic pop

__attribute__((optimize("O3")))
static inline bool native_lock_try_lock (native_lock_t *lock) {
	return !(__sync_lock_test_and_set (lock, 1));
}

__attribute__((optimize("O3")))
static inline void native_lock_release (native_lock_t *lock) {
	__sync_lock_release (lock);
}

__attribute__((optimize("O3")))
static inline bool native_lock_is_locked (native_lock_t *lock) {
	if (native_lock_try_lock (lock)) {
		native_lock_release (lock);
		return false;
	}

	return true;
}

#endif

// vim: set ts=4 sw=4 noet syn=c:
