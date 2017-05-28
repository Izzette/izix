// kernel/arch/x86/drivers/cmos/nmi.c

#include <cmos/cmos.h>

void enable_nmi () {
	cmos_command_t cmd = cmos_get_cmd ();
	cmd.nmi = nmi_enabled;
	cmos_set_cmd (cmd);
}

void disable_nmi () {
	cmos_command_t cmd = cmos_get_cmd ();
	cmd.nmi = nmi_disabled;
	cmos_set_cmd (cmd);
}

// vim: set ts=4 sw=4 noet syn=c:
