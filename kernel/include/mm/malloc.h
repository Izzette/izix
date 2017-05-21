// kernel/include/mm/malloc.h

#ifndef IZIX_MALLOC_H
#define IZIX_MALLOC_H 1

#include <stddef.h>
#include <stdalign.h>

#ifdef _GCC_MAX_ALIGN_T
typedef max_align_t __malloc_max_align_t;
#else
typedef struct {
	long long __max_align_ll __attribute__((__aligned__(__alignof__(long long))));
	long double __max_align_ld __attribute__((__aligned__(__alignof__(long double))));
} __malloc_max_align_t;
#endif

// 8-or-so really isn't enough to fit many data structures into, 64-or-so is much better.
#define MALLOC_ALIGNMENT (alignof(__malloc_max_align_t) * alignof(__malloc_max_align_t))

void *malloc (size_t)
	__attribute__((malloc));
void *realloc (void *, size_t)
	__attribute__((malloc));
void free (void *);

#endif

// vim: set ts=4 sw=4 noet syn=c:
