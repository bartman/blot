/* blot: internal object that calculates axis tick/text placement */
/* vim: set noet sw=8 ts=8 tw=120: */
#pragma once

#include <glib.h>
#include <stdbool.h>

#include "blot_compiler.h"
#include "blot_types.h"
#include "blot_error.h"

/* the most characters of a number label */
#define BLOT_AXIS_LABEL_MAX 16
/* ... and of a vertical one */
#define BLOT_AXIS_Y_LABEL_COLS (BLOT_AXIS_LABEL_MAX - 1)

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
/* blot_axis_new(), with vertical number labels of at most y_label_cols
 * characters (BLOT_AXIS_Y_LABEL_COLS at most): the ticks are fewer, or a
 * label is empty, rather than a label names a value further than half a
 * cell from its tick (#10) */
BLOT_API blot_axis * blot_axis_new_with_room(bool is_vertical,
					   bool is_visible,
					   blot_color color,
					   unsigned screen_length,
					   double data_min, double data_max,
					   const blot_strv *labels,
					   unsigned y_label_cols,
					   GError **);
BLOT_API void blot_axis_delete(blot_axis *axs);

/* The label of the number d_val on this axis in fewer than room
 * characters, by the axis's rule: within half a cell of d_val, or no
 * further off than with all the room it could want.  Returns the length,
 * or -1 when no text is good; nothing is written at or past text[room]. */
BLOT_API int blot_axis_number_label(const blot_axis *axs, char *text, unsigned room, double d_val);

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
