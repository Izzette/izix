// kernel/arch/x86/drivers/cmos/nmi.c

#include <cmos/cmos.h>

nmi_enable_t nmi_enable = nmi_enabled;

static void nmi_update () {
	cmos_command_t cmd = {
		.nmi = nmi_enable,
		.cmosr = 127 // Appears to be the default.
	};

	cmos_set_cmd (cmd);
}

void enable_nmi () {
	nmi_enable = nmi_enabled;
	nmi_update ();
}

void disable_nmi () {
	nmi_enable = nmi_disabled;
	nmi_update ();
}

// vim: set ts=4 sw=4 noet syn=c:
