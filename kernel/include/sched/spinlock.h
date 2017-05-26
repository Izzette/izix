// kernel/include/sched/spinlock.h

#ifndef IZIX_SPINLOCK_H
#define IZIX_SPINLOCK_H 1

#include <stdbool.h>

#include <attributes.h>

#include <sched/native_lock.h>

typedef native_lock_t spinlock_t;

#pragma GCC diagnostic ignored "-Wignored-qualifiers"
static inline volatile spinlock_t new_spinlock () {
	return new_native_lock ();
}
#pragma GCC diagnostic pop

FASTCALL
bool spinlock_try_lock (spinlock_t *);
FASTCALL
void spinlock_lock (spinlock_t *);
FASTCALL
void spinlock_release (spinlock_t *);
FASTCALL
bool spinlock_is_locked (spinlock_t *);

#endif

// // vim: set ts=4 sw=4 noet syn=c:
