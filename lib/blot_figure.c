/* blot: figure represents the top level object */
/* vim: set noet sw=8 ts=8 tw=80: */
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

	fig->screen_size_set = true;
	fig->cols = cols;
	fig->rows = rows;
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

bool blot_figure_scatter(blot_figure *fig, blot_data_type data_type,
				size_t data_count, const void *data_xs, const void *data_ys,
				blot_color data_color, const char *data_label,
				GError **error)
{
	RETURN_EFAULT_IF(fig==NULL, false, error);

	blot_layer *lay = blot_layer_new(BLOT_SCATTER,
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

static blot_xy_limits blot_figure_auto_limits(blot_figure *fig, GError **error)
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

	return lim;
}

static inline void __free_canvases_array(blot_canvas **cans, unsigned count)
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

	if (!fig->screen_size_set) {
		int term_cols, term_rows;
		bool ok = blot_terminal_get_size(&term_cols, &term_rows, error);
		RETURN_IF(!ok, NULL);

		/* use the entire terminal screen, w/o the border */
		fig->cols = max_t(unsigned, term_cols-5, BLOT_MIN_COLS);
		fig->rows = max_t(unsigned, term_rows-5, BLOT_MIN_ROWS);
	}

	RETURN_EINVAL_IF(fig->cols<BLOT_MIN_COLS, NULL, error);
	RETURN_EINVAL_IF(fig->rows<BLOT_MIN_ROWS, NULL, error);

	blot_xy_limits lim = blot_figure_auto_limits(fig, error);
	RETURN_IF(*error, NULL);

	g_autofree blot_canvas **cans = g_new0(blot_canvas*, fig->layer_count);
	RETURN_ERROR(!cans, NULL, error, "new *canvas x %u", fig->layer_count);

	for (int li=0; li<fig->layer_count; li++) {
		blot_layer *lay = fig->layers[li];

		cans[li] = blot_layer_render(lay, &lim,
					     fig->cols, fig->rows,
					     flags, error);
		if (cans[li])
			continue;

		/* error: unwind the canvases we allocated */
		for (--li; li>=0; li--)
			blot_canvas_delete(cans[--li]);
		return NULL;
	}

	blot_screen *scr = blot_screen_new(fig->cols, fig->rows,
					   flags, error);
	if (!scr) {
		__free_canvases_array(cans, fig->layer_count);
		return NULL;
	}

	bool render_ok = blot_screen_render(scr, fig->layer_count,
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

