// kernel/arch/x86/include/pit_8253/pit_8253.h

#ifndef IZIX_PIT_8253_H
#define IZIX_PIT_8253_H 1

#include <stdint.h>

#include <time/time.h>

// PIT base frequency in MHz (~1.19318167 MHZ)
#define PIT_8253_BASE_MEGAHZ 1.19318167l

#define PIT_8253_DIVIDER_MIN 1
// Exclude 65536 for compatibility.
#define PIT_8253_DIVIDER_MAX 65535

#define PIT_8253_INTERVAL_MIN \
	pit_8253_interval (PIT_8253_DIVIDER_MIN)
#define PIT_8253_INTERVAL_MAX \
	pit_8253_interval (PIT_8253_DIVIDER_MAX)

typedef uint16_t pit_8253_divider_t;

// The interval a divider gives.
static inline time_t pit_8253_interval (pit_8253_divider_t divider) {
	time_nanos_t tn = (time_nanos_t)(1000 * divider / PIT_8253_BASE_MEGAHZ);

	return time_from_nanos (tn);
}

// Set the interupt interval (for mode 3, Square Wave Generator)
void pit_8253_set_interval (time_t);
dev_driver_t *pit_8253_get_device_driver ();

#endif

// vim: set ts=4 sw=4 noet syn=c:
