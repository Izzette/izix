// kernel/arch/x86/sched/kthread_preempt.c

#include <stdbool.h>

#include <kprint/kprint.h>
#include <irq/irq.h>
#include <sched/kthread.h>
#include <sched/spinlock.h>

static volatile bool kthread_preempt_init = false;
static spinlock_t
	kthread_preempt_lock_base,
	*kthread_preempt_lock = &kthread_preempt_lock_base;

#pragma GCC diagnostic ignored "-Wunused-parameter"
__attribute__((optimize("O3"))) // Interupt handlers must be very fast!
static void kthread_pit_825x_irq0_hook (irq_t irq) {
	if (spinlock_is_locked (kthread_preempt_lock))
		return;

	// If we can't obtain the exclusive lock, we return to the task because switching
	// isn't safe or desirable for the currently running thread.
	if (!kthread_lock_task ()) {
		kthread_unlock_task ();
		return;
	}

	kthread_yield ();

	kthread_unlock_task ();
}
#pragma GCC diagnostic pop

void kthread_preempt_enable () {
	if (!kthread_preempt_init) {
		kthread_preempt_init = true;

		kthread_preempt_lock_base = new_spinlock ();

		irq_add_post_hook (0, kthread_pit_825x_irq0_hook);

		kputs ("sched/kthread_preempt: Preemptive multitasking enabled.\n");

		return;
	}

	if (spinlock_is_locked (kthread_preempt_lock))
		spinlock_release (kthread_preempt_lock);
}

void kthread_preempt_disable () {
	if (!kthread_preempt_init)
		return;

	// Just make sure it's locked.
	spinlock_try_lock (kthread_preempt_lock);
}

// vim: set ts=4 sw=4 noet syn=c:
