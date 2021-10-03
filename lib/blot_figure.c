/* blot: figure represents the top level object */
/* vim: set noet sw=8 ts=8 tw=80: */
#include <string.h>
#include <math.h>
#include "blot_figure.h"
#include "blot_error.h"
#include "blot_color.h"
#include "blot_layer.h"
#include "blot_canvas.h"
#include "blot_screen.h"
#include "blot_terminal.h"

/* create/delete */

bool blot_figure_init(blot_figure *fig, GError **error)
{
	memset(fig, 0, sizeof(*fig));

	/* config */
	fig->axis_color = WHITE;

	return true;
}

blot_figure * blot_figure_new(GError **error)
{
	blot_figure *fig;

	fig = g_new(blot_figure, 1);
	RETURN_ERROR(!fig, NULL, error, "new blot_figure");

	bool ok = blot_figure_init(fig, error);
	RETURN_IF(!ok, NULL);

	return fig;
}

void blot_figure_delete(blot_figure *fig)
{
	g_free(fig);
}


/* configure */

bool blot_figure_set_axis_color(blot_figure *fig, blot_color color, GError **error)
{
	RETURN_EFAULT_IF(fig==NULL, false, error);
	fig->axis_color = color;
	return true;
}


bool blot_figure_set_screen_size(blot_figure *fig,
				 unsigned cols, unsigned rows, GError **error)
{
	RETURN_EFAULT_IF(fig==NULL, false, error);
	RETURN_EINVAL_IF(cols<BLOT_MIN_COLS, false, error);
	RETURN_EINVAL_IF(rows<BLOT_MIN_ROWS, false, error);

	fig->screen_dimensions_set = true;
	fig->dim.cols = cols;
	fig->dim.rows = rows;
	return true;
}


bool blot_figure_set_x_limits(blot_figure *fig,
			      double x_min, double x_max, GError **error)
{
	RETURN_EFAULT_IF(fig==NULL, false, error);
	RETURN_ERRORx(x_min >= x_max, false, error, EINVAL,
		      "x_min >= x_max");

	fig->x_limits_set = true;
	fig->lim.x_min = x_min;
	fig->lim.x_max = x_max;
	return true;
}

bool blot_figure_set_y_limits(blot_figure *fig,
			      double y_min, double y_max, GError **error)
{
	RETURN_EFAULT_IF(fig==NULL, false, error);
	RETURN_ERRORx(y_min >= y_max, false, error, EINVAL,
		      "y_min >= y_max");

	fig->y_limits_set = true;
	fig->lim.y_min = y_min;
	fig->lim.y_max = y_max;
	return true;
}


/* add layers */

bool blot_figure_plot(blot_figure *fig,
		      blot_plot_type plot_type, blot_data_type data_type,
		      size_t data_count, const void *data_xs, const void *data_ys,
		      blot_color data_color, const char *data_label,
		      GError **error)
{
	RETURN_EFAULT_IF(fig==NULL, false, error);

	blot_layer *lay = blot_layer_new(plot_type,
					 data_type,
					 data_count,
					 data_xs,
					 data_ys,
					 data_color,
					 data_label,
					 error);
	RETURN_IF(lay==NULL,false);

	blot_layer **layers = g_realloc_n(fig->layers,
					 fig->layer_count + 1,
					 sizeof(blot_layer*));
	RETURN_ERROR(!layers, NULL, error,
		     "realloc *blot_layers x %u", fig->layer_count + 1);

	fig->layers = layers;
	fig->layers[fig->layer_count] = lay;
	fig->layer_count ++;

	return true;
}


/* render */

static blot_dimensions blot_figure_finalize_dimensions(const blot_figure *fig,
						       GError **error)
{
	if (fig->screen_dimensions_set)
		return fig->dim;

	blot_dimensions dim = {0,};

	int term_cols, term_rows;
	bool ok = blot_terminal_get_size(&term_cols, &term_rows, error);
	RETURN_IF(!ok, dim);

	/* use the entire terminal screen, w/o the border */
	dim.cols = max_t(unsigned, term_cols-2, BLOT_MIN_COLS);
	dim.rows = max_t(unsigned, term_rows-2, BLOT_MIN_ROWS);

	RETURN_EINVAL_IF(dim.cols<BLOT_MIN_COLS, dim, error);
	RETURN_EINVAL_IF(dim.rows<BLOT_MIN_ROWS, dim, error);

	return dim;
}

static blot_xy_limits blot_figure_finalize_limits(const blot_figure *fig,
						  GError **error)
{
	if (fig->x_limits_set && fig->y_limits_set)
		return fig->lim;

	blot_xy_limits lim = {0,0,0,0};
	bool something_set = false;

	for (int li=0; li<fig->layer_count; li++) {
		blot_layer *lay = fig->layers[li];

		if (unlikely (!something_set && lay->count)) {
			double x, y;
			bool ok = blot_layer_get_double(lay, 0, &x, &y, error);
			RETURN_IF(!ok, lim);

			lim.x_min = lim.x_max = x;
			lim.y_min = lim.y_max = y;
			something_set = true;
		}

		if (lay->plot_type == BLOT_HISTOGRAM && lay->count) {
			double x_min = -0.5;
			double x_max = lay->count - 0.5;

			lim.x_min = min_t(double, lim.x_min, x_min);
			lim.x_max = max_t(double, lim.x_max, x_max);
		}

		for (int di=0; di<lay->count; di++) {
			double x, y;
			bool ok = blot_layer_get_double(lay, di, &x, &y, error);
			RETURN_IF(!ok, lim);

			lim.x_min = min_t(double, lim.x_min, x);
			lim.x_max = max_t(double, lim.x_max, x);

			lim.y_min = min_t(double, lim.y_min, y);
			lim.y_max = max_t(double, lim.y_max, y);
		}
	}

	if (unlikely (!something_set))
		blot_set_error_unix(error, ENOENT,
				    "could not determine limits automatically, since there is no data");

	double x_range = lim.x_max - lim.x_min;
	if (!x_range) {
		lim.x_min --;
		lim.x_max ++;
	}

	double y_range = lim.y_max - lim.y_min;
	if (!y_range) {
		lim.y_min --;
		lim.y_max ++;
	}

	return lim;
}

static blot_margins blot_figure_finalize_margins(const blot_figure *fig,
						 blot_render_flags flags,
						 const blot_xy_limits *lim,
						 const blot_dimensions *dim,
						 GError **error)
{
	blot_margins mrg = {0,};

	if (!(flags & BLOT_RENDER_NO_X_AXIS)) {
		/* we will draw an X-axis */
		mrg.bottom += 3;

		/* we always reserve 1 line for text */
		mrg.bottom += 1;
	}

	if (!(flags & BLOT_RENDER_NO_Y_AXIS)) {
		/* we will draw an Y-axis */
		mrg.left += 3;

		/* how big are the numbers? */
		double amin = ceil(abs_t(double, lim->y_min));
		double wmin = ceil(log10(amin));
		if (lim->y_min < 0) wmin ++;

		double amax = ceil(abs_t(double, lim->y_max));
		double wmax = ceil(log10(amax));
		if (lim->y_max < 0) wmax ++;

		/* worst case we need this many columns for text */
		mrg.left += max_t(unsigned, wmin, wmax);

		/* add 3 digits of precision */
		mrg.left += 4;
	}

#if 1
	/* for testing only */
	mrg.right += 1;
	mrg.top += 1;
#endif

	return mrg;
}

static blot_dimensions blot_figure_finalize_usable(const blot_figure *fig,
						   const blot_dimensions *dim,
						   const blot_margins *mrg,
						   GError **error)
{
	blot_dimensions use = {0,};
	unsigned mrg_cols = mrg->left + mrg->right;
	unsigned mrg_rows = mrg->top + mrg->bottom;

	RETURN_EFAULT_IF(mrg_cols>dim->cols, use, error);
	RETURN_EFAULT_IF(mrg_rows>dim->rows, use, error);

	use.cols = dim->cols - mrg_cols;
	use.rows = dim->rows - mrg_rows;

	RETURN_EINVAL_IF(use.cols<BLOT_MIN_COLS, use, error);
	RETURN_EINVAL_IF(use.rows<BLOT_MIN_ROWS, use, error);

	return use;
}

static void __free_canvases_array(blot_canvas **cans, unsigned count)
{
	for (int ci=0; ci<count; ci++) {
		if (!cans[ci])
			continue;
		blot_canvas_delete(cans[ci]);
		cans[ci] = NULL;
	}
}

blot_screen * blot_figure_render(blot_figure *fig, blot_render_flags flags,
				 GError **error)
{
	RETURN_EFAULT_IF(fig==NULL, NULL, error);
	RETURN_EINVAL_IF(fig->layer_count==0, NULL, error);
	RETURN_EINVAL_IF(fig->layers==NULL, NULL, error);

	/* finalize the screen dimensions */

	blot_dimensions dim = blot_figure_finalize_dimensions(fig, error);
	RETURN_IF(*error, NULL);

	/* finalize X/Y limits */

	blot_xy_limits lim = blot_figure_finalize_limits(fig, error);
	RETURN_IF(*error, NULL);

	/* finalize the portion of the screen that is used for plotting */

	blot_margins mrg = blot_figure_finalize_margins(fig, flags,
							&lim, &dim, error);
	RETURN_IF(*error, NULL);

	/* finalize the usable screen available */

	blot_dimensions use = blot_figure_finalize_usable(fig, &dim, &mrg,
							  error);
	RETURN_IF(*error, NULL);

	/* generate the canvases */

	g_autofree blot_canvas **cans = g_new0(blot_canvas*, fig->layer_count);
	RETURN_ERROR(!cans, NULL, error, "new *canvas x %u", fig->layer_count);

	for (int li=0; li<fig->layer_count; li++) {
		blot_layer *lay = fig->layers[li];

		cans[li] = blot_layer_render(lay, &lim, &use,
					     flags, error);
		if (cans[li])
			continue;

		/* error: unwind the canvases we allocated */
		for (--li; li>=0; li--)
			blot_canvas_delete(cans[--li]);
		return NULL;
	}

	/* merge canvases to screen */

	blot_screen *scr = blot_screen_new(&dim, &mrg, flags, error);
	if (!scr) {
		__free_canvases_array(cans, fig->layer_count);
		return NULL;
	}

	bool render_ok = blot_screen_render(scr, fig->axis_color, &lim, fig->layer_count,
					    fig->layers, cans, error);

	/* we no longer need the canvases */
	__free_canvases_array(cans, fig->layer_count);

	/* if render to screen failed, we return */
	if (!render_ok) {
		blot_screen_delete(scr);
		return NULL;
	}

	return scr;
}

