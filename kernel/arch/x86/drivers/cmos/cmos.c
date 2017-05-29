// kernel/arch/x86/drivers/cmos/cmos.c

#include <attributes.h>

#include <kpanic/kpanic.h>
#include <kprint/kprint.h>
#include <cmos/cmos.h>
#include <cmos/nmi.h>
#include <asm/io.h>

#define CMOS_CMD_PORT  0x70
#define CMOS_DATA_PORT 0x71

FASTCALL FAST HOT
void cmos_set_cmd (cmos_command_t cmd) {
	outb (*(char *)&cmd, CMOS_CMD_PORT);
}

FASTCALL FAST HOT
char cmos_get (cmos_register_t cmosr) {
	cmos_command_t cmd = {
		.cmosr = cmosr,
		.nmi = nmi_enable
	};

	cmos_set_cmd (cmd);

	return inb (CMOS_DATA_PORT);
}

void cmos_set (char c, cmos_register_t cmosr) {
	switch ((int)cmosr) {
		case cmosr_rtc_seconds:
		case cmosr_rtc_minutes:
		case cmosr_rtc_hours:
		case cmosr_rtc_weekday:
		case cmosr_rtc_monthday:
		case cmosr_rtc_month:
		case cmosr_rtc_century:
		case cmosr_rtc_year:
		case cmosr_rtc_status_a:
		case cmosr_rtc_status_b:
			break;
		default:
			kputs ("cmos/cmos: Attempt to write non-RTC register!\n");
			kpanic ();
	}

	cmos_command_t cmd = {
		.cmosr = cmosr,
		.nmi = nmi_enable
	};

	cmos_set_cmd (cmd);

	outb (c, CMOS_DATA_PORT);
}

// vim: set ts=4 sw=4 noet syn=c:
