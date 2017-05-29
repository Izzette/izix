// kernel/arch/x86/time/clock_tick.c

#include <attributes.h>

#include <irq/irq.h>
#include <pit_8253/pit_8253.h>
#include <cmos/cmos.h>
#include <cmos/rtc.h>
#include <time/time.h>
#include <time/clock.h>
#include <time/clock_tick.h>

// TODO: define based on bogomips.
#define CLOCK_TICK_RTC_RATE 10
#define CLOCK_TICK_PIT_INTERVAL (time_from_millis (1))

#pragma GCC diagnostic ignored "-Wunused-parameter"
FASTCALL FAST HOT
static void clock_tick_pit_8253_irq0_hook (irq_t irq) {
	clock_fast_add (pit_8253_current_interval);
}

FASTCALL FAST HOT
static void clock_tick_rtc_irq8_hook (irq_t irq) {
	clock_tick ();
	cmos_get (cmosr_rtc_status_c);
}
#pragma GCC diagnostic pop

void clock_tick_start () {
	clock_real_interval_multiplier = rtc_interval_from_rate (CLOCK_TICK_RTC_RATE);
	// rtc_interval_from_rate is gurenteed to produce a multiplier that does not truncate
	// any bits.
	clock_real_interval_divisor = 1;

	pit_8253_set_interval (CLOCK_TICK_PIT_INTERVAL);

	rtc_set_rate (CLOCK_TICK_RTC_RATE);
	rtc_irq_enable ();

	irq_add_pre_hook (0, clock_tick_pit_8253_irq0_hook);
	irq_add_pre_hook (8, clock_tick_rtc_irq8_hook);

	// Clear any pending RTC interupts.
	cmos_get (cmosr_rtc_status_c);
}

// vim: set ts=4 sw=4 noet syn=c:
