/* blot: color handling */
/* vim: set noet sw=8 ts=8 tw=120: */
#pragma once

#include <glib.h>
#include <stdbool.h>
#include "blot_utils.h"

enum {
	BLACK,
	RED,
	GREEN,
	YELLOW,
	BLUE,
	MAGENTA,
	CYAN,
	WHITE,

	DIM_BLACK   = BLACK,
	DIM_RED     = RED,
	DIM_GREEN   = GREEN,
	DIM_YELLOW  = YELLOW,
	DIM_BLUE    = BLUE,
	DIM_MAGENTA = MAGENTA,
	DIM_CYAN    = CYAN,
	DIM_WHITE   = WHITE,

	BRIGHT_BLACK   = 8 + BLACK,
	BRIGHT_RED     = 8 + RED,
	BRIGHT_GREEN   = 8 + GREEN,
	BRIGHT_YELLOW  = 8 + YELLOW,
	BRIGHT_BLUE    = 8 + BLUE,
	BRIGHT_MAGENTA = 8 + MAGENTA,
	BRIGHT_CYAN    = 8 + CYAN,
	BRIGHT_WHITE   = 8 + WHITE,
};
#define ESC "\033["
#define COL_BUF_LEN 16

#define COL_FG_PREFIX   ESC "38;5;"
#define COL_BG_PREFIX   ESC "48;5;"
#define COL_SUFFIX      "m"

#define COL_FG_FMT   COL_FG_PREFIX "%d" COL_SUFFIX
#define COL_BG_FMT   COL_BG_PREFIX "%d" COL_SUFFIX

#define COL_RESET    ESC "0" COL_SUFFIX
#define CLR_SCR      ESC "2J" ESC "H"

extern bool have_color_support;

/* runtime support */

static inline char *mkcol(char *buf, size_t size, const char *fmt, int col)
{
	g_snprintf(buf, COL_BUF_LEN, fmt, col & 0xFF);
	return buf;
}

/* macros to build char colors */

#define colbuf(name) \
	char name[COL_BUF_LEN] = {}

#define fg(col) ( \
	have_color_support ? ({ \
		 char *buf = alloca(COL_BUF_LEN); \
		 mkcol(buf, COL_BUF_LEN, COL_FG_FMT, col); \
		 }) : "" )
#define bg(col) ( \
	have_color_support ? ({ \
		 char *buf = alloca(COL_BUF_LEN); \
		 mkcol(buf, COL_BUF_LEN, COL_BG_FMT, col); \
		 }) : "" )

