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

/* draw one cell given as signed coordinates; cells off the canvas are skipped */
static inline void blot_canvas_draw_cell_l(blot_canvas *can, long x, long y)
{
	if (x >= 0 && y >= 0 && x < (long)can->dim.cols && y < (long)can->dim.rows)
		blot_canvas_draw_point(can, (unsigned)x, (unsigned)y);
}

/* Draw a line between two points in canvas coordinates.
 *
 * - Each end is drawn in its nearest cell.
 * - Between them the walk takes one cell per step along the major axis
 *   (the one with more cells between the two end cells); on the minor
 *   axis it takes the cell nearest to the true segment at that major
 *   cell (rounded half towards the far end).
 * - A cell never moves more than one cell on the minor axis from the
 *   previous one and always stays within reach of the far end, so the
 *   line has no gaps.  This only matters for segments whose true slope is
 *   steeper than their end cells suggest; otherwise the cells are the
 *   nearest ones.
 * - The walk always starts at the end whose cell has the smaller x (the
 *   smaller y for a vertical line), so swapping the ends gives the same
 *   cells, and a line from a point to itself draws that point's cell.
 * - Cells off the canvas are skipped.
 *
 * A segment with a non-finite end is not drawn: lround() of NaN or inf is
 * unspecified (LONG_MIN on x86-64), and the walk would then take ~2^63
 * steps.  How NaN and inf should be shown is #6.  Ends beyond the range of
 * a long are not handled here either (#5): they need clipping on the
 * doubles, before the rounding.  The step and offset arithmetic below is
 * unsigned, so it is defined for any two longs the rounding returns. */
static inline void blot_canvas_draw_line(blot_canvas *can,
					double x0, double y0,
					double x1, double y1)
{
	if (!isfinite(x0) || !isfinite(y0) || !isfinite(x1) || !isfinite(y1))
		return;

	long cx0 = lround(x0), cy0 = lround(y0);
	long cx1 = lround(x1), cy1 = lround(y1);

	if (cx0 > cx1 || (cx0 == cx1 && cy0 > cy1)) {
		swap_t(long, cx0, cx1);
		swap_t(long, cy0, cy1);
		swap_t(double, x0, x1);
		swap_t(double, y0, y1);
	}

	bool y_down = cy1 < cy0;
	unsigned long dx = (unsigned long)cx1 - (unsigned long)cx0;
	unsigned long dy = y_down ? (unsigned long)cy0 - (unsigned long)cy1
				  : (unsigned long)cy1 - (unsigned long)cy0;

	/* n steps along the major axis; the minor axis moves m in total.
	 * Both go away from the start cell: x always up, y up or down. */
	bool x_major = dx >= dy;
	unsigned long n = x_major ? dx : dy;
	unsigned long m = x_major ? dy : dx;

	/* the true segment, as major -> minor, in the walk's direction */
	double a0 = x_major ? x0 : y0;           /* major start */
	double a1 = x_major ? x1 : y1;
	double b0 = x_major ? y0 : x0;           /* minor start */
	double b1 = x_major ? y1 : x1;
	double ca0 = (double)(x_major ? cx0 : cy0);
	double cb0 = (double)(x_major ? cy0 : cx0);
	double a_dir = (!x_major && y_down) ? -1 : 1;   /* major direction */
	double b_dir = (x_major && y_down) ? -1 : 1;    /* minor direction */

	unsigned long off = 0;  /* minor offset from the start cell, 0..m */

	for (unsigned long i = 0; ; i++) {
		if (i == n) {
			off = m;
		} else if (i > 0) {
			/* nearest minor cell to the true segment at this
			 * major cell; n >= 2 here, so a1 != a0 */
			double a = ca0 + a_dir * (double)i;
			double b = b0 + (a - a0) * (b1 - b0) / (a1 - a0);
			double d = (b - cb0) * b_dir;
			unsigned long want;
			if (!(d > 0))
				want = 0;
			else if (d >= (double)m)
				want = m;
			else
				want = (unsigned long)lround(d);

			/* no gaps: at most one cell from the previous one,
			 * and the far end still reachable in n - i steps */
			unsigned long lo = off ? off - 1 : 0;
			unsigned long hi = off < m ? off + 1 : m;
			if (m - lo > n - i)
				lo = m - (n - i);
			if (want < lo)
				want = lo;
			if (want > hi)
				want = hi;
			off = want;
		}

		unsigned long ox = x_major ? i : off;
		unsigned long oy = x_major ? off : i;

		/* within [cx0,cx1] and between cy0 and cy1, so they fit a long */
		long x = (long)((unsigned long)cx0 + ox);
		long y = y_down ? (long)((unsigned long)cy0 - oy)
				: (long)((unsigned long)cy0 + oy);

		blot_canvas_draw_cell_l(can, x, y);

		if (i == n)
			break;
	}
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

