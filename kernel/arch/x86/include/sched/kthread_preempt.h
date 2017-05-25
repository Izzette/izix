// kernel/arch/x86/include/sched/kthread_preempt.h

#ifndef IZIX_KTHREAD_PREEMPT_H
#define IZIX_KTHREAD_PREEMPT_H 1

void kthread_preempt_enable ();
void kthread_preempt_disable ();
void kthread_preempt_slow ();
void kthread_preempt_fast ();

#endif

// vim: set ts=4 sw=4 noet syn=c:
