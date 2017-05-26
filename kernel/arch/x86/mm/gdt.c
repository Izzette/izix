// kernel/arch/x86/mm/gdt.c

#include <stddef.h>
#include <stdint.h>

#include <attributes.h>

#include <kprint/kprint.h>
#include <kpanic/kpanic.h>
#include <mm/malloc.h>
#include <mm/gdt.h>
#include <sched/tss.h>

#ifndef MAX
#define MAX(a, b) \
	(a > b ? a : b)
#endif

#define GDT_MAX_SELECTOR \
	MAX(GDT_SUPERVISOR_CODE_SELECTOR, \
	MAX(GDT_SUPERVISOR_DATA_SELECTOR, \
	MAX(GDT_USERSPACE_CODE_SELECTOR, \
	MAX(GDT_USERSPACE_DATA_SELECTOR, \
		GDT_SUPERVISOR_TSS_SELECTOR))))

#define GDT_LENGTH \
	(GDT_MAX_SELECTOR + sizeof(gdt_entry_t))

typedef enum gdt_accessed_enum {
	gdt_unaccessed = 0b0,
	gdt_accessed   = 0b1
} gdt_accessed_t;

typedef enum gdt_code_readable_enum {
	gdt_code_x  = 0b0,
	gdt_code_rx = 0b1
} gdt_code_readable_t;

typedef enum gdt_data_writable_enum {
	gdt_data_r  = 0b0,
	gdt_data_rw = 0b1
} gdt_data_writable_t;

typedef enum gdt_code_conforming_enum {
	gdt_code_non_conforming = 0b0,
	gdt_code_conforming     = 0b1
} gdt_code_conforming_t;

typedef enum gdt_data_direction_enum {
	gdt_data_up   = 0b0,
	gdt_data_down = 0b1
} gdt_data_direction_t;

typedef enum gdt_executable_enum {
	gdt_data = 0b0,
	gdt_code = 0b1
} gdt_executable_t;

typedef enum gdt_ring_enum {
	gdt_ring_zero  = 0b00,
	gdt_ring_one   = 0b01,
	gdt_ring_two   = 0b10,
	gdt_ring_three = 0b11
} gdt_ring_t;

typedef enum gdt_present_enum {
	gdt_absent  = 0b0,
	gdt_present = 0b1
} gdt_present_t;

typedef enum gdt_size_enum {
	gdt_16bit = 0b0,
	gdt_32bit = 0b1
} gdt_size_t;

typedef enum gdt_granularity_enum {
	gdt_granularity_byte = 0b0,
	gdt_granularity_page = 0b1
} gdt_granularity_t;


#define GDT_LIMIT_LOW_LENGTH  020
#define GDT_LIMIT_LOW_OFFSET  000
#define GDT_LIMIT_HIGH_LENGTH 004
#define GDT_LIMIT_HIGH_OFFSET (GDT_LIMIT_LOW_LENGTH + GDT_LIMIT_LOW_OFFSET)

#define GDT_BASE_LOW_LENGTH  030
#define GDT_BASE_LOW_OFFSET  000
#define GDT_BASE_HIGH_LENGTH 010
#define GDT_BASE_HIGH_OFFSET (GDT_BASE_LOW_LENGTH + GDT_BASE_LOW_OFFSET)

#define GDT_TSS_ACCESS ((gdt_access_tss_t)0x89)

#define GDT_NULL ((gdt_entry_null_t)0x0000000000000000)

typedef struct PACKED gdt_access_code_struct {
	gdt_accessed_t        accessed   : 1;
	gdt_code_readable_t   readable   : 1;
	gdt_code_conforming_t conforming : 1;
	gdt_executable_t      executable : 1; // Always gdt_code
	unsigned char         _rsv0      : 1; // Always 0b1
	gdt_ring_t            ring       : 2; // Protection level
	gdt_present_t         present    : 1;
} gdt_access_code_t;

typedef struct PACKED gdt_access_data_struct {
	gdt_accessed_t       accessed   : 1;
	gdt_data_writable_t  writable   : 1;
	gdt_data_direction_t direction  : 1;
	gdt_executable_t     executable : 1; // Always gdt_data
	unsigned char        _rsv0      : 1; // Always 0b1
	gdt_ring_t           ring       : 2; // Protection level
	gdt_present_t        present    : 1;
} gdt_access_data_t;

typedef uint8_t gdt_access_tss_t;

typedef struct PACKED gdt_entry_code_struct {
	uint32_t          limit_low   : GDT_LIMIT_LOW_LENGTH;
	uint32_t          base_low    : GDT_BASE_LOW_LENGTH;
	gdt_access_code_t access;
	uint32_t          limit_high  : GDT_LIMIT_HIGH_LENGTH;
	// flags ...
	unsigned char     _rsv0       : 2; // Always 0b00
	gdt_size_t        size        : 1;
	gdt_granularity_t granularity : 1;
	uint32_t          base_high   : GDT_BASE_HIGH_LENGTH;
} gdt_entry_code_t;

