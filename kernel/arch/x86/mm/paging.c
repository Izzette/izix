// kernel/arch/x86/mm/paging.c

#include <kprint/kprint.h>
#include <kpanic/kpanic.h>
#include <mm/freemem.h>
#include <mm/page.h>
#include <mm/paging.h>

page_directory_entry_t *page_directory;

#define PAGE_TABLE_LENGTH     1024
#define PAGE_DIRECTORY_LENGTH 1024

static inline page_table_entry_t *paging_create_table (page_t *offset) {
	size_t i;
	freemem_region_t page_table_region;
	page_table_entry_t *page_table;

	page_table_region = freemem_suggest (
		PAGE_TABLE_LENGTH * sizeof(page_table_entry_t), PAGE_SIZE, 0);
	if (!page_table_region.length) {
		kputs ("mm/paging: Failed to allocate a page table!\n");
		kpanic ();
	}

	freemem_remove_region (page_table_region);
	page_table = page_table_region.p;

	for (i = 0; PAGE_TABLE_LENGTH > i; ++i) {
		page_table_entry_logical_t logical_table_entry = {
			.present = true,
			.writable = true,
			.user = false,
			.write_through = false,
			.cache_disabled = false,
			.accessed = false,
			.dirty = false,
			.global = false,
			.physical_page_offset = offset + i
		};

		page_table[i] = page_table_entry_encode (logical_table_entry);
	}

	return page_table;
}

static inline page_directory_entry_t *paging_create_directory () {
	size_t i;
	freemem_region_t page_directory_region;
	page_directory_entry_t *page_directory;
	page_t *offset;

	page_directory_region = freemem_suggest (
		PAGE_DIRECTORY_LENGTH * sizeof(page_directory_entry_t), PAGE_SIZE, 0);
	if (!page_directory_region.length) {
		kputs ("mm/paging: Failed to allocate the page directory!\n");
		kpanic ();
	}

	freemem_remove_region (page_directory_region);
	page_directory = page_directory_region.p;

	offset = 0;

	for (i = 0, offset = 0; PAGE_TABLE_LENGTH > i; ++i, offset += PAGE_TABLE_LENGTH) {
		page_table_entry_t *page_table = paging_create_table (offset);

		page_directory_entry_logical_t logical_directory_entry = {
			.present = true,
			.writable = true,
			.user = false,
			.write_through = false,
			.cache_disabled = false,
			.accessed = false,
			.size = false,
			.ignore = false,
			.page_table_base = page_table
		};

		page_directory[i] = page_directory_entry_encode (logical_directory_entry);
	}

	return page_directory;
}

void paging_init () {
	page_directory = paging_create_directory ();
}

void paging_load () {
	asm volatile (
		"mov		%0,					%%cr3;\n"
		"mov		%%cr0,				%%eax;\n"
		"or			$0x80000000,		%%eax;\n"
		"mov		%%eax,				%%cr0;\n"
		:
		:"r"(page_directory)
		:"memory", "eax");

	kputs ("mm/paging: Successfully enabled paging.\n");
}

// vim: set ts=4 sw=4 noet syn=c:
