/* blot: internal object that calculates axis tick/text placement */
/* vim: set noet sw=8 ts=8 tw=120: */
#pragma once

#include <glib.h>
#include <stdbool.h>

#include "blot_compiler.h"
#include "blot_types.h"
#include "blot_error.h"

struct blot_axis_tick {
	double value;
	char *label;
};
typedef struct blot_axis_tick blot_axis_tick;

struct blot_axis {
	bool is_vertical;
	bool is_visible;
	blot_color color;
	unsigned screen_length;
	double data_min, data_max;

	blot_axis_tick *entries[0]; // must be last
};
typedef struct blot_axis blot_axis;

/* create/delete */

BLOT_EXTERN_C_START

BLOT_API blot_axis * blot_axis_new(bool is_vertical,
				 bool is_visible,
				 blot_color color,
				 unsigned screen_length,
				 double data_min, double data_max,
				 const blot_strv *labels,
				 GError **);
BLOT_API void blot_axis_delete(blot_axis *axs);

BLOT_EXTERN_C_END

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
