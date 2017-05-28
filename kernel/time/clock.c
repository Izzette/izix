// kernel/time/clock.c

#include <stdbool.h>

#include <attributes.h>

#include <kprint/kprint.h>
#include <kpanic/kpanic.h>
#include <sched/kthread.h>
#include <time/time.h>
#include <time/clock.h>

// First known real time.
static time_t clock_boot_rt = 0;
// First known real time since last wake.
static time_t clock_wake_rt = 0;
// Last known real time.
static time_t clock_last_known_rt = 0;

// Ticks and clock_fast are *not* updated atomically, they must be read twice
// until the same result is obtained!

// Clock ticks grand total.
static volatile clock_t clock_ticks = 0;
// Clock ticks before clock_lask_known_rt.
static volatile clock_t clock_last_known_ticks = 0;
// Clock ticks before clock_wake_rt.
static volatile clock_t clock_last_wake_ticks = 0;
// Clock ticks before clock_boot_rt.
static volatile clock_t clock_last_boot_ticks = 0;
// Fast clock updates.
static volatile time_t clock_fast = 0;
// Fast clock state at last tick.
static volatile time_t clock_fast_last_tick = 0;

clock_t clocks_per_sec = 0;

FAST
static time_t clock_get_time_atomic (volatile time_t *t_ptr) {
	time_t t = *t_ptr;

	while ((t != *t_ptr)) {
		kthread_yield ();
		t = *t_ptr;
	}

	return t;
}

FAST
static clock_t clock_get_clock_atomic (volatile clock_t *c_ptr) {
	clock_t c = *c_ptr;

	while ((c != *c_ptr))
		c = *c_ptr;

	return c;
}

FAST
static bool clock_tick_occured (clock_t last_ticks) {
	return last_ticks != clock_get_clock_atomic (&clock_ticks);
}

FAST
clock_t clock_get_ticks () {
	return clock_get_clock_atomic (&clock_ticks);
}

FAST
clock_t clock_get_boot_ticks () {
	clock_t ticks, last_boot_ticks;

	do {
		ticks = clock_get_ticks ();
		last_boot_ticks = clock_get_clock_atomic (&clock_last_boot_ticks);
	} while (clock_tick_occured (ticks));

	const clock_t boot_ticks = ticks - last_boot_ticks;

	return CLOCKS_PER_SEC * boot_ticks;
}

FAST
clock_t clock_get_wake_ticks () {
	clock_t ticks, last_wake_ticks;

	do {
		ticks = clock_get_ticks ();
		last_wake_ticks = clock_get_clock_atomic (&clock_last_wake_ticks);
	} while (clock_tick_occured (ticks));

	const clock_t wake_ticks = ticks - last_wake_ticks;

	return CLOCKS_PER_SEC * wake_ticks;
}

FAST
time_t clock_get_time () {
	clock_t ticks;
	time_t last_known_rt;

	do {
		ticks = clock_get_ticks ();
		last_known_rt = clock_get_time_atomic (&clock_last_known_rt);
	} while (clock_tick_occured (ticks));

	return CLOCKS_PER_SEC * ticks + last_known_rt;
}

FAST
time_t clock_get_fast_time () {
	clock_t ticks;
	time_t slow_time;
	time_t fast_last_tick;

	do {
		ticks = clock_get_ticks ();
		slow_time = clock_get_time ();
		fast_last_tick = clock_get_time_atomic (&clock_fast_last_tick);
	} while (clock_tick_occured (ticks));

	const time_t fast = clock_get_time_atomic (&clock_fast);
	const time_t fast_since_last_tick = fast - fast_last_tick;

	return fast_since_last_tick + slow_time;
}

FAST
time_t clock_get_boot_time () {
	time_t ticks;
	clock_t boot_ticks;
	time_t boot_rt;

	do {
		ticks = clock_get_ticks ();
		boot_ticks = clock_get_boot_ticks ();
		boot_rt = clock_get_time_atomic (&clock_boot_rt);
	} while (clock_tick_occured (ticks));

	return CLOCKS_PER_SEC * boot_ticks + boot_rt;
}

FAST
time_t clock_get_wake_time () {
	time_t ticks;
	clock_t wake_ticks;
	time_t wake_rt;

	do {
		ticks = clock_get_ticks ();
		wake_ticks = clock_get_wake_ticks ();
		wake_rt = clock_get_time_atomic (&clock_wake_rt);
	} while (clock_tick_occured (ticks));

	return CLOCKS_PER_SEC * wake_ticks + wake_rt;
}

void clock_set_time (time_t t) {
	if (!t) {
		kputs ("time/clock: Attempt to set time to zero!\n");
		kpanic ();
	}

	if (!clock_get_time_atomic (&clock_wake_rt)) {
		// Also sets clock_last_known_rt;
		clock_set_wake_time (t);
	} else {
		clock_last_known_rt = t;
		clock_last_known_ticks = clock_get_ticks ();
	}
}

void clock_set_wake_time (time_t t) {
	if (!t) {
		kputs ("time/clock: Attempt to set wake time to zero!\n");
		kpanic ();
	}

	if (!clock_get_time_atomic (&clock_boot_rt)) {
		clock_boot_rt = t;
		clock_last_boot_ticks = clock_get_ticks ();
	}

	clock_wake_rt = t;
	clock_last_wake_ticks = clock_get_ticks ();

	// Will not call this function, because clock_wake_rt is set.
	clock_set_time (t);
}

FAST HOT
void clock_tick () {
	clock_ticks += 1;
}

FASTCALL FAST HOT
void clock_add_fast_time (time_t t) {
	// Gurenteed not to change, clock_tick and clock_add_fast_time together
	// are a critical section.
	clock_fast_last_tick = clock_ticks;
	clock_fast += t;
}

// vim: set ts=4 sw=4 noet syn=c:
