/* blot: a layer contains the raw data to be plotted */
/* vim: set noet sw=8 ts=8 tw=120: */
#pragma once

#include <glib.h>
#include <math.h>
#include <stdbool.h>

#include "blot_compiler.h"
#include "blot_types.h"
#include "blot_error.h"

/* the points of a layer whose X and Y are both finite; the others
 * (layer count - summary count) are left out.  xttl and yttl are plain
 * sums and can overflow to +-inf; yttl_scaled is the sum of y * 2^-64,
 * which doesn't, see blot_layer_summary_yavg() */
typedef struct blot_layer_summary {
	double xmin, xmax, xttl;
	double ymin, ymax, yttl;
	size_t count;
	bool enabled; // when (render_flags & BLOT_RENDER_LEGEND_DETAILS)
	double yttl_scaled;
} blot_layer_summary;

#define BLOT_LAYER_SUMMARY_SCALE 0x1p-64

/* the mean of the summarized Y values, NAN when there are none.  It is
 * yttl / count while yttl is finite.  When the sum of the finite values
 * overflows, it comes from the scaled sum: scaling by a power of two is
 * exact, so that is the same sum with 64 more bits of exponent.  The
 * mean lies between ymin and ymax; rounding doesn't take it out */
static inline double blot_layer_summary_yavg(const blot_layer_summary *sum)
{
	double avg;

	if (!sum->count)
		return NAN;
	if (isfinite(sum->yttl))
		avg = sum->yttl / sum->count;
	else
		avg = sum->yttl_scaled / sum->count / BLOT_LAYER_SUMMARY_SCALE;
	if (!(avg >= sum->ymin))
		avg = sum->ymin;
	if (!(avg <= sum->ymax))
		avg = sum->ymax;
	return avg;
}

typedef struct blot_layer {

	blot_plot_type plot_type;
	blot_data_type data_type;
	size_t count;
	const void *xs;
	const void *ys;
	blot_color color;
	const char *label;
	blot_layer_summary summary;

} blot_layer;

/* create/delete */

BLOT_EXTERN_C_START

BLOT_API blot_layer * blot_layer_new(blot_plot_type plot_type,
				   blot_data_type data_type,
				   size_t data_count,
				   const void *data_xs,
				   const void *data_ys,
				   blot_color data_color,
				   const char *data_label,
				   GError **);
BLOT_API void blot_layer_delete(blot_layer *fig);

BLOT_EXTERN_C_END

/* data */

static inline bool blot_layer_get_x(const blot_layer *lay, unsigned index,
				    double *x, GError **error)
{
	RETURN_ERRORx(index >= lay->count, false, error, ERANGE,
		      "index %u is out of range %u", index, lay->count);

	if (unlikely (!lay->xs)) {
		/* X data can be NULL, that means we
		 * are plotting the index as X */
		*x = index;
		return true;
	}

	switch (lay->data_type & BLOT_DATA_X_MASK) {
	case BLOT_DATA_X_INT16:
		*x = ((const gint16 *)lay->xs)[index];
		break;
	case BLOT_DATA_X_INT32:
		*x = ((const gint32 *)lay->xs)[index];
		break;
	case BLOT_DATA_X_INT64:
		*x = ((const gint64 *)lay->xs)[index];
		break;
	case BLOT_DATA_X_FLOAT:
		*x = ((const float *)lay->xs)[index];
		break;
	case BLOT_DATA_X_DOUBLE:
		*x = ((const double *)lay->xs)[index];
		break;
	default:
		g_error("unexpected X of lay data_type == %02x", lay->data_type);
		return false;
	}

	return true;
}

static inline bool blot_layer_get_y(const blot_layer *lay, unsigned index,
				    double *y, GError **error)
{
	RETURN_ERRORx(!lay->ys, false, error, ENOENT, "Y-data is NULL");
	RETURN_ERRORx(index >= lay->count, false, error, ERANGE,
		      "index %u is out of range %u", index, lay->count);

	switch (lay->data_type & BLOT_DATA_Y_MASK) {
	case BLOT_DATA_Y_INT16:
		*y = ((const gint16 *)lay->ys)[index];
		break;
	case BLOT_DATA_Y_INT32:
		*y = ((const gint32 *)lay->ys)[index];
		break;
	case BLOT_DATA_Y_INT64:
		*y = ((const gint64 *)lay->ys)[index];
		break;
	case BLOT_DATA_Y_FLOAT:
		*y = ((const float *)lay->ys)[index];
		break;
	case BLOT_DATA_Y_DOUBLE:
		*y = ((const double *)lay->ys)[index];
		break;
	default:
		g_error("unexpected Y of lay data_type == %02x", lay->data_type);
		return false;
	}

	return true;
}

static inline bool blot_layer_get_x_y(const blot_layer *lay, unsigned index,
					 double *x, double *y, GError **error)
{
	bool ok;

	ok = blot_layer_get_x(lay, index, x, error);
	RETURN_IF(!ok, false);

	return blot_layer_get_y(lay, index, y, error);
}

BLOT_EXTERN_C_START

BLOT_API bool blot_layer_get_lim(const blot_layer *lay, blot_xy_limits *lim,
			       GError **);

/* render */

BLOT_API struct blot_canvas * blot_layer_render(blot_layer *lay,
					      const blot_xy_limits *lim,
					      const blot_dimensions *dim,
					      blot_render_flags flags,
					      GError **);

BLOT_EXTERN_C_END
