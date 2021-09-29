/* blot: a layer contains the raw data to be plotted */
/* vim: set noet sw=8 ts=8 tw=80: */
#include "blot_layer.h"
#include "blot_error.h"
#include "blot_color.h"

/* create/delete */

extern blot_layer * blot_layer_new(blot_plot_type plot_type,
				   blot_data_type data_type,
				   size_t data_count,
				   const void *data_xs,
				   const void *data_ys,
				   blot_color data_color,
				   const char *data_label,
				   GError **error)
{
	blot_layer *lay;

	RETURN_ERRORx(plot_type >= BLOT_PLOT_TYPE_MAX, false, error, EINVAL,
		      "x_min >= x_max");


	lay = g_new(blot_layer, 1);
	RETURN_ERROR(!lay, NULL, error, "new blot_layer");

	lay->plot_type = plot_type;
	lay->data_type = data_type;
	lay->data_count = data_count;
	lay->data_xs = data_xs;
	lay->data_ys = data_ys;
	lay->data_color = data_color;
	lay->data_label = data_label;

	return lay;
}

void blot_layer_delete(blot_layer *lay)
{
	g_free(lay);
}


