/* blot: color handling */
/* vim: set noet sw=8 ts=8 tw=80: */
#pragma once

#include <glib.h>
#include <stdbool.h>

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
#define COL_BUF_LEN 16
#define COL_FG_FMT "\033[38;5;%dm"
#define COL_BG_FMT "\033[48;5;%dm"

#define COL_RESET  "\033[0m"
//#define CLR_SCR    "\033c"    // this one also drops scrollback in tmux
#define CLR_SCR    "\033[2J\033[H"

extern bool have_color_support;

/* macros to build char colors */

static inline const char *mkcol(char *buf, const char *fmt, unsigned col)
{
	g_snprintf(buf, COL_BUF_LEN, fmt, col & 0xFF);
	return buf;
}
#define fg(col) ({ \
	char _buf[COL_BUF_LEN] = {0,}; \
	const char *res = ""; \
	if (have_color_support) \
		res = mkcol(_buf, COL_FG_FMT, col); \
	res; \
})
#define bg(col) ({ \
	char _buf[COL_BUF_LEN] = {0,}; \
	const char *res = ""; \
	if (have_color_support) \
		res = mkcol(_buf, COL_BG_FMT, col); \
	res; \
})

