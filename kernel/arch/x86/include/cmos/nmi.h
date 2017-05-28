// kernel/arch/x86/include/cmos/nmi.h

#ifndef IZIX_NMI_H
#define IZIX_NMI_H 1

#include <cmos/cmos.h>

extern nmi_enable_t nmi_enable;

void enable_nmi ();
void disable_nmi ();

#endif

// vim: set ts=4 sw=4 noet syn=c:
