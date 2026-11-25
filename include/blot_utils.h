/* blot: various utility macros */
/* vim: set noet sw=8 ts=8 tw=120: */
#pragma once

#include <glib.h>

#include "blot_compiler.h"

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
#undef ONCE
#define ONCE(block) ({                                          \
	static bool done = false;                               \
	if (unlikely (__sync_bool_compare_and_swap(&done,false,true))) {   \
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

#if !__has_builtin(__builtin_constant_p)
#define __builtin_constant_p(x) (0)
#endif

BLOT_EXTERN_C_START

BLOT_API unsigned blot_env_to_uint(const char *name, unsigned dflt);
BLOT_API int blot_format_number(char *p, unsigned room, double d_val);
/* blot_format_number(), for an axis label: a text within tol of d_val
 * (half a cell of the axis) where one fits.  Fixed point (%.3f, %.2f,
 * %.1f) when it is within tol, else the fewest significant digits that
 * are (0.0012, 1.4e-09); large values keep blot_format_number()'s most
 * precise text when it is within tol.  When nothing that fits is within
 * tol, the most precise text that fits.  Never a negative zero.  A tol
 * that is inf, NaN or negative means no tolerance: the same text as
 * blot_format_number().  Returns the length, or -1 when nothing fits;
 * nothing is written at or past p[room]. */
BLOT_API int blot_format_number_within(char *p, unsigned room, double d_val, double tol);

BLOT_EXTERN_C_END
