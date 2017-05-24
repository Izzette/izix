// kernel/arch/x86/mm/paging.c

#include <kprint/kprint.h>
#include <kpanic/kpanic.h>
#include <mm/freemem.h>
#include <mm/page.h>
#include <mm/paging.h>

#define PAGE_TABLE_LENGTH     1024
#define PAGE_DIRECTORY_LENGTH 1024

static page_table_entry_t *paging_create_table () {
	size_t i;
	freemem_region_t page_table_region;
	page_table_entry_t *page_table;

	page_table_region = freemem_alloc (
		PAGE_TABLE_LENGTH * sizeof(page_table_entry_t), PAGE_SIZE, 0);
	if (!page_table_region.length) {
		kputs ("mm/paging: Failed to allocate a page table!\n");
		kpanic ();
	}

	page_table = page_table_region.p;

	for (i = 0; PAGE_TABLE_LENGTH > i; ++i) {
		page_attrs_t attrs = {
			.present = false,
			.writable = false,
			.user = false,
			.write_through = false,
			.cache_disabled = false,
			.accessed = false,
			.dirty = false,
			.global = false
		};

		page_table_entry_logical_t logical_table_entry = {
			.attrs = attrs,
			.physical_page_offset = NULL
		};

		page_table[i] = page_table_entry_encode (logical_table_entry);
	}

	return page_table;
}

static page_directory_entry_t *paging_create_directory () {
	size_t i;
	freemem_region_t page_directory_region;
	page_directory_entry_t *page_directory;

	page_directory_region = freemem_alloc (
		PAGE_DIRECTORY_LENGTH * sizeof(page_directory_entry_t), PAGE_SIZE, 0);
	if (!page_directory_region.length) {
		kputs ("mm/paging: Failed to allocate the page directory!\n");
		kpanic ();
	}

	page_directory = page_directory_region.p;

	for (i = 0; PAGE_TABLE_LENGTH > i; ++i) {
		page_directory_entry_logical_t logical_directory_entry = {
			.present = false,
			.writable = false,
			.user = false,
			.write_through = false,
			.cache_disabled = false,
			.accessed = false,
			.size = false,
			.ignore = false,
			.page_table_base = NULL
		};

		page_directory[i] = page_directory_entry_encode (logical_directory_entry);
	}

	return page_directory;
}

static void paging_atomic_write_long (
		uint32_t *dest,
		uint32_t *src
) {
	asm volatile (
		"		movl	(%0),				%1;\n"
		"		lock;\n"
		"		xchg	%1,					(%2);\n"
		:
		:"r"(src), "r"(NULL), "r"(dest)
		:"memory");
}

static void paging_atomic_write_directory (
		page_directory_entry_t *dest,
		page_directory_entry_t src
) {
	paging_atomic_write_long ((uint32_t *)dest, (uint32_t *)&src);
}

static void paging_atomic_write_table (
		page_table_entry_t *dest,
		page_table_entry_t src
) {
	paging_atomic_write_long ((uint32_t *)dest, (uint32_t *)&src);
}

static size_t paging_directory_index (page_t *virtual) {
	return (size_t)virtual / PAGE_SIZE / PAGE_TABLE_LENGTH;
}

static size_t paging_table_index (page_t *virtual) {
	return (size_t)virtual / PAGE_SIZE % PAGE_TABLE_LENGTH;
}

static page_table_entry_t *paging_get_entries (
		page_t *virtual,
		page_directory_entry_t *directory,
		page_directory_entry_t **directory_entry_ptr
) {
	size_t directory_index = paging_directory_index (virtual);
	size_t table_index     = paging_table_index (virtual);

	*directory_entry_ptr =
		directory + directory_index;
	page_directory_entry_logical_t directory_entry_logical =
		page_directory_entry_decode (**directory_entry_ptr);

	if (!directory_entry_logical.present)
		return NULL;

	return directory_entry_logical.page_table_base + table_index;
}

void paging_init (paging_data_t *page_directory_ptr) {
	*page_directory_ptr = paging_create_directory ();
}

void paging_enable (paging_data_t *page_directory_ptr) {
	paging_switch (page_directory_ptr);

	asm volatile (
		"mov		%%cr0,				%%eax;\n"
		"or			$0x80000000,		%%eax;\n"
		"mov		%%eax,				%%cr0;\n"
		:
		:
		:"memory", "eax");

	kputs ("mm/paging: Paging enabled successfully.\n");
}

void paging_switch (paging_data_t *page_directory_ptr) {
	asm volatile (
		"mov		%0,					%%cr3;\n"
		:
		:"r"(*page_directory_ptr)
		:"memory");
}

bool paging_table_present (page_t *virtual, paging_data_t *page_directory_ptr) {
	page_directory_entry_t *directory_entry;
	paging_get_entries (virtual, *page_directory_ptr, &directory_entry);

	const page_directory_entry_logical_t directory_entry_logical =
		page_directory_entry_decode (*directory_entry);

	return directory_entry_logical.present;
}

void paging_set_map (
		page_t *virtual,
		page_t *physical,
		paging_data_t *page_directory_ptr
) {
	bool write_directory = false;

	page_directory_entry_t *directory_entry;
	page_table_entry_t *table_entry =
		paging_get_entries (virtual, *page_directory_ptr, &directory_entry);

	page_directory_entry_logical_t directory_entry_logical =
		page_directory_entry_decode (*directory_entry);

	if (!table_entry) {
		write_directory = true;
		directory_entry_logical.page_table_base = paging_create_table ();
		table_entry =
			directory_entry_logical.page_table_base + paging_table_index (virtual);

		directory_entry_logical.present = true;
		// Pages can be marked as not writable or not user accessible in the page table.
		directory_entry_logical.writable = true;
		directory_entry_logical.user = true;
	}

	page_table_entry_logical_t table_entry_logical =
		page_table_entry_decode (*table_entry);

	if (physical != table_entry_logical.physical_page_offset) {
		table_entry_logical.physical_page_offset = physical;

		page_table_entry_t table_entry_tmp =
			page_table_entry_encode (table_entry_logical);
		paging_atomic_write_table (table_entry, table_entry_tmp);
	}

	if (write_directory) {
		page_directory_entry_t directory_entry_tmp =
			page_directory_entry_encode (directory_entry_logical);
		paging_atomic_write_directory (directory_entry, directory_entry_tmp);
	}
}

page_t *paging_get_map (page_t *virtual, paging_data_t *page_directory_ptr) {
	page_directory_entry_t *directory_entry;
	page_table_entry_t *table_entry =
		paging_get_entries (virtual, *page_directory_ptr, &directory_entry);

	if (!table_entry) {
		kputs ("mm/paging: Failed to get mapping of page in non-present table!\n");
		kpanic ();
	}

	page_table_entry_logical_t table_entry_logical =
		page_table_entry_decode (*table_entry);

	return table_entry_logical.physical_page_offset;
}

void paging_set_attrs (
		page_t *virtual,
		page_attrs_t attrs,
		paging_data_t *page_directory_ptr
) {
	page_directory_entry_t *directory_entry;
	page_table_entry_t *table_entry =
		paging_get_entries (virtual, *page_directory_ptr, &directory_entry);

	if (!table_entry) {
		kputs ("mm/paging: Failed to set attributes of page in non-present table!\n");
		kpanic ();
	}

	page_table_entry_logical_t table_entry_logical =
		page_table_entry_decode (*table_entry);

	table_entry_logical.attrs = attrs;

	page_table_entry_t table_entry_tmp = page_table_entry_encode (table_entry_logical);
	paging_atomic_write_table (table_entry, table_entry_tmp);
}

page_attrs_t paging_get_attrs (
		page_t *virtual,
		paging_data_t *page_directory_ptr
) {
	page_directory_entry_t *directory_entry;
	page_table_entry_t *table_entry =
		paging_get_entries (virtual, *page_directory_ptr, &directory_entry);

	if (!table_entry) {
		kputs ("mm/paging: Failed to set attributes of page in non-present table!\n");
		kpanic ();
	}

	page_table_entry_logical_t table_entry_logical =
		page_table_entry_decode (*table_entry);

	return table_entry_logical.attrs;
}

page_attrs_t paging_compatable_attrs (page_attrs_t a, page_attrs_t b) {
	page_attrs_t compatable = {
		.present        = a.present        || b.present,
		.writable       = a.writable       || b.writable,
		.user           = a.user           || b.user,
		.write_through  = a.write_through  || b.write_through,
		.cache_disabled = a.cache_disabled || b.cache_disabled,
		.accessed       = a.accessed       || b.accessed,
		.dirty          = a.dirty          || b.dirty,
		.global         = a.global         || b.global
	};

	return compatable;
}

// vim: set ts=4 sw=4 noet syn=c:
