/* blot: a layer contains the raw data to be plotted */
/* vim: set noet sw=8 ts=8 tw=120: */
#include <string.h>
#include <math.h>
#include "blot_layer.h"
#include "blot_error.h"
#include "blot_canvas.h"

/* create/delete */

blot_layer * blot_layer_new(blot_plot_type plot_type,
				   blot_data_type data_type,
				   size_t count,
				   const void *xs,
				   const void *ys,
				   blot_color color,
				   const char *label,
				   GError **error)
{
	blot_layer *lay;

	RETURN_ERRORx(!count, NULL, error, EFAULT, "count is NULL");
	RETURN_ERRORx(!ys, NULL, error, EFAULT, "ys pointer is NULL");

	RETURN_ERRORx(plot_type >= BLOT_PLOT_TYPE_MAX, false, error, EINVAL,
		      "x_min >= x_max");

	lay = g_new(blot_layer, 1);
	RETURN_ERROR(!lay, NULL, error, "new blot_layer");

	lay->plot_type = plot_type;
	lay->data_type = data_type;
	lay->count     = count;
	lay->xs        = xs;
	lay->ys        = ys;
	lay->color     = color;
	lay->label     = label ?: "";

	return lay;
}

void blot_layer_delete(blot_layer *lay)
{
	g_free(lay);
}

/* data */

/* The limits of each coordinate are those of its finite values: NaN and
 * +-inf are not drawn, so they don't count (#6).  A coordinate with no
 * finite value has NaN limits, which the figure reports as "no data".
 * fmin()/fmax() return the other argument when one is NaN, so starting
 * from NaN takes the first finite value. */
bool blot_layer_get_lim(const blot_layer *lay, blot_xy_limits *lim, GError **error)
{
	RETURN_EFAULT_IF(lay==NULL, NULL, error);
	double x, y;
	bool ok;

	if (unlikely (!lay->count)) {
		memset(lim, 0, sizeof(*lim));
		return true;
	}

	lim->x_min = lim->x_max = NAN;
	lim->y_min = lim->y_max = NAN;

	for (int di=0; di<lay->count; di++) {
		ok = blot_layer_get_x_y(lay, di, &x, &y, error);
		RETURN_IF(!ok, lim);

		/* with xs NULL, x is the index: 0 .. count-1 */
		if (likely (isfinite(x))) {
			lim->x_min = fmin(lim->x_min, x);
			lim->x_max = fmax(lim->x_max, x);
		}

		if (likely (isfinite(y))) {
			lim->y_min = fmin(lim->y_min, y);
			lim->y_max = fmax(lim->y_max, y);
		}
	}

	return true;
}

/* summary */

static inline void blot_layer_summary_init(struct blot_layer_summary *sum, blot_render_flags flags)
{
	if (!(flags & BLOT_RENDER_LEGEND_DETAILS)) {
		sum->enabled = false;
		return;
	}

	memset(sum, 0, sizeof(*sum));
	sum->enabled = true;
}

static void blot_layer_summary_update(struct blot_layer_summary *sum, double x, double y)
{
	if (likely (!sum->enabled))
		return;

	if (likely (sum->count)) {
		if (sum->xmax < x) sum->xmax = x;
		if (sum->xmin > x) sum->xmin = x;
		sum->xttl += x;
		if (sum->ymax < y) sum->ymax = y;
		if (sum->ymin > y) sum->ymin = y;
		sum->yttl += y;
		sum->count ++;
	} else {
		sum->xmax = x;
		sum->xmin = x;
		sum->xttl = x;
		sum->ymax = y;
		sum->ymin = y;
		sum->yttl = y;
		sum->count = 1;
	}
}

/* render */

static bool blot_layer_scatter(blot_layer *lay, const blot_xy_limits *lim,
			 blot_canvas *can, GError **);
static bool blot_layer_scatter_int64(blot_layer *lay, const blot_xy_limits *lim,
			       blot_canvas *can, GError **);
static bool blot_layer_line(blot_layer *lay, const blot_xy_limits *lim,
		      blot_canvas *can, GError **);
static bool blot_layer_bar(blot_layer *lay, const blot_xy_limits *lim,
		      blot_canvas *can, GError **);

typedef bool (*layer_to_canvas_fn)(blot_layer *lay, const blot_xy_limits *lim,
				   blot_canvas *can, GError **);
static layer_to_canvas_fn blot_layer_to_canvas_type_fns[BLOT_PLOT_TYPE_MAX][BLOT_DATA_TYPE_MAX] = {
	[BLOT_SCATTER] = {
		[BLOT_DATA_INT64] = blot_layer_scatter_int64,
	}
};
static layer_to_canvas_fn blot_layer_to_canvas_fns[BLOT_PLOT_TYPE_MAX] = {
	[BLOT_SCATTER]   = blot_layer_scatter,
	[BLOT_LINE]      = blot_layer_line,
	[BLOT_BAR]       = blot_layer_bar,
};

