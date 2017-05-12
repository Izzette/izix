// kernel/include/mm/malloc.h

#ifndef IZIX_MALLOC_H
#define IZIX_MALLOC_H 1

#include <stddef.h>
#include <stdalign.h>

#ifdef _GCC_MAX_ALIGN_T
typedef max_align_t __malloc_max_align_t;
#else
typedef struct __malloc_max_align_struct {
	long long __max_align_ll __attribute__((aligned(alignof(long long))));
	long double __max_align_ld __attribute__((aligned(alignof(long double))));
} __malloc_max_align_t;
#endif

#define MALLOC_ALIGNMENT alignof(__malloc_max_align_t)

void *malloc (size_t)
	__attribute__((malloc));
void *realloc (void *, size_t)
	__attribute__((malloc));
void free (void *);
void *aligned_alloc (size_t, size_t)
	__attribute__((malloc))
	__attribute__((alloc_size(2)));

#endif

// vim: set ts=4 sw=4 noet syn=c:
