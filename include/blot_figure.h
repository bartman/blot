/* blot: figure represents the top level object */
/* vim: set noet sw=8 ts=8 tw=80: */
#pragma once

#include <glib.h>
#include "blot_types.h"

#define BLOT_MIN_COLUMNS 10
#define BLOT_MIN_ROWS 10

typedef struct blot_figure {
	/* config */
	blot_color axis_color;

	bool screen_size_set;
	unsigned columns, rows;

	bool x_limits_set;
	double x_min, x_max;

	bool y_limits_set;
	double y_min, y_max;

	/* layers */
	gsize layer_count;
	struct blot_layer *layers;

} blot_figure;

/* create/delete */

extern bool blot_figure_init(blot_figure *fig, GError **);
extern blot_figure * blot_figure_new(GError **);
extern void blot_figure_delete(blot_figure *fig);

/* configure */

extern bool blot_figure_set_axis_color(blot_figure *fig, blot_color, GError **);

extern bool blot_figure_set_screen_size(blot_figure *fig,
					unsigned columns, unsigned rows, GError **);

extern bool blot_figure_set_x_limits(blot_figure *fig,
				     double x_min, double x_max, GError **);
extern bool blot_figure_set_y_limits(blot_figure *fig,
				     double y_min, double y_max, GError **);

/* add layers */

extern bool blot_figure_scatter(blot_figure *fig, blot_data_type data_type,
				size_t data_count, const void *data_xs, const void *data_ys,
				blot_color data_color, const char *data_label,
				GError **);

/* render */

enum blot_figure_render_flags {
	BLOT_FIGURE_RENDER_NONE = 0
};

extern char * blot_figure_render(blot_figure *fig, enum blot_figure_render_flags flags,
				 gsize *txt_size, GError **);
extern void blot_figure_render_free(blot_figure *fig, char *txt);

