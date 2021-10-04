/* blot: internal object that calculates axis tick/text placement */
/* vim: set noet sw=8 ts=8 tw=80: */
#pragma once

#include <glib.h>
#include "blot_types.h"
#include "blot_error.h"

typedef struct blot_axis_tick {
	double value;
	char *label;
} blot_axis_tick;

typedef struct blot_axis {
	bool is_vertical;
	bool is_visible;
	blot_color color;
	unsigned screen_length;
	double data_min, data_max;

	blot_axis_tick *entries[0]; // must be last
} blot_axis;

/* create/delete */

extern blot_axis * blot_axis_new(bool is_vertical,
				 bool is_visible,
				 blot_color color,
				 unsigned screen_length,
				 double data_min, double data_max,
				 const blot_strv *labels,
				 GError **);
extern void blot_axis_delete(blot_axis *axs);

/* access */

static inline const blot_axis_tick * blot_axis_get_tick_at(const blot_axis *axs,
						   unsigned screen_index,
						   GError **error)
{
	RETURN_EFAULT_IF(axs==NULL, NULL, error);

	if (unlikely (!axs->is_visible || screen_index >= axs->screen_length))
		return NULL;

	return axs->entries[screen_index];
}

