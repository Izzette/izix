// kernel/arch/x86/sched/tss.c

#include <kprint/kprint.h>
#include <sched/tss.h>
#include <mm/gdt.h>
#include <mm/malloc.h>

static tss_t *tss_task_state_segment;
static void *tss_esp;

void tss_init (void *esp) {
	tss_esp = esp;

	tss_logical_t logical_tss = new_tss_logical ();

	// TODO: panic if 0x00
	logical_tss.ss0  = gdt_get_first_data_selector ();
	logical_tss.esp0 = esp;
	logical_tss.iopb = sizeof(tss_t);

	// TODO: panic if NULL
	tss_task_state_segment = malloc (sizeof(tss_t));

	*tss_task_state_segment = tss_encode (logical_tss);
}

tss_t *tss_get () {
	return tss_task_state_segment;
}

void tss_load (segment_selector_t tss_selector) {
	asm (
		"	ltr		%0;\n"
		:
		:"r"(tss_selector));

	kprintf ("sched/tss: TSS selector 0x%04hx loaded.\n", tss_selector);
}

// vim: set ts=4 sw=4 noet syn=c:
