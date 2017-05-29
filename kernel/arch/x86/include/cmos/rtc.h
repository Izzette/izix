// kernel/arch/x86/include/cmos/rtc.h

#ifndef IZIX_RTC_H
#define IZIX_RTC_H 1

#include <stdint.h>
#include <stdbool.h>

#include <dev/dev_driver.h>
#include <time/time.h>

// Higher rates are faster, lower rates are slower.
#define RTC_FASTEST_RATE ((rtc_rate_t)3)
#define RTC_SLOWEST_RATE ((rtc_rate_t)15)
#define RTC_DISABLE_RATE ((rtc_rate_t)0)

typedef uint8_t rtc_rate_t;

typedef struct rtc_datetime_struct {
	unsigned char seconds  : 6;
	unsigned char minutes  : 6;
	unsigned char hours    : 5;
	// Unreliable.
	unsigned char weekday  : 3;
	// Unreliable.
	unsigned char monthday : 5;
	unsigned char month    : 4;
	unsigned char year     : 7;
	unsigned char century  : 7;
} rtc_datetime_t;

time_t rtc_interval_from_rate (rtc_rate_t);
void rtc_set_rate (rtc_rate_t);
rtc_rate_t rtc_get_rate ();
void rtc_irq_enable ();
void rtc_irq_disable ();
rtc_datetime_t rtc_get_datetime ();
dev_driver_t *rtc_get_device_driver ();

#endif

// vim: set ts=4 sw=4 noet syn=c:
