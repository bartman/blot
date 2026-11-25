/* blot: figure represents the top level object */
/* vim: set noet sw=8 ts=8 tw=120: */
#pragma once

#include <glib.h>
#include <stdbool.h>

#include "blot_compiler.h"
#include "blot_types.h"

/* The smallest plot area: the canvas each layer is drawn on, without the
 * axes and the legend.  The render never draws a smaller one (#17). */
#define BLOT_MIN_PLOT_COLS 10
#define BLOT_MIN_PLOT_ROWS 10

/* The smallest screen: blot_figure_set_screen_size() and
 * blot_terminal_set_size() reject a smaller one, and a terminal size from
 * COLUMNS/LINES or the tty is raised to it.  It renders with every flag:
 * - columns: the Y-axis margin takes at most half the screen (its labels
 *   are formatted again for the room there is), the plot the other half;
 * - rows: the X-axis takes 2 rows (the line and its labels) under the
 *   plot.  A legend gets only the rows the plot can spare, so at the
 *   minimum with the X-axis there is no room for one (#17). */
#define BLOT_MIN_COLS (2 * BLOT_MIN_PLOT_COLS)
#define BLOT_MIN_ROWS (BLOT_MIN_PLOT_ROWS + 2)
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

	blot_strv xlabels;	/* owned: see blot_figure_set_x_axis_labels() */

	/* layers */
	gsize layer_count;
	struct blot_layer **layers;

} blot_figure;

/* create/delete */

BLOT_EXTERN_C_START

BLOT_API bool blot_figure_init(blot_figure *fig, GError **);
BLOT_API blot_figure * blot_figure_new(GError **);
/* frees the layers and the X labels and leaves the figure without them;
 * may be called again, or followed by blot_figure_init() */
BLOT_API void blot_figure_cleanup(blot_figure *fig);
BLOT_API void blot_figure_delete(blot_figure *fig);

/* configure */

BLOT_API bool blot_figure_set_axis_color(blot_figure *fig, blot_color, GError **);

/* cols x rows from BLOT_MIN_COLS x BLOT_MIN_ROWS to BLOT_MAX_COLS x
 * BLOT_MAX_ROWS; EINVAL otherwise */
BLOT_API bool blot_figure_set_screen_size(blot_figure *fig,
					unsigned cols, unsigned rows, GError **);

BLOT_API bool blot_figure_set_x_limits(blot_figure *fig,
				     double x_min, double x_max, GError **);
BLOT_API bool blot_figure_set_y_limits(blot_figure *fig,
				     double y_min, double y_max, GError **);

/* BLOT_LABEL_TEXT: legend labels (data_label) and custom X labels are
 * UTF-8 in every locale, the C locale too (#18); they are not read in the
 * locale's own encoding.  Each byte that isn't valid UTF-8 (a Latin-1 'é'
 * from an ISO-8859-1 locale, say) is shown as one '?', and so is a control
 * character or a code point that isn't a printable character.  Their width
 * is counted as a UTF-8 terminal shows them: 2 columns for East Asian wide
 * and fullwidth characters and emoji, 0 for combining marks (at most 4
 * after each character are kept), in every locale.  The screen text is
 * UCS-4 wchar_t whatever the locale. */

/* The figure keeps a copy of the array and of the strings (NULL entries
 * stay NULL); the caller may free or reuse its own after the call.  A new
 * set replaces (and frees) the old copy, label_count=0 with x_labels=NULL
 * clears it, and blot_figure_cleanup() frees it.  An invalid call (a count
 * without an array, or an array without a count) fails with EINVAL and
 * leaves the current labels unchanged.  The strings are not
 * modified: x_labels is not const only to keep the C API unchanged.
 * Labels are UTF-8 text in every locale (BLOT_LABEL_TEXT above). */
BLOT_API bool blot_figure_set_x_axis_labels(blot_figure *fig, size_t label_count,
					  char **x_labels, GError **);

/* render */

BLOT_API struct blot_screen * blot_figure_render(blot_figure *fig,
					       blot_render_flags flags,
					       GError **);

/* add layers */

/* The layer keeps the data_xs and data_ys pointers and the data_label
 * pointer, not copies: the data and the label must stay valid, and
 * unchanged, until the last blot_figure_render() of this figure.  The
 * label is UTF-8 text in every locale (BLOT_LABEL_TEXT above). */
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

