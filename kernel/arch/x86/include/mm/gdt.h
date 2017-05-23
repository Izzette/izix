// kernel/arch/x86/mm/gdt.h

#ifndef IZIX_GDT_H
#define IZIX_GDT_H 1

#include <stdbool.h>

#include <mm/segment.h>
#include <sched/tss.h>

#define GDT_NULL_SELECTOR ((segment_selector_t)0x0000)
#define GDT_SUPERVISOR_CODE_SELECTOR ((segment_selector_t)0x0008)
#define GDT_SUPERVISOR_DATA_SELECTOR ((segment_selector_t)0x0010)
#define GDT_USERSPACE_CODE_SELECTOR ((segment_selector_t)0x0018)
#define GDT_USERSPACE_DATA_SELECTOR ((segment_selector_t)0x0020)
#define GDT_SUPERVISOR_TSS_SELECTOR  ((segment_selector_t)0x0028)

void gdt_init (tss_t *);

#endif

// vim: set ts=4 sw=4 noet syn=c:
