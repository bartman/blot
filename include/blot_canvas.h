/* blot: a canvas is a render of a single layer w/o colour coding */
/* vim: set noet sw=8 ts=8 tw=120: */
#pragma once

#include <glib.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>

#include "blot_compiler.h"
#include "blot_types.h"
#include "blot_utils.h"
#include "blot_braille.h"

typedef struct blot_canvas {
	blot_dimensions dim;
	blot_render_flags flags;
	blot_color color;

	union {
		struct {
			// only when braille is on
			guint8 *masks;          // braille_masks or braille_upsidedown_masks
		} braille;
		struct {
			// only when braille is off
			wchar_t plot_char;
		} no_braille;
	};

	gsize bitmap_size;              // number of bits available
	gsize bitmap_bytes;             // byte size of bitmap array
	guint8 bitmap[] __aligned64;    // must be at end of structure
} blot_canvas;

#define BLOT_CANVAS_BITMAP_CELL_SIZE 8

/* create/delete */

BLOT_EXTERN_C_START

BLOT_API blot_canvas * blot_canvas_new(unsigned cols, unsigned rows,
				     blot_render_flags flags, blot_color color,
				     GError **);
BLOT_API void blot_canvas_delete(blot_canvas *fig);

BLOT_EXTERN_C_END

/* render */

static inline bool blot_canvas_set(blot_canvas *can, unsigned col, unsigned row, bool val)
{
	g_assert_nonnull(can);

	if (unlikely (col >= can->dim.cols))
		return false;

	if (unlikely (row >= can->dim.rows))
		return false;

	if (can->flags & BLOT_RENDER_BRAILLE) {

		unsigned byte = ((row/4) * (can->dim.cols/2)) + (col/2);
		unsigned bit = ((row%4)*2) + (col%2);
		guint8   mask = can->braille.masks[bit];

		g_assert_cmpuint(byte, <, can->bitmap_bytes);

#if 0
		wchar_t wch = BRAILLE_GLYPH_BASE + mask;
		g_print("%s[%u,%u] = %u, bitmap[%u] & 0x%02x (%u) -> %lc\n",
			__func__, col, row, val, byte, mask, bit, wch);
#endif

		if (val)
			can->bitmap[byte] |= mask;
		else
			can->bitmap[byte] &= ~(mask);

	} else {
		unsigned idx = (row * can->dim.cols) + col;

		g_assert_cmpuint(idx, <, can->bitmap_size);

		unsigned byte = idx / BLOT_CANVAS_BITMAP_CELL_SIZE;
		unsigned bit  = idx % BLOT_CANVAS_BITMAP_CELL_SIZE;
		guint8   mask = 1<<bit;

		g_assert_cmpuint(byte, <, can->bitmap_bytes);

#if 0
		g_print("%s[%u,%u] = %u, idx=%u, bitmap[%u] & 0x%02x (%u)\n",
			__func__, col, row, val, idx, byte, mask, bit);
#endif

		if (val)
			can->bitmap[byte] |= mask;
		else
			can->bitmap[byte] &= ~(mask);
	}

	return true;
}

static inline bool blot_canvas_get(const blot_canvas *can, unsigned col, unsigned row)
{
	g_assert_nonnull(can);

	if (col >= can->dim.cols)
		return 0;

	if (row >= can->dim.rows)
		return 0;

	if (can->flags & BLOT_RENDER_BRAILLE) {

		unsigned byte = ((row/4) * (can->dim.cols/2)) + (col/2);
		unsigned bit = ((row%4)*2) + (col%2);
		guint8   mask = can->braille.masks[bit];

		g_assert_cmpuint(byte, <, can->bitmap_bytes);

		bool val = !!(can->bitmap[byte] & (mask));

#if 0
		if (val) {
			wchar_t wch = BRAILLE_GLYPH_BASE + mask;
			g_print("%s[%u,%u] = %u, bitmap[%u] & 0x%02x (%u) -> %lc\n",
				__func__, col, row, val, byte, mask, bit, wch);
		}
#endif

		return val;

	} else {
		unsigned idx = (row * can->dim.cols) + col;

		g_assert_cmpuint(idx, <, can->bitmap_size);

		unsigned byte = idx / BLOT_CANVAS_BITMAP_CELL_SIZE;
		unsigned bit  = idx % BLOT_CANVAS_BITMAP_CELL_SIZE;
		unsigned mask = 1<<bit;

		g_assert_cmpuint(byte, <, can->bitmap_bytes);

		bool val = !!(can->bitmap[byte] & (mask));

#if 0
		if (val)
			g_print("%s[%u,%u] = %u, idx=%u, bitmap[%u] & 0x%02x (%u)\n",
				__func__, col, row, val, idx, byte, mask, bit);
#endif

		return val;
	}
}

static inline wchar_t blot_canvas_get_cell(const blot_canvas *can,
					    unsigned cell_col, unsigned cell_row)
{
	g_assert_nonnull(can);

	if (can->flags & BLOT_RENDER_BRAILLE) {
		unsigned max_cell_cols = can->dim.cols/BRAILLE_GLYPH_COLS;
		unsigned max_cell_rows = can->dim.rows/BRAILLE_GLYPH_ROWS;

		if (cell_col >= max_cell_cols)
			return 0;

		if (cell_row >= max_cell_rows)
			return 0;

		unsigned idx = (cell_row * max_cell_cols) + cell_col;

		g_assert_cmpuint(idx, <, can->bitmap_bytes);

		unsigned char val = can->bitmap[idx];
		if (likely (!val))
			return 0;

		wchar_t wch = BRAILLE_GLYPH_BASE + can->bitmap[idx];
		return wch;

	} else {
		bool val = blot_canvas_get(can, cell_col, cell_row);

		return val ? can->no_braille.plot_char : 0;
	}
}

/* draw functions */

static inline void blot_canvas_draw_point(blot_canvas *can, unsigned x, unsigned y)
{
	blot_canvas_set(can, x, y, 1);
}

/* Draw a line between two cells, both included.
 *
 * Integer stepping: the major axis advances one cell per step and the
 * minor axis is the ideal line rounded to the nearest cell, so there are
 * no gaps, both ends are drawn, and a line from a cell to itself draws
 * that cell.  The walk always starts at the end with the smaller x (the
 * smaller y for a vertical line), so swapping the ends gives the same
 * cells.  Cells outside the canvas are skipped.
 *
 * The arithmetic is unsigned so that it is defined for any pair of longs;
 * the caller decides what to do with ends far outside the canvas. */
static inline void blot_canvas_draw_line_cells(blot_canvas *can,
					       long x0, long y0,
					       long x1, long y1)
{
	if (x0 > x1 || (x0 == x1 && y0 > y1)) {
		swap_t(long, x0, x1);
		swap_t(long, y0, y1);
	}

	bool y_down = y1 < y0;
	unsigned long dx = (unsigned long)x1 - (unsigned long)x0;
	unsigned long dy = y_down ? (unsigned long)y0 - (unsigned long)y1
				  : (unsigned long)y1 - (unsigned long)y0;

	/* n steps along the major axis; the minor axis moves m in total */
	bool x_major = dx >= dy;
	unsigned long n = x_major ? dx : dy;
	unsigned long m = x_major ? dy : dx;

	/* at step i the minor offset is i*m/n = q + t/n with 0 <= t < n */
	unsigned long q = 0, t = 0;

	for (unsigned long i = 0; ; i++) {
		/* round to nearest: up when t/n >= 1/2 */
		unsigned long minor = q + (t && t >= n - t);
		unsigned long major = i;

		unsigned long ox = x_major ? major : minor;
		unsigned long oy = x_major ? minor : major;

		/* both are within [x0,x1] and [y0,y1], so they fit a long */
		long x = (long)((unsigned long)x0 + ox);
		long y = y_down ? (long)((unsigned long)y0 - oy)
				: (long)((unsigned long)y0 + oy);

		if (x >= 0 && y >= 0 && x < (long)can->dim.cols && y < (long)can->dim.rows)
			blot_canvas_draw_point(can, (unsigned)x, (unsigned)y);

		if (i == n)
			break;

		/* advance i*m/n by m/n, without overflow */
		if (t >= n - m) {
			t -= n - m;
			q++;
		} else {
			t += m;
		}
	}
}

/* Draw a line between two points in canvas coordinates.  Each end is
 * rounded to its nearest cell and the cells between are stepped as
 * integers (see blot_canvas_draw_line_cells). */
static inline void blot_canvas_draw_line(blot_canvas *can,
					double x0, double y0,
					double x1, double y1)
{
	blot_canvas_draw_line_cells(can, lround(x0), lround(y0),
				    lround(x1), lround(y1));
}

static inline void blot_canvas_draw_rect(blot_canvas *can,
					double x0, double y0,
					double x1, double y1)
{
	blot_canvas_draw_line(can, x0, y0, x0, y1);
	blot_canvas_draw_line(can, x1, y0, x1, y1);
	blot_canvas_draw_line(can, x1, y1, x0, y1);
	blot_canvas_draw_line(can, x0, y0, x1, y0);
}

static inline void blot_canvas_fill_rect(blot_canvas *can,
					unsigned x0, unsigned y0,
					unsigned x1, unsigned y1)
{
	if (x0 > x1)
		swap_t(unsigned, x0, x1);

	if (y0 > y1)
		swap_t(unsigned, y0, y1);

	for (unsigned y=y0; y<=y1; y++) {
		for (unsigned x=x0; x<=x1; x++) {
			blot_canvas_draw_point(can, x, y);
		}
	}
}