struct blot_canvas * blot_layer_render(blot_layer *lay,
				       const blot_xy_limits *lim,
				       const blot_dimensions *dim,
				       blot_render_flags flags,
				       GError **error)
{
	RETURN_EFAULT_IF(lay==NULL, NULL, error);
	RETURN_EINVAL_IF(lay->plot_type>=BLOT_PLOT_TYPE_MAX, NULL, error);
	RETURN_EINVAL_IF(lay->data_type>=BLOT_DATA_TYPE_MAX, NULL, error);

	blot_canvas *can = blot_canvas_new(dim->cols, dim->rows, flags, lay->color, error);
	RETURN_IF(!can, NULL);

	blot_layer_summary_init(&lay->summary, flags);

	layer_to_canvas_fn fn;
	/* try to find function specialized for this type */
	fn = blot_layer_to_canvas_type_fns[lay->plot_type][lay->data_type];
	if (!fn)
		/* otherwise use the generic function, that will be a bit slower */
		fn = blot_layer_to_canvas_fns[lay->plot_type];

	RETURN_ERRORx(!fn, NULL, error, EINVAL,
		      "no handler for plot_type=%u", lay->plot_type);

	bool plot_ok = fn(lay, lim, can, error);
	if (!plot_ok) {
		blot_canvas_delete(can);
		return NULL;
	}

	return can;
}

/* data to canvas mapping
 *
 * A value v between min and max goes to cell floor((v - min) * len / (max - min)), and max itself to the last
 * cell, len - 1.  This is the mapping the axis ticks use (blot_axis_new), so a mark is drawn in the cell whose
 * tick reads its value.  The arithmetic is done in double; a value outside min..max (or NaN) has no cell. */

static inline double blot_layer_pos(double v, double min, double max, unsigned len)
{
	double pos = (v - min) * len / (max - min);

	/* (v - min) * len overflows to inf when the range is wider than
	 * about DBL_MAX / len (#6); divide first then.  Only then: for every
	 * other value the cell is exactly the one of the formula above, the
	 * axis's.  A v so far outside the limits that v - min itself is
	 * inf stays inf, and has no cell. */
	if (unlikely (isinf(pos)) && isfinite(v - min))
		pos = (v - min) / (max - min) * len;

	return pos;
}

static inline bool blot_layer_cell(double v, double min, double max, unsigned len, unsigned *cell)
{
	double pos = blot_layer_pos(v, min, max, len);

	/* written so that NaN fails too */
	if (!(pos >= 0 && pos <= len))
		return false;

	*cell = pos < len ? (unsigned)floor(pos) : len - 1;
	return true;
}

/* scatter */

static bool blot_layer_scatter(blot_layer *lay, const blot_xy_limits *lim,
			 blot_canvas *can, GError **error)
{
	RETURN_ERRORx(lim->x_max <= lim->x_min, false, error, ERANGE, "invalid column limits %f..%f", lim->x_min, lim->x_max);
	RETURN_ERRORx(lim->y_max <= lim->y_min, false, error, ERANGE, "invalid row limits %f..%f", lim->y_min, lim->y_max);

	for (int di=0; di<lay->count; di++) {
		// read the location
		double rx, ry;

		gboolean ok = blot_layer_get_x_y(lay, di, &rx, &ry, error);
		if (unlikely (!ok))
			return false;

		blot_layer_summary_update(&lay->summary, rx, ry);

		// compute location, skip points outside the limits
		unsigned cx, cy;
		if (!blot_layer_cell(rx, lim->x_min, lim->x_max, can->dim.cols, &cx)
		    || !blot_layer_cell(ry, lim->y_min, lim->y_max, can->dim.rows, &cy))
			continue;

		// plot it
		blot_canvas_draw_point(can, cx, cy);
	}
	return true;
}

static bool blot_layer_scatter_int64(blot_layer *lay, const blot_xy_limits *lim,
			       blot_canvas *can, GError **error)
{
	/* this function should never be called with any other data type */
	g_assert(lay->data_type == BLOT_DATA_INT64);

	const gint64 *xs = lay->xs;
	const gint64 *ys = lay->ys;

	RETURN_ERRORx(!lay->ys, false, error, ENOENT, "Y-data is NULL");
	RETURN_ERRORx(lim->x_max <= lim->x_min, false, error, ERANGE, "invalid column limits %f..%f", lim->x_min, lim->x_max);
	RETURN_ERRORx(lim->y_max <= lim->y_min, false, error, ERANGE, "invalid row limits %f..%f", lim->y_min, lim->y_max);

	for (int di=0; di<lay->count; di++) {
		// read the location
		double rx, ry;

		rx = xs ? xs[di] : di;
		ry = ys[di];

		blot_layer_summary_update(&lay->summary, rx, ry);

		// compute location, skip points outside the limits
		unsigned cx, cy;
		if (!blot_layer_cell(rx, lim->x_min, lim->x_max, can->dim.cols, &cx)
		    || !blot_layer_cell(ry, lim->y_min, lim->y_max, can->dim.rows, &cy))
			continue;

		// plot it
		blot_canvas_draw_point(can, cx, cy);
	}
	return true;
}

static bool blot_layer_line(blot_layer *lay, const blot_xy_limits *lim,
		      blot_canvas *can, GError **error)
{
	double x_range = lim->x_max - lim->x_min;
	double y_range = lim->y_max - lim->y_min;

	RETURN_ERRORx(x_range <= 0, false, error, ERANGE, "invalid column limits %f..%f", lim->x_min, lim->x_max);
	RETURN_ERRORx(x_range <= 0, false, error, ERANGE, "invalid column limits %f..%f", lim->x_min, lim->x_max);

	double per_col = (double)(can->dim.cols-1) / x_range;
	double per_row = (double)(can->dim.rows-1) / y_range;

	bool have_prev = false;
	double px=0, py=0;

	for (int di=0; di<lay->count; di++) {
		// read the location
		double rx, ry;

		gboolean ok = blot_layer_get_x_y(lay, di, &rx, &ry, error);
		if (unlikely (!ok))
			return false;

		blot_layer_summary_update(&lay->summary, rx, ry);

		// a NaN or infinite point is a gap (#6): no segment goes to it
		// or from it, and the next finite point starts a new run, which
		// is drawn as a single cell if nothing follows it
		if (unlikely (!isfinite(rx) || !isfinite(ry))) {
			have_prev = false;
			continue;
		}

		// compute location
		double dx = (double)(rx - lim->x_min) * per_col;
		double dy = (double)(ry - lim->y_min) * per_row;

		// plot it: a segment from the previous point, or the first
		// point on its own so that a one-point series is drawn too
		if (likely (have_prev))
			blot_canvas_draw_line(can, px, py, dx, dy);
		else
			blot_canvas_draw_line(can, dx, dy, dx, dy);

		// remember current point for next line
		px = dx;
		py = dy;
		have_prev = true;
	}
	return true;
}

static bool blot_layer_bar(blot_layer *lay, const blot_xy_limits *lim,
		      blot_canvas *can, GError **error)
{
	RETURN_ERRORx(lim->x_max <= lim->x_min, false, error, ERANGE, "invalid column limits %f..%f", lim->x_min, lim->x_max);
	RETURN_ERRORx(lim->y_max <= lim->y_min, false, error, ERANGE, "invalid row limits %f..%f", lim->y_min, lim->y_max);

	/* X uses a slot model, not the axis mapping: the X range is cut
	 * into (x_max - x_min + 1) slots of equal width, one per integer
	 * from x_min to x_max, and a bar fills the left half of its slot.
	 * With x = 0..N-1 (xs NULL) that is N slots, and bar i sits under
	 * custom X label i, which blot_axis_new spreads at the same
	 * len / N spacing.  On a numeric X axis the slots and the tick
	 * values disagree; that is the bar layout question of #12. */

	double slots = lim->x_max - lim->x_min + 1;
	double slot_cols = can->dim.cols / slots;
	double half_slot = 0.5 * slot_cols;

	for (int di=0; di<lay->count; di++) {
		// read the location
		double rx, ry;

		gboolean ok = blot_layer_get_x_y(lay, di, &rx, &ry, error);
		if (unlikely (!ok))
			return false;

		blot_layer_summary_update(&lay->summary, rx, ry);

		// skip a bar whose x is outside the limits (also NaN), and one
		// whose height is NaN or infinite: +inf is not a bar cut at
		// the top (#6)
		if (!(rx >= lim->x_min && rx <= lim->x_max) || !isfinite(ry))
			continue;

		// columns: the left edge of the slot, and half a slot to the right;
		// x_max is in slot (slots - 1), which starts before the last column
		double sx = (rx - lim->x_min) * slot_cols;
		unsigned x0 = min_t(double, round(sx), can->dim.cols - 1);
		unsigned x1 = min_t(double, round(sx + half_slot), can->dim.cols - 1);

		// height: the axis mapping; skip a bar whose top is below the
		// limits, cut one that is taller at the top row
		unsigned cy;
		if (ry > lim->y_max)
			cy = can->dim.rows - 1;
		else if (!blot_layer_cell(ry, lim->y_min, lim->y_max, can->dim.rows, &cy))
			continue;

		// plot it
		blot_canvas_fill_rect(can, x0, 0, x1, cy);
	}

	return true;
}