typedef struct PACKED gdt_entry_data_struct {
	uint32_t          limit_low   : GDT_LIMIT_LOW_LENGTH;
	uint32_t          base_low    : GDT_BASE_LOW_LENGTH;
	gdt_access_data_t access;
	uint32_t          limit_high  : GDT_LIMIT_HIGH_LENGTH;
	// flags ...
	unsigned char     _rsv0       : 2; // Always 0b00
	gdt_size_t        size        : 1;
	gdt_granularity_t granularity : 1;
	uint32_t          base_high   : GDT_BASE_HIGH_LENGTH;
} gdt_entry_data_t;

typedef struct PACKED gdt_entry_tss_struct {
	uint32_t          limit_low   : GDT_LIMIT_LOW_LENGTH;
	uint32_t          base_low    : GDT_BASE_LOW_LENGTH;
	gdt_access_tss_t  access;
	uint32_t          limit_high  : GDT_LIMIT_HIGH_LENGTH;
	// flags ...
	unsigned char     _rsv0       : 2; // Always 0b00
	gdt_size_t        size        : 1; // Always gdt_32bit
	gdt_granularity_t granularity : 1; // Always gdt_granularity_byte
	uint32_t          base_high   : GDT_BASE_HIGH_LENGTH;
} gdt_entry_tss_t;

typedef uint64_t gdt_entry_null_t;

typedef union gdt_entry_union {
	gdt_entry_code_t   code;
	gdt_entry_data_t   data;
	gdt_entry_tss_t    tss;
	gdt_entry_null_t   null;
} gdt_entry_t;

typedef struct PACKED gdt_register_struct {
	uint16_t size; // Byte length minus 1;
	gdt_entry_t *offset;
} gdt_register_t;

static gdt_register_t gdtr;

COLD
static void gdt_populate (tss_t *tss) {
	const size_t s_null_i = 0;
	const size_t s_code_i = GDT_SUPERVISOR_CODE_SELECTOR / sizeof(gdt_entry_t);
	const size_t s_data_i = GDT_SUPERVISOR_DATA_SELECTOR / sizeof(gdt_entry_t);
	const size_t u_code_i = GDT_USERSPACE_CODE_SELECTOR  / sizeof(gdt_entry_t);
	const size_t u_data_i = GDT_USERSPACE_DATA_SELECTOR  / sizeof(gdt_entry_t);
	const size_t s_tss_i  = GDT_SUPERVISOR_TSS_SELECTOR  / sizeof(gdt_entry_t);

	gdtr.offset[s_null_i].null = GDT_NULL;

	gdtr.offset[s_code_i].code = (gdt_entry_code_t){
		.limit_low   = 0xffff,
		.base_low    = 0x000000,
		.access      = {
			.accessed   = gdt_unaccessed,
			.readable   = gdt_code_rx,
			.conforming = gdt_code_conforming,
			.executable = gdt_code,
			._rsv0      = 0b1,
			.ring       = gdt_ring_zero,
			.present    = gdt_present
		},
		.limit_high  = 0xf,
		.size        = gdt_32bit,
		.granularity = gdt_granularity_page,
		.base_high   = 0x00
	};

	// Data is just like code.
	gdtr.offset[s_data_i].code = gdtr.offset[s_code_i].code;
	gdtr.offset[s_data_i].data.access.writable   = gdt_data_rw;
	gdtr.offset[s_data_i].data.access.direction  = gdt_data_up;
	gdtr.offset[s_data_i].data.access.executable = gdt_data;

	// Userspace code is just like supervisor.
	gdtr.offset[u_code_i].code = gdtr.offset[s_code_i].code;
	gdtr.offset[u_code_i].code.access.ring = gdt_ring_three;

	// Userspace data is just like supervisor.
	gdtr.offset[u_data_i].data = gdtr.offset[s_data_i].data;
	gdtr.offset[u_data_i].data.access.ring = gdt_ring_three;

	gdtr.offset[s_tss_i].tss = (gdt_entry_tss_t){
		.limit_low = sizeof(tss_t),
		.base_low = (size_t)tss,
		.access = GDT_TSS_ACCESS,
		.limit_high = sizeof(tss_t) >> GDT_LIMIT_HIGH_OFFSET,
		.size = gdt_32bit,
		.granularity = gdt_granularity_byte,
		.base_high = (size_t)tss >> GDT_BASE_HIGH_OFFSET
	};
}

COLD
static void gdt_load () {
	asm volatile (
		"		lgdt	(%0);\n"
		"		ljmp	%1,				$.Lgdt_load_fin;\n"
		".Lgdt_load_fin:\n"
		"		nop;\n"
		:
		:"r"(&gdtr), "i"(GDT_SUPERVISOR_CODE_SELECTOR));

	kputs ("mm/gdt: GDT loaded successfuly.\n");
}

COLD
void gdt_init (tss_t *tss) {
	// One NULL selector, a code and data selector for supervisor and userland, and one
	// TSS selector.
	gdtr.size = GDT_LENGTH - 1;

	gdtr.offset = malloc (GDT_LENGTH);
	if (!gdtr.offset) {
		kputs ("mm/gdt: Failed to allocate GDT entries!\n");
		kpanic ();
	}

	gdt_populate (tss);

	gdt_load ();
}

// vim: set ts=4 sw=4 noet syn=c:
