/* blot: a layer contains the raw data to be plotted */
/* vim: set noet sw=8 ts=8 tw=80: */
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

/* render */

bool blot_layer_get_double(blot_layer *lay, unsigned index,
				  double *x, double *y, GError **error)
{
	/* current limitation, we only support INT64 data */
	g_assert(lay->data_type == BLOT_LAYER_INT64);
	const gint64 *xs = lay->xs;
	const gint64 *ys = lay->ys;

	*x = xs[index];
	*y = ys[index];
	return true;
}

/* render */

static bool blot_scatter(const blot_layer *lay, const blot_xy_limits *lim,
			 blot_canvas *can, GError **);

typedef bool (*layer_to_canvas_fn)(const blot_layer *lay, const blot_xy_limits *lim,
				   blot_canvas *can, GError **);
static layer_to_canvas_fn blot_layer_to_canvas_fns[BLOT_PLOT_TYPE_MAX] = {
	[BLOT_SCATTER] = blot_scatter,
};

struct blot_canvas * blot_layer_render(blot_layer *lay,
				       const blot_xy_limits *lim,
				       unsigned cols, unsigned rows,
				       blot_render_flags flags,
				       GError **error)
{
	RETURN_EFAULT_IF(lay==NULL, NULL, error);

	blot_canvas *can = blot_canvas_new(cols, rows, flags, lay->color, error);
	RETURN_IF(!can, NULL);

	layer_to_canvas_fn fn = blot_layer_to_canvas_fns[lay->plot_type];
	bool plot_ok = fn(lay, lim, can, error);
	if (!plot_ok) {
		blot_canvas_delete(can);
		return NULL;
	}

	return can;
}

static bool blot_scatter(const blot_layer *lay, const blot_xy_limits *lim,
			 blot_canvas *can, GError **error)
{
	/* current limitation, we only support INT64 data */
	g_assert(lay->data_type == BLOT_LAYER_INT64);
	const gint64 *xs = lay->xs;
	const gint64 *ys = lay->ys;

	double x_range = lim->x_max - lim->x_min;
	double y_range = lim->y_max - lim->y_min;

	for (int di=0; di<lay->count; di++) {
		// compute location
		double dx = (double)(xs[di] - lim->x_min) * can->cols / x_range;
		double dy = (double)(ys[di] - lim->y_min) * can->rows / y_range;

		// convert to integer
		unsigned ux = dx;
		unsigned uy = dy;

		// out of bounds
		if (ux >= can->cols || uy >= can->rows)
			continue;

		// and finally plot the point
		blot_canvas_set(can, ux, uy, 1);
	}

	return true;
}
