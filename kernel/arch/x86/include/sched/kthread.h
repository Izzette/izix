// kernel/arch/x86/include/sched/kthread.h

#ifndef IZIX_KTHREADS_H
#define IZIX_KTHREADS_H 1

#include <stdbool.h>

typedef int kpid_t;

void kthread_init ();
kpid_t kthread_new_task (void (*) ());
void kthread_end_task ();
void kthread_yeild ();
bool kthread_wake (kpid_t);
void kthread_block ();
kpid_t kthread_get_running ();
void kthread_run ();

#endif

// vim: set ts=4 sw=4 noet syn=c:
