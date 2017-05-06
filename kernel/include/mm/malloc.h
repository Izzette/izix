// kernel/include/mm/malloc.h

#ifndef IZIX_MALLOC_H
#define IZIX_MALLOC_H 1

#include <stdint.h>

#define MALLOC_ALIGNMENT sizeof(intmax_t)

void *malloc (size_t)
	__attribute__((malloc));
void free (void *);

#endif

// vim: set ts=4 sw=4 noet syn=c:
