// kernel/arch/x86/include/mm/page.h

#ifndef IZIX_PAGE_H
#define IZIX_PAGE_H 1

#define PAGE_SIZE 4096

typedef char page_t[PAGE_SIZE] __attribute__((aligned (PAGE_SIZE)));

#endif

// vim: set ts=4 sw=4 noet syn=c:
