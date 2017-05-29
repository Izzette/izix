// kernel/include/time/clock.h

#ifndef IZIX_CLOCK_H
#define IZIX_CLOCK_H 1

#include <attributes.h>

#include <time/time.h>

#define CLOCKS_PER_SEC ((clock_t)1000000)
#define CLOCK_INTERVAL \
	((time_t)(time_from_nanos (time_nanos (time_from_secs (1)) / CLOCKS_PER_SEC)))

typedef unsigned long long clock_t;

// The interval for the clock tick, multiplier and divisor to avoid rounding errors`.
extern time_t clock_real_interval_multiplier;
extern clock_t clock_real_interval_divisor;

// The total number of recorded ticks.
clock_t clock_get_ticks ();
// The number of clock ticks since last wake.
clock_t clock_get_wake_ticks ();

// The clock_get_*time functions CANNOT be called in INTERRUPT HANDLERS!

// The time since the Unix Epoch, or time since first known.
time_t clock_get_time ();
// The time since last wake.
time_t clock_get_wake_time ();
// The time since boot.
time_t clock_get_boot_time ();

// Set the time, use to set boot time.
void clock_set_time (time_t);
// Set the time since last wake.
void clock_set_wake_time (time_t);

// These functions together form one NON-REENTRANT CRITICAL SECTION and MUST BE EXECUTED
// TO COMPLETION but *can* be called in interrupt handlers.
// Add a clock tick.
void clock_tick ();
// Add to the fast clock.
FASTCALL
void clock_fast_add (time_t);

#endif

// vim: set ts=4 sw=4 noet syn=c:
