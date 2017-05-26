// kernel/arch/x86/include/mm/tss.h

#ifndef IZIX_TSS_H
#define IZIX_TSS_H 1

#include <stdint.h>

#include <attributes.h>
#include <string.h>

#include <mm/segment.h>

typedef struct tss_struct tss_t;

typedef struct PACKED tss_struct {
	volatile uint16_t link;
	uint16_t _rsv0;

	// TODO: are this actually volatile?
	volatile uint32_t esp0;
	volatile uint16_t ss0;
	uint16_t _rsv1;
	volatile uint32_t esp1;
	volatile uint16_t ss1;
	uint16_t _rsv2;
	volatile uint32_t esp2;
	volatile uint16_t ss2;
	uint16_t _rsv3;

	volatile uint32_t cr3;

	volatile uint32_t eip;

	volatile uint32_t eflags;

	volatile uint32_t eax;
	volatile uint32_t ecx;
	volatile uint32_t edx;
	volatile uint32_t ebx;

	volatile uint32_t esp;
	volatile uint32_t ebp;

	volatile uint32_t esi;
	volatile uint32_t edi;

	volatile uint16_t es;
	uint16_t _rsv4;
	volatile uint16_t cs;
	uint16_t _rsv5;
	volatile uint16_t ss;
	uint16_t _rsv6;
	volatile uint16_t ds;
	uint16_t _rsv7;
	volatile uint16_t fs;
	uint16_t _rsv8;
	volatile uint16_t gs;
	uint16_t _rsv9;

	volatile uint16_t ldt;
	uint16_t _rsv10;

	uint16_t _rsv11;
	volatile uint16_t iopb;
} tss_t;

typedef struct tss_logical_struct {
	segment_selector_t link;

	segment_selector_t ss0;
	segment_selector_t ss1;
	segment_selector_t ss2;

	void *esp0;
	void *esp1;
	void *esp2;

	uint32_t cr3;

	void *eip;

	uint32_t eflags;

	uint32_t eax;
	uint32_t ecx;
	uint32_t edx;
	uint32_t ebx;

	void *esp;
	void *ebp;

	uint32_t esi;
	uint32_t edi;

	segment_selector_t es;
	segment_selector_t cs;
	segment_selector_t ss;
	segment_selector_t ds;
	segment_selector_t fs;
	segment_selector_t gs;

	uint16_t ldt;

	uint16_t iopb;
} tss_logical_t;

static inline tss_logical_t new_tss_logical () {
	tss_logical_t logical_tss;

	memset (&logical_tss, 0, sizeof(logical_tss));

	return logical_tss;
}

static inline tss_t new_tss () {
	tss_t tss;

	memset (&tss, 0, sizeof(tss));

	return tss;
}

static inline tss_logical_t tss_decode (tss_t tss) {
#pragma GCC diagnostic ignored "-Wint-to-pointer-cast"
	tss_logical_t logical_tss = {
		.link = tss.link,

		.ss0 = tss.ss0,
		.ss1 = tss.ss1,
		.ss2 = tss.ss2,

		.esp0 = (void *)tss.esp0,
		.esp1 = (void *)tss.esp1,
		.esp2 = (void *)tss.esp2,

		.cr3 = tss.cr3,

		.eip = (void *)tss.eip,

		.eflags = tss.eflags,

		.eax = tss.eax,
		.ecx = tss.ecx,
		.edx = tss.edx,
		.ebx = tss.ebx,

		.esp = (void *)tss.esp,
		.ebp = (void *)tss.ebp,

		.esi = tss.esi,
		.edi = tss.edi,

		.es = tss.es,
		.cs = tss.cs,
		.ss = tss.ss,
		.ds = tss.ds,
		.fs = tss.fs,
		.gs = tss.gs,

		.ldt = tss.ldt,

		.iopb = tss.iopb
	};
#pragma GCC diagnostic pop

	return logical_tss;
}

static inline tss_t tss_encode (tss_logical_t logical_tss) {
#pragma GCC diagnostic ignored "-Wpointer-to-int-cast"
	tss_t tss = {
		.link = logical_tss.link,
		._rsv0 = 0,

		.esp0 = (uint32_t)logical_tss.esp0,
		.ss0 = logical_tss.ss0,
		._rsv1 = 0,
		.esp1 = (uint32_t)logical_tss.esp1,
		.ss1 = logical_tss.ss1,
		._rsv2 = 0,
		.esp2 = (uint32_t)logical_tss.esp2,
		.ss2 = logical_tss.ss2,
		._rsv3 = 0,

		.cr3 = logical_tss.cr3,

		.eip = (uint32_t)logical_tss.eip,

		.eflags = logical_tss.eflags,

		.eax = logical_tss.eax,
		.ecx = logical_tss.ecx,
		.edx = logical_tss.edx,
		.ebx = logical_tss.ebx,

		.esp = (uint32_t)logical_tss.esp,
		.ebp = (uint32_t)logical_tss.ebp,

		.esi = logical_tss.esi,
		.edi = logical_tss.edi,

		.es = logical_tss.es,
		._rsv4 = 0,
		.cs = logical_tss.cs,
		._rsv5 = 0,
		.ss = logical_tss.ss,
		._rsv6 = 0,
		.ds = logical_tss.ds,
		._rsv7 = 0,
		.fs = logical_tss.fs,
		._rsv8 = 0,
		.gs = logical_tss.gs,
		._rsv9 = 0,

		.ldt = logical_tss.ldt,
		._rsv10 = 0,

		._rsv11 = 0,
		.iopb = logical_tss.iopb,
	};
#pragma GCC diagnostic pop

	return tss;
}

void tss_init (void *);
tss_t *tss_get ();
void tss_load (segment_selector_t);

#endif

// vim: set ts=4 sw=4 noet syn=c:
