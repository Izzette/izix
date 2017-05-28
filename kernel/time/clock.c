// kernel/time/clock.c

#include <stdbool.h>
#include <stdint.h>

#include <attributes.h>

#include <kprint/kprint.h>
#include <kpanic/kpanic.h>
#include <sched/mutex.h>
#include <time/time.h>
#include <time/clock.h>

#define CLOCK_INTERVAL \
	((time_t)(time_from_nanos (time_nanos (time_from_secs (1)) / CLOCKS_PER_SEC)))

// clock_ticks is *not* updated atomically and must be lock-free. See
// clock_fetch_ticks_lock_free.

// Clock ticks grand total.
static volatile clock_t clock_ticks = 0;
// Interval of a slow clock tick, this should be updated once before enabling the clock.
time_t clock_real_interval_multiplier = 0;
clock_t clock_real_interval_divisor = 0;

// clock_last_*_ticks and clock_*_rt protected by the locks clock_ticks_mutex and
// clock_time_mutex, respectively.  See clock_fetch_ticks and clock_fetch_time.

// Clock ticks before clock_last_known_rt.
static volatile clock_t clock_last_known_ticks = 0;
// Clock ticks before clock_wake_rt.
static volatile clock_t clock_last_wake_ticks = 0;

// First known real time since last wake.
static time_t clock_wake_rt = 0;
// Last known real time.
static time_t clock_last_known_rt = 0;

// Lock for clock_last_*_ticks.
static mutex_t
	clock_ticks_mutex_base,
	*clock_ticks_mutex = &clock_ticks_mutex_base;
// Lock for clock_*_rt.
static mutex_t
	clock_time_mutex_base,
	*clock_time_mutex = &clock_time_mutex_base;

// For use with clock_ticks only!
FAST
static clock_t clock_fetch_clock_lock_free (volatile clock_t *c_ptr) {
	clock_t c = *c_ptr;

	while ((c != *c_ptr))
		c = *c_ptr;

	return c;
}

// For use with clock_last_*_ticks only!
FAST
static clock_t clock_fetch_clock (volatile clock_t *c_ptr) {
	mutex_lock (clock_ticks_mutex);
	const clock_t c = *c_ptr;
	mutex_release (clock_ticks_mutex);

	return c;
}

// For use with clock_*_rt only!
FAST
static time_t clock_fetch_time (volatile time_t *t_ptr) {
	mutex_lock (clock_time_mutex);
	const time_t t = *t_ptr;
	mutex_release (clock_time_mutex);

	return t;
}

CONSTRUCTOR
static void clock_construct () {
	clock_ticks_mutex_base = new_mutex ();
	clock_time_mutex_base = new_mutex ();
}

FAST
clock_t clock_get_ticks () {
	// Ticks and fast_last_tick must be obtained within the same slow tick.
	const clock_t ticks = clock_fetch_clock_lock_free (&clock_ticks);

	// The time in ticks.
	const time_t tick_time =
		clock_real_interval_multiplier * ticks / clock_real_interval_divisor;

	// Fake XSI-compliant 1000000 ticks per second.
	return tick_time / CLOCK_INTERVAL;
}

FAST
clock_t clock_get_wake_ticks () {
	const clock_t last_wake_ticks = clock_fetch_clock (&clock_last_wake_ticks);
	const clock_t ticks = clock_get_ticks ();

	const clock_t wake_ticks = ticks - last_wake_ticks;

	return wake_ticks;
}

FAST
time_t clock_get_time () {
	const time_t last_known_rt = clock_fetch_time (&clock_last_known_rt);
	// If we don't know the time, return the time since boot.
	if (!last_known_rt)
		return clock_get_boot_time ();

	const clock_t ticks = clock_fetch_clock (&clock_last_known_ticks);

	return CLOCK_INTERVAL * ticks + last_known_rt;
}

FAST
time_t clock_get_wake_time () {
	const time_t wake_rt = clock_fetch_time (&clock_wake_rt);
	// If we don't know the last wake time, return the time since boot.
	if (!wake_rt)
		return clock_get_boot_time ();

	const clock_t wake_ticks = clock_get_wake_ticks ();

	return CLOCK_INTERVAL * wake_ticks + wake_rt;
}

FAST
time_t clock_get_boot_time () {
	const clock_t boot_ticks = clock_get_ticks ();

	return CLOCK_INTERVAL * boot_ticks;
}

void clock_set_time (time_t t) {
	if (!t) {
		kputs ("time/clock: Attempt to set time to zero!\n");
		kpanic ();
	}

	if (!clock_fetch_time (&clock_wake_rt)) {
		clock_set_wake_time (t);
	} else {
		const clock_t ticks = clock_get_ticks ();

		mutex_lock (clock_ticks_mutex);
		mutex_lock (clock_time_mutex);
		clock_last_known_ticks = ticks;
		clock_last_known_rt = t;
		mutex_release (clock_time_mutex);
		mutex_release (clock_ticks_mutex);
	}
}

void clock_set_wake_time (time_t t) {
	if (!t) {
		kputs ("time/clock: Attempt to set wake time to zero!\n");
		kpanic ();
	}

	const clock_t ticks = clock_get_ticks ();

	mutex_lock (clock_ticks_mutex);
	mutex_lock (clock_time_mutex);
	clock_last_known_rt = t;
	clock_last_known_ticks = ticks;
	clock_wake_rt = t;
	clock_last_wake_ticks = ticks;
	mutex_release (clock_time_mutex);
	mutex_release (clock_ticks_mutex);
}

FAST HOT
void clock_tick () {
	clock_ticks += 1;
}

// vim: set ts=4 sw=4 noet syn=c:
