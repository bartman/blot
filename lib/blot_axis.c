/* blot: internal object that calculates axis tick/text placement */
/* vim: set noet sw=8 ts=8 tw=80: */
#include <stdio.h>
#include <math.h>
#include "blot_axis.h"
#include "blot_error.h"
#include "blot_types.h"
#include "blot_utils.h"

#define BLOT_AXIS_LABEL_MAX 16

/* create/delete */

blot_axis * blot_axis_new(bool is_vertical, blot_color color,
			  unsigned screen_length,
			  double data_min, double data_max,
			  const blot_strv *labels,
			  GError **error)
{
	/* figure out how many ticks we will have */

	unsigned tick_count;
	if (labels && labels->count)
		tick_count = labels->count;
	else if (is_vertical)
		tick_count = screen_length / 5;
	else
		tick_count = screen_length / 10;

	/* compute the size of the labels */

	bool use_labels_arg = labels && labels->count;
	size_t string_bytes = 0;
	if (!use_labels_arg)
		string_bytes = 128 + (BLOT_AXIS_LABEL_MAX+1) * tick_count;

	/* allocate the object */

	blot_axis *axs;

	size_t total_size = sizeof(blot_axis)
		+ screen_length * sizeof(axs->entries[0])
		+ tick_count * sizeof(blot_axis_tick)
		+ string_bytes;

	axs = g_malloc(total_size);
	RETURN_ERROR(!axs, NULL, error, "new blot_axis");

	/* populate the constants */

	axs->is_vertical = is_vertical;
	axs->color = color;
	axs->screen_length = screen_length;
	axs->data_min = data_min;
	axs->data_min = data_max;

	memset(axs->entries, 0, screen_length * sizeof(axs->entries[0]));

	/* derive pointers to ticks and strings */

	blot_axis_tick *ticks = (void*)&axs->entries[screen_length];
	char *data = (void*)&ticks[tick_count];
	char *p = data;
	char *end = data + string_bytes;
	g_assert(((void*)axs + total_size) == (end));

	/* populate the entries */

	double data_range = data_max - data_min;

	double d_jump = data_range / tick_count;
	double s_jump = (double)(screen_length) / tick_count;

	for (int ti=0; ti<tick_count; ti++) {

		double   d_val = (ti * d_jump) + data_min;
		unsigned s_val = (ti * s_jump);

		char *label = NULL;
		if (use_labels_arg) {
			unsigned li = round(d_val);
			if (li<labels->count)
				label = labels->strings[li];
		} else {
			size_t room = min_t(size_t, end-p, BLOT_AXIS_LABEL_MAX);
			int rc = snprintf(p, (unsigned)room,
					  "%.3f", d_val);
			RETURN_ERROR(rc<0, NULL, error,
				     "axis label for %f format", d_val);
			g_assert_cmpuint(rc, <=, BLOT_AXIS_LABEL_MAX);
			label = p;
			p += rc + 1;
			g_assert(p < end);
		}

		blot_axis_tick *tck = ticks + ti;
		tck->value = d_val;
		tck->label = label;

		axs->entries[s_val] = tck;
	}

	g_assert(p <= end);

	return axs;
}

void blot_axis_delete(blot_axis *axs)
{
	g_free(axs);
}

