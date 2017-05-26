// kerenl/arch/x86/include/sched/kthread_bootstrap.h

#ifndef IZIX_KTHREAD_BOOTSTRAP_H
#define IZIX_KTHREAD_BOOTSTRAP_H 1

#include <attributes.h>

void kthread_bootstrap (void (*) ())
	NORETURN;

#endif

// vim: set ts=4 sw=4 noet syn=c:
