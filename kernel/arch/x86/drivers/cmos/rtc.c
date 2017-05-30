// kernel/arch/x86/drivers/cmos/rtc.c

#include <stddef.h>
#include <stdint.h>

#include <attributes.h>
#include <format.h>

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

#if !defined(IZIX)
#define COMPILE_CENTURY 20
#define COMPILE_YEAR    17
#endif

#define RTC_BASE_HZ 32768

typedef enum rtc_in_flight_enum {
	rtc_stable             = 0b0,
	rtc_update_in_progress = 0b1
} rtc_in_flight_t;

typedef enum rtc_24h_enum {
	rtc_12h = 0b0,
	rtc_24h = 0b1
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
	rtc_rate_t      rate      : 4;
	unsigned char   _rsv0     : 3; // ???
	rtc_in_flight_t in_flight : 1;
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

static bool rtc_is_in_flight () {
	rtc_status_a_t status_a = rtc_get_status_a ();

	return rtc_update_in_progress == status_a.in_flight;
}

// Just say no century register available for now ...
FAST
static void rtc_read_datetime (
		unsigned char *seconds_register_ptr,
		unsigned char *minutes_register_ptr,
		unsigned char *hours_register_ptr,
		unsigned char *weekday_register_ptr,
		unsigned char *monthday_register_ptr,
		unsigned char *month_register_ptr,
		unsigned char *year_register_ptr
) {
	*seconds_register_ptr  = cmos_get (cmosr_rtc_seconds);
	*minutes_register_ptr  = cmos_get (cmosr_rtc_minutes);
	*hours_register_ptr    = cmos_get (cmosr_rtc_hours);
	*weekday_register_ptr  = cmos_get (cmosr_rtc_weekday);
	*monthday_register_ptr = cmos_get (cmosr_rtc_monthday);
	*month_register_ptr    = cmos_get (cmosr_rtc_month);
	*year_register_ptr     = cmos_get (cmosr_rtc_year);
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
	uint64_t rtc_div_hz = RTC_BASE_HZ >> (rate - 1);
	time_nanos_t ns = TIME_NANOS_PER_SEC / rtc_div_hz;

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

FAST
rtc_datetime_t rtc_get_datetime () {
	const rtc_status_b_t status_b = rtc_get_status_b ();

	unsigned char
		seconds_register, minutes_register,  hours_register,
		weekday_register, monthday_register, month_register,
		year_register;

	for (;;) {
		// Wait until update completes.
		while (rtc_is_in_flight ());

		rtc_read_datetime (
			&seconds_register, &minutes_register,  &hours_register,
			&weekday_register, &monthday_register, &month_register,
			&year_register);

		if (rtc_is_in_flight ())
			continue;

		unsigned char
			seconds_register_2nd, minutes_register_2nd,  hours_register_2nd,
			weekday_register_2nd, monthday_register_2nd, month_register_2nd,
			year_register_2nd;

		rtc_read_datetime (
			&seconds_register_2nd, &minutes_register_2nd,  &hours_register_2nd,
			&weekday_register_2nd, &monthday_register_2nd, &month_register_2nd,
			&year_register_2nd);

		if (	seconds_register  != seconds_register_2nd  ||
				minutes_register  != minutes_register_2nd  ||
				hours_register    != hours_register_2nd    ||
				weekday_register  != weekday_register_2nd  ||
				monthday_register != monthday_register_2nd ||
				month_register    != month_register_2nd    ||
				year_register     != year_register_2nd)
			continue;

		break;
	}

	// Will be unset if binary time.
	const unsigned char hours_register_offset =
		12 * (hours_register >> 7);
	const unsigned char hours_register_low =
		(hours_register & 0x7f) - (rtc_12h == status_b.hour_format ? 1 : 0);

	unsigned char
		seconds,  minutes, hours, weekday,
		monthday, month,   year,  century;
	if (rtc_bcd == status_b.numeric) {
		seconds  = bcd_to_bin (seconds_register);
		minutes  = bcd_to_bin (minutes_register);
		hours    = hours_register_offset + bcd_to_bin (hours_register_low);
		weekday  = bcd_to_bin (weekday_register);
		monthday = bcd_to_bin (monthday_register);
		month    = bcd_to_bin (month_register);
		year     = bcd_to_bin (year_register);
	} else {
		seconds  = seconds_register;
		minutes  = minutes_register;
		hours    = hours_register_offset + hours_register_low;
		weekday  = weekday_register;
		monthday = monthday_register;
		month    = month_register;
		year     = year_register;
	}

	if (year < COMPILE_YEAR)
		// Century rolled over (hopefuly just once ...)
		century = COMPILE_CENTURY + 1;
	else
		century = COMPILE_CENTURY;

	const rtc_datetime_t datetime = {
		.seconds  = seconds,
		.minutes  = minutes,
		.hours    = hours,
		.weekday  = weekday,
		.monthday = monthday,
		.month    = month,
		.year     = year,
		.century  = century
	};

	return datetime;
}

dev_driver_t *rtc_get_device_driver () {
	return &rtc_driver;
}

// vim: set ts=4 sw=4 noet syn=c:
