/* blot: a layer contains the raw data to be plotted */
/* vim: set noet sw=8 ts=8 tw=80: */
#pragma once

#include <glib.h>
#include "blot_types.h"
#include "blot_error.h"

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

/* data */

static inline bool blot_layer_get_x(const blot_layer *lay, unsigned index,
				    double *x, GError **error)
{
	RETURN_ERRORx(index > lay->count, false, error, ERANGE,
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
	RETURN_ERRORx(index > lay->count, false, error, ERANGE,
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

extern bool blot_layer_get_lim(const blot_layer *lay, blot_xy_limits *lim,
			       GError **);

/* render */

extern struct blot_canvas * blot_layer_render(blot_layer *lay,
					      const blot_xy_limits *lim,
					      const blot_dimensions *dim,
					      blot_render_flags flags,
					      GError **);


