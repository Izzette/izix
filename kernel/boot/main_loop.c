// kernel/boot/main_loop.c

#include <kprint/kprint.h>
#include <sched/kthread.h>

#if defined(ARCH_X86)
# include <asm/toggle_int.h>
# include <asm/halt.h>
#endif

void other_task () {
	kprintf ("hello world from kpid: %d!\n", kthread_get_running ());
}

void main_loop () {
#if defined(ARCH_X86)
	enable_int ();
#endif

	kthread_init ();
	kpid_t kpid = kthread_new_task (other_task);
	kprintf ("child kpid: %d\n", kpid);

	for (;;) {
#if defined(ARCH_X86)
		halt ();
		kthread_run ();
#endif
	}
}

// vim: set ts=4 sw=4 noet syn=c:
