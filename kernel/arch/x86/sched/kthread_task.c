// kernel/arch/x86/sched/kthread_task.c

#include <stddef.h>
#include <stdbool.h>

#include <sched/kthread_switch.h>
#include <sched/kthread_bootstrap.h>
#include <sched/kthread_task.h>

kthread_task_t new_kthread_task (void (*entry) (), void *stack) {
	kthread_registers_t new_registers = new_kthread_registers (stack);

	// Argument to kthread_bootstrap, will call.
	kthread_registers_push_caller (&new_registers, entry);

	// Saved caller address.
	kthread_registers_pushl (&new_registers, 0);
	// Our bootstrap function (actually entry point).
	kthread_registers_push_caller (&new_registers, kthread_bootstrap);

	return (kthread_task_t)new_registers;
}

kthread_task_t new_kthread_task_from_running () {
	kthread_registers_t new_registers = new_kthread_registers (NULL);

	kthread_switch (&new_registers, &new_registers);

	return (kthread_task_t)new_registers;
}

// vim: set ts=4 sw=4 noet syn=c:
