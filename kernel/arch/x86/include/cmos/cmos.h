// kernel/arch/x86/include/cmos/cmos.h

#ifndef IZIX_CMOS_H
#define IZIX_CMOS_H 1

#include <attributes.h>

typedef enum nmi_enable_enum {
	nmi_enabled  = 0b0,
	nmi_disabled = 0b1
} nmi_enable_t;

typedef enum cmos_register_enum {
	cmosr_rtc_seconds  = 0x00,
	cmosr_rtc_minutes  = 0x02,
	cmosr_rtc_hours    = 0x04,
	cmosr_rtc_weekday  = 0x06,
	cmosr_rtc_monthday = 0x07,
	cmosr_rtc_month    = 0x08,
	cmosr_rtc_year     = 0x09,
	cmosr_rtc_status_a = 0x0a,
	cmosr_rtc_status_b = 0x0b,
	cmosr_rtc_status_c = 0x0c,
	cmosr_floppy_type  = 0x10,
// Century is unreliable, query ACPI to be sure.
	cmosr_rtc_century  = 0x32
} cmos_register_t;

typedef struct PACKED cmos_command_struct {
// CMOS register selection.
	cmos_register_t cmosr : 7;
// NMI enable/disable bit.
	nmi_enable_t    nmi   : 1;
} MAY_ALIAS cmos_command_t;

FASTCALL
void cmos_set_cmd (cmos_command_t);
FASTCALL
char cmos_get (cmos_register_t);
// NEVER EVER set a non RTC register!  They are often used by the BIOS and may cause a
// failure to boot.  This function will kpanic if it recieves a non-RTC register.
void cmos_set (char, cmos_register_t);

#endif

// vim: set ts=4 sw=4 noet syn=c:
