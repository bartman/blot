/* blot: figure represents the top level object */
/* vim: set noet sw=8 ts=8 tw=120: */
#pragma once

#include <glib.h>
#include <stdbool.h>

#include "blot_compiler.h"
#include "blot_types.h"

#define BLOT_MIN_COLS 10
#define BLOT_MIN_ROWS 10
#define BLOT_MAX_COLS 1000
#define BLOT_MAX_ROWS 1000

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

BLOT_EXTERN_C_START

BLOT_API bool blot_figure_init(blot_figure *fig, GError **);
BLOT_API blot_figure * blot_figure_new(GError **);
BLOT_API void blot_figure_cleanup(blot_figure *fig);
BLOT_API void blot_figure_delete(blot_figure *fig);

/* configure */

BLOT_API bool blot_figure_set_axis_color(blot_figure *fig, blot_color, GError **);

BLOT_API bool blot_figure_set_screen_size(blot_figure *fig,
					unsigned cols, unsigned rows, GError **);

BLOT_API bool blot_figure_set_x_limits(blot_figure *fig,
				     double x_min, double x_max, GError **);
BLOT_API bool blot_figure_set_y_limits(blot_figure *fig,
				     double y_min, double y_max, GError **);

BLOT_API bool blot_figure_set_x_axis_labels(blot_figure *fig, size_t label_count,
					  char **x_labels, GError **);

/* render */

BLOT_API struct blot_screen * blot_figure_render(blot_figure *fig,
					       blot_render_flags flags,
					       GError **);

/* add layers */

BLOT_API bool blot_figure_plot(blot_figure *fig,
		      blot_plot_type plot_type, blot_data_type data_type,
		      size_t data_count, const void *data_xs, const void *data_ys,
		      blot_color data_color, const char *data_label,
		      GError **error);

BLOT_EXTERN_C_END

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

