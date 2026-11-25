/* blot: color handling */
/* vim: set noet sw=8 ts=8 tw=120: */
#pragma once

#include <glib.h>
#include <stdbool.h>

#include "blot_compiler.h"
#include "blot_utils.h"

#define BLACK     0
#define RED       1
#define GREEN     2
#define YELLOW    3
#define BLUE      4
#define MAGENTA   5
#define CYAN      6
#define WHITE     7

#define DIM_BLACK   BLACK
#define DIM_RED     RED
#define DIM_GREEN   GREEN
#define DIM_YELLOW  YELLOW
#define DIM_BLUE    BLUE
#define DIM_MAGENTA MAGENTA
#define DIM_CYAN    CYAN
#define DIM_WHITE   WHITE

#define BRIGHT_BLACK   8
#define BRIGHT_RED     9
#define BRIGHT_GREEN   10
#define BRIGHT_YELLOW  11
#define BRIGHT_BLUE    12
#define BRIGHT_MAGENTA 13
#define BRIGHT_CYAN    14
#define BRIGHT_WHITE   15

#define ESC "\033["
#define COL_BUF_LEN 16

#define COL_FG_PREFIX   ESC "38;5;"
#define COL_BG_PREFIX   ESC "48;5;"
#define COL_SUFFIX      "m"

#define COL_FG_FMT   COL_FG_PREFIX "%d" COL_SUFFIX
#define COL_BG_FMT   COL_BG_PREFIX "%d" COL_SUFFIX

#define COL_RESET    ESC "0" COL_SUFFIX
#define CLR_SCR      ESC "2J" ESC "H"

BLOT_EXTERN_C_START

extern BLOT_API bool have_color_support;

BLOT_EXTERN_C_END

/* constant expression support */
#define mkcol_constant(prefix,col) \
	(prefix __stringify(col) COL_SUFFIX)

/* the escape sequence for a runtime colour (col & 0xFF): a pointer into a
 * static table, so it needs no buffer and stays valid; fg() and bg() used
 * to alloca() one per call, which overflowed the stack in a loop (#11) */
BLOT_EXTERN_C_START

BLOT_API const char *blot_color_fg(int col);
BLOT_API const char *blot_color_bg(int col);

BLOT_EXTERN_C_END

/* macros to build char colors */
#define fg(col) ( \
	have_color_support ? ( \
		__builtin_constant_p(col) ? ( \
			mkcol_constant(COL_FG_PREFIX, col) \
		) : blot_color_fg(col) \
	) : "" \
)
#define bg(col) ( \
	have_color_support ? ( \
		__builtin_constant_p(col) ? ( \
			mkcol_constant(COL_BG_PREFIX, col) \
		) : blot_color_bg(col) \
	) : "" \
)
