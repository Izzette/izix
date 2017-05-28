// kernel/arch/x86/drivers/cmos/rtc.c

#include <stddef.h>
#include <stdint.h>

#include <attributes.h>

#include <kprint/kprint.h>
#include <kpanic/kpanic.h>
#include <dev/dev_driver.h>
#include <dev/dev_types.h>
#include <mm/page.h>
#include <time/time.h>
#include <cmos/rtc.h>
#include <cmos/nmi.h>
#include <cmos/cmos.h>
#include <asm/toggle_int.h>

#define RTC_BASE_MHZ 32768000000ULL

typedef enum rtc_24h_enum {
	rtc_24h_disable = 0b0,
	rtc_24h_enable  = 0b1
} rtc_24h_t;

typedef enum rtc_numeric_enum {
	rtc_bcd = 0b0,
	rtc_bin = 0b1
} rtc_numeric_t;

typedef enum rtc_tick_enum {
	rtc_tick_disable = 0b0,
	rtc_tick_enable  = 0b1
} rtc_tick_t;

typedef struct PACKED rtc_status_a_struct {
	rtc_rate_t    rate  : 4;
	unsigned char _rsv0 : 4; // ???
} MAY_ALIAS rtc_status_a_t;


typedef struct PACKED rtc_status_b_struct {
	unsigned char _rsv0       : 1; // ???
// These bits cannot be changed on every RTC chip.
	rtc_24h_t     hour_format : 1;
	rtc_numeric_t numeric     : 1;
	unsigned char _rsv1       : 3; // ???
	rtc_tick_t    tick        : 1;
	unsigned char _rsv2       : 1; // ???
} MAY_ALIAS rtc_status_b_t;

static page_t *rtc_next_page_mapping (dev_driver_t *, page_t *, bool *);
static dev_driver_t rtc_driver = {
	.pimpl = NULL,
	.dev = {
		.maj = dev_maj_arch,
		.min = dev_min_arch_rtc
	},
	.next_page_mapping = rtc_next_page_mapping
};

#pragma GCC diagnostic ignored "-Wunused-parameter"
static page_t *rtc_next_page_mapping (
		dev_driver_t *this,
		page_t *last_page,
		bool *need_write
) {
	return NULL;
}
#pragma GCC diagnostic pop

#define MKRTC_GET_STATUS(x) \
static rtc_status_##x##_t rtc_get_status_##x () { \
	char status_##x##_register = cmos_get (cmosr_rtc_status_##x); \
	return *(rtc_status_##x##_t *)&status_##x##_register; \
} \
\
static void rtc_set_status_##x (rtc_status_##x##_t status_##x) { \
	cmos_set (*(char *)&status_##x, cmosr_rtc_status_##x); \
}

MKRTC_GET_STATUS(a)
MKRTC_GET_STATUS(b)

static bool rtc_validate_rate (rtc_rate_t rate) {
	return (RTC_DISABLE_RATE == rate ||
			(RTC_SLOWEST_RATE >= rate && RTC_FASTEST_RATE <= rate));
}

void rtc_set_rate (rtc_rate_t rate) {
	if (!rtc_validate_rate (rate)) {
		kputs ("cmos/rtc: Attempt to set rate out of bounds!\n");
		kpanic ();
	}

	const bool int_enabled = int_is_enabled ();

	disable_int ();
	disable_nmi ();

	rtc_status_a_t status_a = rtc_get_status_a ();
	status_a.rate = rate;
	rtc_set_status_a (status_a);

	enable_nmi ();
	if (int_enabled)
		enable_int ();
}

rtc_rate_t rtc_get_rate () {
	rtc_status_a_t status_a = rtc_get_status_a ();
	rtc_rate_t rate = status_a.rate;

	if (!rtc_validate_rate (rate))
		kprintf (
			"cmos/rtc: [W] Recieved out of bounds rate from RTC: %hhd!\n",
			rate);

	return rate;
}

time_t rtc_interval_from_rate (rtc_rate_t rate) {
	uint64_t rtc_div_mhz = RTC_BASE_MHZ >> (rate - 1);
	time_nanos_t ns =
		(TIME_NANOS_PER_SEC * TIME_MICROS_PER_SEC) / rtc_div_mhz;

	return time_from_nanos (ns);
}

void rtc_irq_enable () {
	const bool int_enabled = int_is_enabled ();

	disable_int ();
	disable_nmi ();

	rtc_status_b_t status_b = rtc_get_status_b ();
	status_b.tick = rtc_tick_enable;
	rtc_set_status_b (status_b);

	enable_nmi ();
	if (int_enabled)
		enable_int ();
}

void rtc_irq_disable () {
	const bool int_enabled = int_is_enabled ();

	disable_int ();
	disable_nmi ();

	rtc_status_b_t status_b = rtc_get_status_b ();
	status_b.tick = rtc_tick_disable;
	rtc_set_status_b (status_b);

	enable_nmi ();
	if (int_enabled)
		enable_int ();
}

dev_driver_t *rtc_get_device_driver () {
	return &rtc_driver;
}

// vim: set ts=4 sw=4 noet syn=c:
