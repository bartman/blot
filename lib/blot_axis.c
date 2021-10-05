/* blot: internal object that calculates axis tick/text placement */
/* vim: set noet sw=8 ts=8 tw=80: */
#include <stdio.h>
#include <math.h>
#include <string.h>
#include "blot_axis.h"
#include "blot_error.h"
#include "blot_types.h"
#include "blot_utils.h"

#define BLOT_AXIS_LABEL_MAX 16

/* create/delete */

blot_axis * blot_axis_new(bool is_vertical, bool is_visible,
			  blot_color color,
			  unsigned screen_length,
			  double data_min, double data_max,
			  const blot_strv *labels,
			  GError **error)
{
	blot_axis *axs;

	/* special case for an axis that is not displayed */

	if (!is_visible) {

		axs = g_new0(blot_axis, 1);
		RETURN_ERROR(!axs, NULL, error, "new blot_axis");

		axs->is_vertical = is_vertical;
		axs->is_visible = is_visible;
		axs->color = color;
		axs->screen_length = screen_length;
		axs->data_min = data_min;
		axs->data_min = data_max;

		return axs;

	}

	/* figure out how many ticks we will have */

	unsigned tick_count;
	if (labels && labels->count) {
		tick_count = labels->count;

	} else {
		if (is_vertical)
			tick_count = screen_length / 3;
		else
			tick_count = screen_length / 10;

		// odd is better than even
		if (!(tick_count&1))
			tick_count ++;
	}

	/* compute spacing and offsets of the ticks */

	bool use_labels_arg = labels && labels->count;

	double data_range = data_max - data_min;

	// jump is the width of each tick in data and screen offsets
	double d_jump = data_range / tick_count;
	double s_jump = (double)(screen_length) / (tick_count);

	// base is a fudge factor to add to align it with 0, if needed
	double d_base = 0;
	double s_base = 0;

	if (!use_labels_arg && data_min < 0 && 0 < data_max) {

		/* compute an offset to force 0 to align on a tick location */

		double t_shift = -data_min / d_jump;
		unsigned ti = floor(t_shift);
		double d_shift = (ti * d_jump) + data_min;

		d_base = -d_shift;
		s_base = (t_shift-ti) * s_jump;
	}

	/* compute the size of the labels */

	size_t string_bytes = 0;
	if (!use_labels_arg)
		string_bytes = 128 + (BLOT_AXIS_LABEL_MAX+1) * tick_count;

	/* allocate the object */

	size_t total_size = sizeof(blot_axis)
		+ screen_length * sizeof(axs->entries[0])
		+ tick_count * sizeof(blot_axis_tick)
		+ string_bytes;

	axs = g_malloc(total_size);
	RETURN_ERROR(!axs, NULL, error, "new blot_axis(%u,%u,%zu",
		     screen_length, tick_count, string_bytes);

	/* populate the constants */

	axs->is_vertical = is_vertical;
	axs->is_visible = is_visible;
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

	for (int ti=0; ti<tick_count; ti++) {

		double   d_val = d_base + (ti * d_jump) + data_min;
		unsigned s_val = s_base + (ti * s_jump);

		char *label = NULL;
		if (use_labels_arg) {
			unsigned li = round(d_val);
			if (li<labels->count)
				label = labels->strings[li];
		} else {
			size_t room = min_t(size_t, end-p, BLOT_AXIS_LABEL_MAX);
			if (!is_vertical)
				room = min_t(size_t, end-p, floor(s_jump));
			int rc;
			const char **ff, *formats[] = {
				"%.3f", "%.2f", "%.1f", "%g", "%.3g", "%3.g", NULL };
			for (ff = formats; *ff; ff++) {
				rc = snprintf(p, (unsigned)room, *ff, d_val);
				if (rc>0 && rc < BLOT_AXIS_LABEL_MAX)
					break;
			}
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

		unsigned ei = round(s_val);
		if (ei<axs->screen_length)
			axs->entries[ei] = tck;
	}

	g_assert(p <= end);

	return axs;
}

void blot_axis_delete(blot_axis *axs)
{
	g_free(axs);
}

