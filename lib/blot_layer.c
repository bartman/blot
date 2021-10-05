/* blot: a layer contains the raw data to be plotted */
/* vim: set noet sw=8 ts=8 tw=80: */
#include <string.h>
#include "blot_layer.h"
#include "blot_error.h"
#include "blot_color.h"
#include "blot_canvas.h"

/* create/delete */

extern blot_layer * blot_layer_new(blot_plot_type plot_type,
				   blot_data_type data_type,
				   size_t count,
				   const void *xs,
				   const void *ys,
				   blot_color color,
				   const char *label,
				   GError **error)
{
	blot_layer *lay;

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
	lay->label     = label;

	return lay;
}

void blot_layer_delete(blot_layer *lay)
{
	g_free(lay);
}

/* data */

bool blot_layer_get_lim(const blot_layer *lay, blot_xy_limits *lim, GError **error)
{
	RETURN_EFAULT_IF(lay==NULL, NULL, error);
	double x, y;
	bool ok;

	if (unlikely (!lay->count)) {
		memset(lim, 0, sizeof(*lim));
		return true;
	}

	if (unlikely (!lay->xs)) {

		lim->x_min = 0;
		lim->x_max = lay->count;

		ok = blot_layer_get_y(lay, 0, &y, error);
		RETURN_IF(!ok, lim);

		lim->y_min = lim->y_max = y;

		for (int di=1; di<lay->count; di++) {
			ok = blot_layer_get_y(lay, di, &y, error);
			RETURN_IF(!ok, lim);

			lim->y_min = min_t(double, lim->y_min, y);
			lim->y_max = max_t(double, lim->y_max, y);
		}
		return true;
	}

	ok = blot_layer_get_x_y(lay, 0, &x, &y, error);
	RETURN_IF(!ok, lim);

	lim->x_min = lim->x_max = x;
	lim->y_min = lim->y_max = y;

	for (int di=1; di<lay->count; di++) {
		ok = blot_layer_get_x_y(lay, di, &x, &y, error);
		RETURN_IF(!ok, lim);

		lim->x_min = min_t(double, lim->x_min, x);
		lim->x_max = max_t(double, lim->x_max, x);

		lim->y_min = min_t(double, lim->y_min, y);
		lim->y_max = max_t(double, lim->y_max, y);
	}

	return true;
}

/* render */

static bool blot_layer_scatter(const blot_layer *lay, const blot_xy_limits *lim,
			 blot_canvas *can, GError **);
static bool blot_layer_scatter_int64(const blot_layer *lay, const blot_xy_limits *lim,
			       blot_canvas *can, GError **);
static bool blot_layer_line(const blot_layer *lay, const blot_xy_limits *lim,
		      blot_canvas *can, GError **);
static bool blot_layer_bar(const blot_layer *lay, const blot_xy_limits *lim,
		      blot_canvas *can, GError **);

typedef bool (*layer_to_canvas_fn)(const blot_layer *lay, const blot_xy_limits *lim,
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
	RETURN_EINVAL_IF(lay->plot_type>BLOT_PLOT_TYPE_MAX, NULL, error);
	RETURN_EINVAL_IF(lay->data_type>BLOT_DATA_TYPE_MAX, NULL, error);

	blot_canvas *can = blot_canvas_new(dim->cols, dim->rows, flags, lay->color, error);
	RETURN_IF(!can, NULL);

	layer_to_canvas_fn fn;
	/* try to find function specialized for this type */
	fn = blot_layer_to_canvas_type_fns[lay->plot_type][lay->data_type];
	if (!fn)
		/* otherwise use the generic function, that will be a bit slower */
		fn = blot_layer_to_canvas_fns[lay->plot_type];

	RETURN_ERRORx(!fn, false, error, EINVAL,
		      "no handler for plot_type=%u", lay->plot_type);

	bool plot_ok = fn(lay, lim, can, error);
	if (!plot_ok) {
		blot_canvas_delete(can);
		return NULL;
	}

	return can;
}

/* scatter */

static bool blot_layer_scatter(const blot_layer *lay, const blot_xy_limits *lim,
			 blot_canvas *can, GError **error)
{
	double x_range = lim->x_max - lim->x_min;
	double y_range = lim->y_max - lim->y_min;

	for (int di=0; di<lay->count; di++) {
		// read the location
		double rx, ry;

		gboolean ok = blot_layer_get_x_y(lay, di, &rx, &ry, error);
		if (unlikely (!ok))
			return false;

		// compute location
		double dx = (double)(rx - lim->x_min) * can->dim.cols / x_range;
		double dy = (double)(ry - lim->y_min) * can->dim.rows / y_range;

		// plot it
		blot_canvas_draw_point(can, dx, dy);
	}
	return true;
}

static bool blot_layer_scatter_int64(const blot_layer *lay, const blot_xy_limits *lim,
			       blot_canvas *can, GError **error)
{
	/* this function should never be called with any other data type */
	g_assert(lay->data_type == BLOT_DATA_INT64);

	const gint64 *xs = lay->xs;
	const gint64 *ys = lay->ys;

	RETURN_ERRORx(!lay->ys, false, error, ENOENT, "Y-data is NULL");

	double x_range = lim->x_max - lim->x_min;
	double y_range = lim->y_max - lim->y_min;

	for (int di=0; di<lay->count; di++) {
		// read the location
		double rx, ry;

		rx = xs ? xs[di] : di;
		ry = ys[di];

		// compute location
		double dx = (double)(rx - lim->x_min) * can->dim.cols / x_range;
		double dy = (double)(ry - lim->y_min) * can->dim.rows / y_range;

		// plot it
		blot_canvas_draw_point(can, dx, dy);
	}
	return true;
}

static bool blot_layer_line(const blot_layer *lay, const blot_xy_limits *lim,
		      blot_canvas *can, GError **error)
{
	double x_range = lim->x_max - lim->x_min;
	double y_range = lim->y_max - lim->y_min;

	bool visible = false;
	double px, py;

	for (int di=0; di<lay->count; di++) {
		// read the location
		double rx, ry;

		gboolean ok = blot_layer_get_x_y(lay, di, &rx, &ry, error);
		if (unlikely (!ok))
			return false;

		// compute location
		double dx = (double)(rx - lim->x_min) * can->dim.cols / x_range;
		double dy = (double)(ry - lim->y_min) * can->dim.rows / y_range;

		// plot it
		if (likely (visible)) {
			blot_canvas_draw_line(can, px, py, dx, dy);
		}

		// remember current point for next line
		px = dx;
		py = dy;
		visible = true;
	}
	return true;
}

static bool blot_layer_bar(const blot_layer *lay, const blot_xy_limits *lim,
		      blot_canvas *can, GError **error)
{
	double x_range = lim->x_max - lim->x_min;
	double y_range = lim->y_max - lim->y_min;

	/* bar graphs are plotted as rectangles of this width */

	double wx = (double)(0.5) * can->dim.cols / x_range;

	for (int di=0; di<lay->count; di++) {
		// read the location
		double rx, ry;

		gboolean ok = blot_layer_get_x_y(lay, di, &rx, &ry, error);
		if (unlikely (!ok))
			return false;

		// compute location
		double dx = (double)(rx - lim->x_min) * can->dim.cols / x_range;
		double dy = (double)(ry - lim->y_min) * can->dim.rows / y_range;

		// plot it
		//blot_canvas_draw_rect(can, dx, 0, dx+wx, dy);
		blot_canvas_fill_rect(can, dx, 0, dx+wx, dy);
	}

	return true;
}
