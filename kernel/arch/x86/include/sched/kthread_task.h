// kernel/arch/x86/include/sched/kthread_task.h

#ifndef IZIX_KTHREAD_TASK_H
#define IZIX_KTHREAD_TASK_H 1

#include <stdbool.h>

#include <sched/kthread_switch.h>
#include <sched/kthread_kpid.h>
#include <sched/kthread.h>

typedef kthread_registers_t kthread_task_t;

kthread_task_t new_kthread_task (void (*) (), void *stack);
kthread_task_t new_kthread_task_from_running ();

#endif

// vim: set ts=4 sw=4 noet syn=c:
