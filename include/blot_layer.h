/* blot: a layer contains the raw data to be plotted */
/* vim: set noet sw=8 ts=8 tw=80: */
#pragma once

#include <glib.h>
#include "blot_types.h"

typedef struct blot_layer {

	blot_plot_type plot_type;
	blot_data_type data_type;
	size_t count;
	const void *xs;
	const void *ys;
	blot_color color;
	const char *label;

} blot_layer;

/* create/delete */

extern blot_layer * blot_layer_new(blot_plot_type plot_type,
				   blot_data_type data_type,
				   size_t data_count,
				   const void *data_xs,
				   const void *data_ys,
				   blot_color data_color,
				   const char *data_label,
				   GError **);
extern void blot_layer_delete(blot_layer *fig);

/* configure */

/* render */

extern struct blot_canvas * blot_layer_render(blot_layer *lay,
					      const blot_xy_limits *lim,
					      unsigned cols, unsigned rows,
					      blot_render_flags flags,
					      GError **);


