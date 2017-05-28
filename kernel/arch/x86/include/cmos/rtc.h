// kernel/arch/x86/include/cmos/rtc.h

#ifndef IZIX_RTC_H
#define IZIX_RTC_H 1

#include <stdint.h>

#include <dev/dev_driver.h>
#include <time/time.h>

// Higher rates are faster, lower rates are slower.
#define RTC_FASTEST_RATE ((rtc_rate_t)3)
#define RTC_SLOWEST_RATE ((rtc_rate_t)15)
#define RTC_DISABLE_RATE ((rtc_rate_t)0)

typedef uint8_t rtc_rate_t;

time_t rtc_interval_from_rate (rtc_rate_t);
void rtc_set_rate (rtc_rate_t);
rtc_rate_t rtc_get_rate ();
void rtc_irq_enable ();
void rtc_irq_disable ();
dev_driver_t *rtc_get_device_driver ();

#endif

// vim: set ts=4 sw=4 noet syn=c:
