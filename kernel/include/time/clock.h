// kernel/include/time/clock.h

#ifndef IZIX_CLOCK_H
#define IZIX_CLOCK_H 1

#include <attributes.h>

#include <time/time.h>

#define CLOCKS_PER_SEC clocks_per_sec

typedef unsigned long long clock_t;

extern clock_t clocks_per_sec;

// With the exception of clock_tick and clock_add_fast_time, these functions
// CANNOT be called in INTERUPT HANDLERS!

// The total number of recorded ticks.
clock_t clock_get_ticks ();
// The number of clock ticks since boot.
clock_t clock_get_boot_ticks ();
// The number of clock ticks since last wake.
clock_t clock_get_wake_ticks ();

// The time since the Unix Epoch, or zero if unknown.
time_t clock_get_time ();
// Faster less accurete time since the Unix Epoch (for timing purposes).
time_t clock_get_fast_time ();
// The time since boot.
time_t clock_get_boot_time ();
// The time since last wake.
time_t clock_get_wake_time ();

// Set the time, use to set boot time.
void clock_set_time (time_t);
// Set the time since last wake.
void clock_set_wake_time (time_t);

// These functions together are a CRITICAL SECTION but *can* be called in
// interupt handlers.
// Add a clock tick.
void clock_tick ();
// Add to fast clock time.
FASTCALL
void clock_add_fast_time (time_t);

#endif

// vim: set ts=4 sw=4 noet syn=c:
