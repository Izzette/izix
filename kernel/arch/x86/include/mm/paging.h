// kernel/arch/x86/include/mm/paging.h

#ifndef IZIX_PAGING_H
#define IZIX_PAGING_H 1

#include <stddef.h>
#include <stdbool.h>

#include <attributes.h>

#include <mm/page.h>

#define PAGE_ALIGN_UPPER_WIDTH 024
#define PAGE_ALIGN_LOWER_WIDTH (8 * sizeof(void *) - PAGE_ALIGN_UPPER_WIDTH)

typedef struct page_attrs_struct {
	bool    present;
	bool    writable;
	bool    user;
	bool    write_through;
	bool    cache_disabled;
	bool    accessed;
	bool    dirty;
	bool    global;
} page_attrs_t;

typedef struct page_table_entry_logical_struct {
	page_attrs_t attrs;
	page_t *physical_page_offset;
} page_table_entry_logical_t;

typedef struct PACKED page_table_entry_struct {
	unsigned char          present              : 1;
	unsigned char          writable             : 1;
	unsigned char          user                 : 1;
	unsigned char          write_through        : 1;
	unsigned char          cache_disabled       : 1;
	volatile unsigned char accessed             : 1;
	unsigned char          dirty                : 1;
	unsigned char          _rsv                 : 1;
	unsigned char          global               : 1;
	unsigned char          os_defined           : 3;
	size_t                 physical_page_offset : 20;
} page_table_entry_t;

typedef struct page_directory_entry_logical_struct {
	bool                present;
	bool                writable;
	bool                user;
	bool                write_through;
	bool                cache_disabled;
	bool                accessed;
	bool                size;
	bool                ignore;
	page_table_entry_t *page_table_base;
} page_directory_entry_logical_t;

typedef struct PACKED page_directory_entry_struct {
	unsigned char          present         : 1;
	unsigned char          writable        : 1;
	unsigned char          user            : 1;
	unsigned char          write_through   : 1;
	unsigned char          cache_disabled  : 1;
	volatile unsigned char accessed        : 1;
	unsigned char          _rsv            : 1;
	unsigned char          size            : 1;
	unsigned char          ignore          : 1;
	unsigned char          os_defined      : 3;
	size_t                 page_table_base : 20;
} page_directory_entry_t;

typedef page_directory_entry_t *paging_data_t;

static inline size_t page_get_aligned_upper_bits (void *aligned_ptr) {
#pragma GCC diagnostic ignored "-Wpointer-to-int-cast"
	return (size_t)aligned_ptr >> PAGE_ALIGN_LOWER_WIDTH;
#pragma GCC diagnostic pop
}

static inline void *page_get_aligned_ptr (size_t aligned_upper_bits) {
#pragma GCC diagnostic ignored "-Wint-to-pointer-cast"
	return (void *)(aligned_upper_bits << PAGE_ALIGN_LOWER_WIDTH);
#pragma GCC diagnostic pop
}

static inline page_table_entry_t page_table_entry_encode (
		page_table_entry_logical_t logical_entry
) {
	page_table_entry_t entry = {
		.present              = logical_entry.attrs.present        ? 0b1 : 0b0,
		.writable             = logical_entry.attrs.writable       ? 0b1 : 0b0,
		.user                 = logical_entry.attrs.user           ? 0b1 : 0b0,
		.write_through        = logical_entry.attrs.write_through  ? 0b1 : 0b0,
		.cache_disabled       = logical_entry.attrs.cache_disabled ? 0b1 : 0b0,
		.accessed             = logical_entry.attrs.accessed       ? 0b1 : 0b0,
		.dirty                = logical_entry.attrs.dirty          ? 0b1 : 0b0,
		._rsv                 = 0,
		.global               = logical_entry.attrs.global         ? 0b1 : 0b0,
		.os_defined           = 0,
		.physical_page_offset = page_get_aligned_upper_bits (
			logical_entry.physical_page_offset)
	};

	return entry;
}

static inline page_table_entry_logical_t page_table_entry_decode (
		page_table_entry_t entry
) {
	page_table_entry_logical_t logical_entry = {
		.attrs = {
			.present              = entry.present        ? true : false,
			.writable             = entry.writable       ? true : false,
			.user                 = entry.user           ? true : false,
			.write_through        = entry.write_through  ? true : false,
			.cache_disabled       = entry.cache_disabled ? true : false,
			.accessed             = entry.accessed       ? true : false,
			.dirty                = entry.dirty          ? true : false,
			.global               = entry.global         ? true : false
		},
		.physical_page_offset = page_get_aligned_ptr (
			entry.physical_page_offset)
	};

	return logical_entry;
}

static inline page_directory_entry_t page_directory_entry_encode (
		page_directory_entry_logical_t logical_entry
) {
	page_directory_entry_t entry = {
		.present         = logical_entry.present        ? 0b1 : 0b0,
		.writable        = logical_entry.writable       ? 0b1 : 0b0,
		.user            = logical_entry.user           ? 0b1 : 0b0,
		.write_through   = logical_entry.write_through  ? 0b1 : 0b0,
		.cache_disabled  = logical_entry.cache_disabled ? 0b1 : 0b0,
		.accessed        = logical_entry.accessed       ? 0b1 : 0b0,
		._rsv            = 0,
		.size            = logical_entry.size           ? 0b1 : 0b0,
		.ignore          = logical_entry.ignore         ? 0b1 : 0b0,
		.os_defined      = 0,
		.page_table_base = page_get_aligned_upper_bits (
				logical_entry.page_table_base)
	};

	return entry;
}

static inline page_directory_entry_logical_t page_directory_entry_decode (
		page_directory_entry_t entry
) {
	page_directory_entry_logical_t logical_entry = {
		.present         = entry.present        ? true : false,
		.writable        = entry.writable       ? true : false,
		.user            = entry.user           ? true : false,
		.write_through   = entry.write_through  ? true : false,
		.cache_disabled  = entry.cache_disabled ? true : false,
		.accessed        = entry.accessed       ? true : false,
		.size            = entry.size           ? true : false,
		.ignore          = entry.ignore         ? true : false,
		.page_table_base = page_get_aligned_ptr (
				entry.page_table_base)
	};

	return logical_entry;
}

void paging_init (paging_data_t *);
void paging_enable (paging_data_t *);
void paging_switch (paging_data_t *);
bool paging_table_present (page_t *, paging_data_t *);
void paging_set_map (page_t *, page_t *, paging_data_t *);
page_t *paging_get_map (page_t *, paging_data_t *);
void paging_set_attrs (page_t *, page_attrs_t, paging_data_t *);
page_attrs_t paging_get_attrs (page_t *, paging_data_t *);
page_attrs_t paging_compatable_attrs (page_attrs_t, page_attrs_t);

#endif

// vim: set ts=4 sw=4 noet syn=c:
