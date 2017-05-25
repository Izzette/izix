// kernel/include/time/time.h

#ifndef IZIX_TIME_H
#define IZIX_TIME_H 1

#include <stdint.h>

#define TIME_MILLIS_PER_SEC ((time_millis_t)1000)
#define TIME_MICROS_PER_SEC ((time_micros_t)1000 * TIME_MILLIS_PER_SEC)
#define TIME_NANOS_PER_SEC  ((time_nanos_t)1000 * TIME_MICROS_PER_SEC)

typedef uint64_t time_nanos_t;
typedef uint64_t time_micros_t;
typedef uint64_t time_millis_t;
typedef uint32_t time_secs_t;

typedef time_nanos_t time_t;

// Get time in seconds.
static inline time_secs_t time_secs (time_t t) {
	return t / TIME_NANOS_PER_SEC;
}

// Get time in milli-seconds.
static inline time_millis_t time_millis (time_t t) {
	return t * TIME_MILLIS_PER_SEC / TIME_NANOS_PER_SEC;
}

// Get time in micro-seconds.
static inline time_micros_t time_micros (time_t t) {
	return t * TIME_MICROS_PER_SEC / TIME_NANOS_PER_SEC;
}

// Get time in nano-seconds.
static inline time_nanos_t time_nanos (time_t t) {
	return t;
}

// Get time from seconds.
static inline time_t time_from_secs (time_secs_t ts) {
	return ts * TIME_NANOS_PER_SEC;
}

// Get time from milli-seconds.
static inline time_t time_from_millis (time_millis_t tm) {
	return tm * TIME_NANOS_PER_SEC / TIME_MILLIS_PER_SEC;
}

// Get time from micro-seconds.
static inline time_t time_from_micros (time_micros_t tu) {
	return tu * TIME_NANOS_PER_SEC / TIME_MICROS_PER_SEC;
}

// Get time from nano-seconds.
static inline time_t time_from_nanos (time_nanos_t tn) {
	return tn;
}

#endif

// vim: set ts=4 sw=4 noet syn=c:
