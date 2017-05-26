// libk/include/attributes.h

#ifndef IZIX_LIBK_ATTRIBUTES_H
#define IZIX_LIBK_ATTRIBUTES_H 1

#ifdef ARCH_X86
#define FASTCALL __attribute__((fastcall))
#define THISCALL __attribute__((thiscall))
#else
#define FASTCALL
#define THISCALL
#endif

#define HOT __attribute__((hot))
#define COLD __attribute__((cold))

#define SMALL __attribute__((optimize("Os")))
#define FAST __attribute__((optimize("O3")))
#define DEBUG __attribute__((optimize("O0")))

#define PACKED __attribute__((packed))

#define MAY_ALIAS __attribute__((__may_alias__))

#define FORCE_ALIGN_ARG_POINTER __attribute__((force_align_arg_pointer))

#define CONSTRUCTOR __attribute__((constructor))

#define NORETURN __attribute__((noreturn))

#define MALLOC __attribute__((malloc))

#define ALIGNED(n) __attribute__((aligned(n)))

#define FORMAT(x, a, b) __attribute__((format(x, a, b)))

#endif

// vim: set ts=4 sw=4 noet syn=c:
