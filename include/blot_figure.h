/* blot: figure represents the top level object */
/* vim: set noet sw=8 ts=8 tw=80: */
#pragma once

#include <glib.h>
#include "blot_types.h"

#define BLOT_MIN_COLS 10
#define BLOT_MIN_ROWS 10

typedef struct blot_figure {
	/* config */
	blot_color axis_color;

	bool screen_dimensions_set;
	blot_dimensions dim;

	bool x_limits_set;
	bool y_limits_set;
	blot_xy_limits lim;

	blot_strv xlabels;

	/* layers */
	gsize layer_count;
	struct blot_layer **layers;

} blot_figure;

/* create/delete */

extern bool blot_figure_init(blot_figure *fig, GError **);
extern blot_figure * blot_figure_new(GError **);
extern void blot_figure_delete(blot_figure *fig);

/* configure */

extern bool blot_figure_set_axis_color(blot_figure *fig, blot_color, GError **);

extern bool blot_figure_set_screen_size(blot_figure *fig,
					unsigned cols, unsigned rows, GError **);

extern bool blot_figure_set_x_limits(blot_figure *fig,
				     double x_min, double x_max, GError **);
extern bool blot_figure_set_y_limits(blot_figure *fig,
				     double y_min, double y_max, GError **);

extern bool blot_figure_set_x_axis_labels(blot_figure *fig, size_t label_count,
					  char **x_labels, GError **);

/* add layers */

extern bool blot_figure_plot(blot_figure *fig,
		      blot_plot_type plot_type, blot_data_type data_type,
		      size_t data_count, const void *data_xs, const void *data_ys,
		      blot_color data_color, const char *data_label,
		      GError **error);

static inline bool blot_figure_scatter(blot_figure *fig, blot_data_type data_type,
				size_t data_count, const void *data_xs, const void *data_ys,
				blot_color data_color, const char *data_label,
				GError **error)
{
	return blot_figure_plot(fig, BLOT_SCATTER, data_type,
				data_count, data_xs, data_ys,
				data_color, data_label, error);
}

static inline bool blot_figure_line(blot_figure *fig, blot_data_type data_type,
				size_t data_count, const void *data_xs, const void *data_ys,
				blot_color data_color, const char *data_label,
				GError **error)
{
	return blot_figure_plot(fig, BLOT_LINE, data_type,
				data_count, data_xs, data_ys,
				data_color, data_label, error);
}

static inline bool blot_figure_bar(blot_figure *fig, blot_data_type data_type,
				   size_t data_count, const void *data_xs, const void *data_ys,
				   blot_color data_color, const char *data_label,
				   GError **error)
{
	return blot_figure_plot(fig, BLOT_BAR, data_type,
				data_count, data_xs, data_ys,
				data_color, data_label, error);
}

/* render */

extern struct blot_screen * blot_figure_render(blot_figure *fig,
					       blot_render_flags flags,
					       GError **);

