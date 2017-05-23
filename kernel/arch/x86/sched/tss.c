// kernel/arch/x86/sched/tss.c

#include <kprint/kprint.h>
#include <kpanic/kpanic.h>
#include <sched/tss.h>
#include <mm/segment.h>
#include <mm/gdt.h>
#include <mm/malloc.h>

static tss_t *tss_task_state_segment;
static void *tss_esp;

void tss_init (void *esp) {
	tss_esp = esp;

	tss_logical_t logical_tss = new_tss_logical ();

	logical_tss.ss0  = GDT_SUPERVISOR_DATA_SELECTOR;
	logical_tss.esp0 = esp;
	logical_tss.iopb = sizeof(tss_t);
	if (!logical_tss.ss0) {
		kputs ("sched/tss: Failed to retrieve a valid data segment selector!\n");
		kpanic ();
	}

	tss_task_state_segment = malloc (sizeof(tss_t));
	if (!tss_task_state_segment) {
		kputs ("sched/tss: Failed to allocate TSS!\n");
		kpanic ();
	}

	*tss_task_state_segment = tss_encode (logical_tss);
}

tss_t *tss_get () {
	return tss_task_state_segment;
}

void tss_load (segment_selector_t tss_selector) {
	asm volatile (
		"		ltr		%0;\n"
		:
		:"r"(tss_selector));

	kprintf ("sched/tss: TSS selector 0x%04hx loaded successfuly.\n", tss_selector);
}

// vim: set ts=4 sw=4 noet syn=c:
