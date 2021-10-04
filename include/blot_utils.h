/* blot: various utility macros */
/* vim: set noet sw=8 ts=8 tw=80: */
#pragma once

#include <glib.h>

#ifndef PAGE_SIZE
#define PAGE_SIZE 4096
#endif

/* anything between -4095 and 4095 is almost guaranteed to not be a pointer */
#define __pointer_suspect(ptr) (unlikely( \
	 (uintptr_t)(ptr) < PAGE_SIZE || \
	 ~(uintptr_t)(ptr) < PAGE_SIZE ))

#ifndef offsetof
#define offsetof(type, member)	((size_t)&((type *)0)->member)
#endif

#ifndef container_of
#define container_of(ptr, type, member) ({                      \
        const typeof( ((type *)0)->member ) *__mptr = (ptr);    \
        (type *)( (char *)__mptr - offsetof(type,member) );})
#endif

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x) (sizeof(x) / sizeof(*(x)))
#endif

/* macro that will perform a block only once */
#define ONCE(block) ({                                          \
	static bool done = false;                               \
	if (__sync_bool_compare_and_swap(&done,false,true)) {   \
		block;                                          \
	} })

#define min_t(type,a,b) ({                                      \
	type _a = (a), _b = (b);                                \
	(_a < _b) ? _a : _b;                                    \
	})

#define max_t(type,a,b) ({                                      \
	type _a = (a), _b = (b);                                \
	(_a > _b) ? _a : _b;                                    \
	})

#define swap_t(type,a,b) ({                                     \
	type _t = (a);                                          \
	(a) = (b);                                              \
	(b) = _t;                                               \
	})

#define abs_t(type,a) ({                                        \
	type _a = (a);                                          \
	(_a > 0) ? _a : -_a;                                    \
	})

#define ALIGN_SIZE_UP(size, bytes) (((size_t)(size) + (bytes) - 1) & ~(bytes - 1))
#define ALIGN_PTR_UP(ptr, bytes) ((void *)(((uintptr_t)(ptr) + (bytes) - 1) & ~(bytes - 1)))

#define __stringify_x(x) #x
#define __stringify(x) __stringify_x(x)

#undef likely
#undef unlikely
#ifdef __GNUC__
#define likely(x)       __builtin_expect(!!(x),1)
#define unlikely(x)     __builtin_expect(!!(x),0)
#else
#define likely(x)       (x)
#define unlikely(x)     (x)
#endif

#define __aligned64 __attribute__((aligned(64)))

#if __SIZEOF_WCHAR_T__ != 4
#error expecting sizeof(wchar_t) == 4
#endif
#define _L(x) L ## x
#define L(x) _L(x)

