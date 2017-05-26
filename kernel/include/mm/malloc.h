// kernel/include/mm/malloc.h

#ifndef IZIX_MALLOC_H
#define IZIX_MALLOC_H 1

#include <stddef.h>
#include <stdalign.h>

#include <attributes.h>

#ifdef _GCC_MAX_ALIGN_T
typedef max_align_t __malloc_max_align_t;
#else
typedef struct {
	long long __max_align_ll ALIGNED(alignof(long long));
	long double __max_align_ld ALIGNED(alignof(long double));
} __malloc_max_align_t;
#endif

// 8-or-so really isn't enough to fit many data structures into, 64-or-so is much better.
#define MALLOC_ALIGNMENT (alignof(__malloc_max_align_t) * alignof(__malloc_max_align_t))

void *malloc (size_t)
	MALLOC;
void *realloc (void *, size_t)
	MALLOC;
void free (void *);

#endif

// vim: set ts=4 sw=4 noet syn=c:
