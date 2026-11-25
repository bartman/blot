/* blot: a canvas is a render of a single layer w/o colour coding */
/* vim: set noet sw=8 ts=8 tw=120: */
#pragma once

#include <glib.h>
#include <float.h>
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

/* Ends farther than this from 0, in cells, are first clipped to it (#5):
 * lround() is defined up to the range of a long, and the doubles below
 * keep a fraction of a cell up to here (2^40). */
#define BLOT_CANVAS_LINE_FAR 1099511627776.0

/* a diagonal line (see blot_canvas_draw_line) is walked from its start
 * when the canvas is within this many steps of it */
#define BLOT_CANVAS_LINE_WALK (1ul << 20)

/* q on the segment (p0,q0)-(p1,q1) where p == P, for P between p0 and p1
 * (p0 != p1), interpolated from the end nearer to P; halved so that the
 * differences can't overflow, and no product is larger than an end */
static inline double blot_canvas_line_at(double p0, double q0,
					 double p1, double q1, double P)
{
	if (fabs(P - p0) > fabs(P - p1)) {
		swap_t(double, p0, p1);
		swap_t(double, q0, q1);
	}
	double t = (P/2 - p0/2) / (p1/2 - p0/2);   /* 0..1/2 */
	return q0 * (1 - t) + q1 * t;
}

/* Clip a segment to the square +-BLOT_CANVAS_LINE_FAR, on the doubles
 * (Cohen-Sutherland).  The clipped coordinate is set to the edge itself,
 * and the other one interpolated from the end nearer to the edge.
 * Returns false when the segment misses the square. */
static inline bool blot_canvas_line_clip_far(double *x0, double *y0,
					     double *x1, double *y1)
{
	const double F = BLOT_CANVAS_LINE_FAR;

	for (int k = 0; k < 8; k++) {
		unsigned c0 = (*x0 < -F) | (*x0 > F) << 1 | (*y0 < -F) << 2 | (*y0 > F) << 3;
		unsigned c1 = (*x1 < -F) | (*x1 > F) << 1 | (*y1 < -F) << 2 | (*y1 > F) << 3;
		if (!(c0 | c1))
			return true;
		if (c0 & c1)
			return false;

		double *x = c0 ? x0 : x1, *y = c0 ? y0 : y1;
		unsigned c = c0 ? c0 : c1;
		if (c & 3) {
			double X = (c & 1) ? -F : F;
			*y = blot_canvas_line_at(*x0, *y0, *x1, *y1, X);
			*x = X;
		} else {
			double Y = (c & 4) ? -F : F;
			*x = blot_canvas_line_at(*y0, *x0, *y1, *x1, Y);
			*y = Y;
		}
	}

	/* rounding left an end a hair outside: it's far from the canvas */
	*x0 = CLAMP(*x0, -F, F);
	*y0 = CLAMP(*y0, -F, F);
	*x1 = CLAMP(*x1, -F, F);
	*y1 = CLAMP(*y1, -F, F);
	return true;
}

/* the walk's minor cell nearest to the true segment at major coordinate a,
 * as an offset 0..m from the start cell (see blot_canvas_draw_line) */
static inline unsigned long blot_canvas_line_near(double a, double a0, double a1,
						  double b0, double b1,
						  double cb0, double b_dir,
						  unsigned long m)
{
	double b = b0 + (a - a0) * (b1 - b0) / (a1 - a0);
	double d = (b - cb0) * b_dir;
	if (!(d > 0))
		return 0;
	if (d >= (double)m)
		return m;
	return (unsigned long)lround(d);
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
 * Only the steps whose major coordinate is on the canvas are taken (#5),
 * so the time is bounded by the canvas, not by the length of the line.
 * That needs each step's minor cell without the steps before it.  With
 * w(i) the nearest minor offset at step i (non-decreasing in i), the two
 * rules above make the offset of step 0 < i < n
 *
 *     max(min(w(i), i - lag), m - (n - i))
 *
 * where lag is 1 when w(1) is 0 and 0 otherwise.  Step by step, the
 * offset is min(w(i), previous + 1) unless the far end pulls it up, so it
 * is i plus the least w(k) - k for 0 <= k <= i (w(0) = 0).  With a true
 * slope s (minor cells per major step) below 1, w moves at most one cell
 * per step and w(k) - k only falls: the least is at k = i.  Above 1, w
 * moves at least one cell per step (until it reaches m) and w(k) - k
 * rises and then falls: the least is at k = 0, 1 or i.
 *
 * At s = 1 within rounding (a diagonal, true or of the end cells), the
 * rounding of w(k) at each step decides, and w(k) - k may dip anywhere.
 * Such a line is walked step by step from its start instead, as long as
 * that is at most BLOT_CANVAS_LINE_WALK steps; beyond that, the formula
 * may put the visible part one cell off the full walk, which a single
 * rounding far off the canvas would have moved.
 *
 * A segment with a non-finite end is not drawn: lround() of NaN or inf is
 * unspecified (LONG_MIN on x86-64).  How NaN and inf should be shown is #6.
 * Ends farther out than BLOT_CANVAS_LINE_FAR are clipped to it first, on
 * the doubles: beyond the range of a long lround() would flip the line.
 * Within it, the cells are those of the full walk from end to end.  The
 * step and offset arithmetic below is unsigned, so it is defined for any
 * two longs the rounding returns. */
static inline void blot_canvas_draw_line(blot_canvas *can,
					double x0, double y0,
					double x1, double y1)
{
	if (!isfinite(x0) || !isfinite(y0) || !isfinite(x1) || !isfinite(y1))
		return;

	const double F = BLOT_CANVAS_LINE_FAR;
	if (fabs(x0) > F || fabs(y0) > F || fabs(x1) > F || fabs(y1) > F) {
		if (!blot_canvas_line_clip_far(&x0, &y0, &x1, &y1))
			return;
	}

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

	/* the steps whose major coordinate is on the canvas; all of these
	 * are within +-2^42, so they fit a long */
	long size = (long)(x_major ? can->dim.cols : can->dim.rows);
	long start = x_major ? cx0 : cy0;
	long first, last;
	if (a_dir > 0) {
		first = -start;                  /* coordinate 0 */
		last = size - 1 - start;         /* coordinate size-1 */
	} else {
		first = start - (size - 1);
		last = start;
	}
	if (first < 0)
		first = 0;
	if (last > (long)n)
		last = (long)n;
	if (first > last)
		return;

	/* n >= 2 for any step strictly between the ends, so a1 != a0 */
	unsigned long lag = 0;
	bool walk = false;
	if (n >= 2) {
		if (!blot_canvas_line_near(ca0 + a_dir, a0, a1, b0, b1, cb0, b_dir, m))
			lag = 1;

		/* the slope against 1, with room for the rounding of w(k) */
		double s = fabs((b1 - b0) / (a1 - a0));
		double mag = fabs(a0) + fabs(a1) + fabs(b0) + fabs(b1)
			   + fabs(ca0) + fabs(cb0) + 1;
		walk = fabs(s - 1) <= 64 * DBL_EPSILON * mag
		    && (unsigned long)last <= BLOT_CANVAS_LINE_WALK;
	}

	unsigned long off = 0;  /* minor offset from the start cell, 0..m */

	for (unsigned long i = walk ? 0 : (unsigned long)first;
	     i <= (unsigned long)last; i++) {
		if (i == n) {
			off = m;
		} else if (i == 0) {
			off = 0;
		} else {
			double a = ca0 + a_dir * (double)i;
			unsigned long want = blot_canvas_line_near(a, a0, a1, b0, b1,
								   cb0, b_dir, m);
			unsigned long reach = m + i > n ? m + i - n : 0;
			if (walk) {
				/* no gaps: at most one cell from the previous
				 * one, and the far end still reachable in n - i
				 * steps */
				unsigned long lo = off ? off - 1 : 0;
				unsigned long hi = off < m ? off + 1 : m;
				if (lo < reach)
					lo = reach;
				off = CLAMP(want, lo, hi);
			} else {
				off = MAX(MIN(want, i - lag), reach);
			}
		}

		if (i < (unsigned long)first)
			continue;

		unsigned long ox = x_major ? i : off;
		unsigned long oy = x_major ? off : i;

		/* within [cx0,cx1] and between cy0 and cy1, so they fit a long */
		long x = (long)((unsigned long)cx0 + ox);
		long y = y_down ? (long)((unsigned long)cy0 - oy)
				: (long)((unsigned long)cy0 + oy);

		blot_canvas_draw_cell_l(can, x, y);
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

