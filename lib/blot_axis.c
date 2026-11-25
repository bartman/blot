/* blot: internal object that calculates axis tick/text placement */
/* vim: set noet sw=8 ts=8 tw=120: */
#include <stdio.h>
#include <math.h>
#include <string.h>
#include "blot_axis.h"
#include "blot_error.h"
#include "blot_types.h"
#include "blot_utils.h"

#define BLOT_AXIS_LABEL_MAX 16

/* Where the ticks go: tick ti has the value d_base + ti*d_jump + data_min
 * and sits in screen cell (unsigned)(s_base + ti*s_jump). */
struct blot_axis_layout {
	unsigned count;
	double d_jump, s_jump;
	double d_base, s_base;
};

static void blot_axis_layout_init(struct blot_axis_layout *lay, unsigned tick_count,
				  unsigned screen_length, double data_min, double data_max,
				  bool align_zero)
{
	lay->count = tick_count;

	// jump is the width of each tick in data and screen offsets
	lay->d_jump = (data_max - data_min) / tick_count;
	lay->s_jump = (double)(screen_length) / (tick_count);

	// base is a fudge factor to add to align it with 0, if needed
	lay->d_base = 0;
	lay->s_base = 0;

	if (align_zero && data_min < 0 && 0 < data_max) {

		/* compute an offset to force 0 to align on a tick location;
		 * it moves every tick right by less than one tick spacing,
		 * which leaves the last label less room (see the room below) */

		double t_shift = -data_min / lay->d_jump;
		unsigned ti = floor(t_shift);
		double d_shift = (ti * lay->d_jump) + data_min;

		lay->d_base = -d_shift;
		lay->s_base = (t_shift-ti) * lay->s_jump;
	}
}

static double blot_axis_tick_value(const struct blot_axis_layout *lay, unsigned ti, double data_min)
{
	return lay->d_base + (ti * lay->d_jump) + data_min;
}

static unsigned blot_axis_tick_cell(const struct blot_axis_layout *lay, unsigned ti)
{
	return lay->s_base + (ti * lay->s_jump);
}

/* The columns a horizontal label at tick ti may use, plus one: the room
 * blot_format_number() takes.  A label is followed by at least one blank
 * column before the next tick (the tick spacing, rounded down, as always),
 * and the last one ends at the edge of the axis.  No label is longer than
 * BLOT_AXIS_LABEL_MAX. */
static unsigned blot_axis_label_room(const struct blot_axis_layout *lay, unsigned ti,
				     unsigned screen_length)
{
	unsigned room = min_t(unsigned, floor(lay->s_jump), BLOT_AXIS_LABEL_MAX + 1);
	unsigned cell = blot_axis_tick_cell(lay, ti);

	if (ti + 1 < lay->count) {
		unsigned next = blot_axis_tick_cell(lay, ti + 1);
		room = min_t(unsigned, room, next > cell ? next - cell : 0);
	} else {
		room = min_t(unsigned, room, cell < screen_length ? screen_length - cell + 1 : 0);
	}
	return room;
}

/* How well the horizontal number labels of a layout fit. */
enum blot_axis_fit {
	BLOT_AXIS_FITS,
	BLOT_AXIS_LAST_DOES_NOT_FIT,	/* only the last label has no text that fits */
	BLOT_AXIS_DOES_NOT_FIT,
};

/* Every label must fit its room, and no two may be the same text only
 * because the room made them shorter (-2e+05 for both -200100 and -166700).
 * Labels that are the same even with all the room they could want are the
 * formatter's business, not the layout's. */
static enum blot_axis_fit blot_axis_numbers_fit(const struct blot_axis_layout *lay,
						unsigned screen_length, double data_min)
{
	char text[lay->count][BLOT_AXIS_LABEL_MAX + 1];
	bool last_fits = true;

	for (unsigned ti=0; ti<lay->count; ti++) {
		double d_val = blot_axis_tick_value(lay, ti, data_min);
		unsigned room = blot_axis_label_room(lay, ti, screen_length);
		if (blot_format_number(text[ti], room, d_val) >= 0)
			continue;
		if (ti + 1 < lay->count)
			return BLOT_AXIS_DOES_NOT_FIT;
		last_fits = false;
	}

	unsigned checked = last_fits ? lay->count : lay->count - 1;
	for (unsigned ti=0; ti<checked; ti++) {
		for (unsigned tj=ti+1; tj<checked; tj++) {
			if (strcmp(text[ti], text[tj]))
				continue;

			char a[BLOT_AXIS_LABEL_MAX + 1], b[BLOT_AXIS_LABEL_MAX + 1];
			int ra = blot_format_number(a, sizeof(a), blot_axis_tick_value(lay, ti, data_min));
			int rb = blot_format_number(b, sizeof(b), blot_axis_tick_value(lay, tj, data_min));
			if (ra >= 0 && rb >= 0 && strcmp(a, b))
				return BLOT_AXIS_DOES_NOT_FIT;
		}
	}

	return last_fits ? BLOT_AXIS_FITS : BLOT_AXIS_LAST_DOES_NOT_FIT;
}

/* The first n characters of a UTF-8 label (n bytes if it isn't UTF-8);
 * returns the bytes they take. */
static size_t blot_axis_label_prefix(const char *label, size_t n)
{
	size_t bytes = strlen(label);

	if (!g_utf8_validate(label, bytes, NULL))
		return min_t(size_t, n, bytes);

	if ((size_t)g_utf8_strlen(label, bytes) <= n)
		return bytes;
	return g_utf8_offset_to_pointer(label, n) - label;
}

/* create/delete */

blot_axis * blot_axis_new(bool is_vertical, bool is_visible,
			  blot_color color,
			  unsigned screen_length,
			  double data_min, double data_max,
			  const blot_strv *labels,
			  GError **error)
{
	RETURN_ERRORx(screen_length == 0, NULL, error, EINVAL, "screen_length cannot be zero");
	RETURN_ERRORx(data_min >= data_max, NULL, error, EINVAL, "data_min must be less than data_max");

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
		axs->data_max = data_max;

		return axs;

	}

	/* figure out how many ticks we will have */

	bool use_labels_arg = labels && labels->count;

	unsigned tick_count;
	if (use_labels_arg) {
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

	struct blot_axis_layout lay;
	blot_axis_layout_init(&lay, tick_count, screen_length, data_min, data_max,
			      !use_labels_arg);

	/* Horizontal number labels are written side by side, so they must
	 * fit between the ticks.  When the zero alignment left the last label
	 * too little room, drop that one tick: the others keep their place.
	 * When other labels don't fit, use fewer ticks.  One tick at the start
	 * of the axis always has room (BLOT_MIN_COLS); on a tiny axis a label
	 * that still doesn't fit is left empty, the render never fails on it. */

	if (!is_vertical && !use_labels_arg) {
		for (;;) {
			enum blot_axis_fit fit = blot_axis_numbers_fit(&lay, screen_length, data_min);
			if (fit == BLOT_AXIS_FITS || lay.count == 1)
				break;
			if (fit == BLOT_AXIS_LAST_DOES_NOT_FIT)
				lay.count --;
			else
				blot_axis_layout_init(&lay, lay.count - 1, screen_length,
						      data_min, data_max, true);
		}

		if (blot_axis_numbers_fit(&lay, screen_length, data_min) != BLOT_AXIS_FITS)
			blot_axis_layout_init(&lay, 1, screen_length,
					      data_min, data_max, false);
	}

	/* Custom labels each get a tick, unless there are so many that the
	 * ticks would be closer than two columns (one character and a blank):
	 * then only every label_step'th label gets one, starting with the
	 * first, so the ticks are evenly spaced again. */

	unsigned label_step = 1;
	if (use_labels_arg && !is_vertical) {
		while (label_step < lay.count && floor(label_step * lay.s_jump) < 2)
			label_step ++;
	}

	/* compute the size of the labels */

	/* numbers, or the copies of the custom labels cut to their room */
	size_t string_bytes = 128 + (BLOT_AXIS_LABEL_MAX+1) * tick_count;
	if (use_labels_arg) {
		for (unsigned li=0; li<labels->count; li++)
			if (labels->strings[li])
				string_bytes += strlen(labels->strings[li]);
	}

	/* allocate the object */

	size_t total_size = sizeof(blot_axis)
		+ screen_length * sizeof(axs->entries[0]) // NOLINT - we want the sizeof a pinter
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
	axs->data_max = data_max;

	memset(axs->entries, 0, screen_length * sizeof(axs->entries[0])); // NOLINT - we want the sizeof a pointer

	/* derive pointers to ticks and strings */

	blot_axis_tick *ticks = (void*)&axs->entries[screen_length];
	char *data = (void*)&ticks[tick_count];
	char *p = data;
	char *end = data + string_bytes;
	g_assert(((void*)axs + total_size) == (end));

	/* populate the entries */

	for (unsigned ti=0; ti<lay.count; ti+=label_step) {

		double   d_val = blot_axis_tick_value(&lay, ti, data_min);
		unsigned s_val = blot_axis_tick_cell(&lay, ti);

		/* the label never reaches the next labelled tick (less a blank
		 * column) or the end of the axis */
		unsigned room = BLOT_AXIS_LABEL_MAX;
		if (!is_vertical) {
			struct blot_axis_layout every = lay;
			every.count = (lay.count + label_step - 1) / label_step;
			every.s_jump = lay.s_jump * label_step;
			room = blot_axis_label_room(&every, ti / label_step, screen_length);
		}

		const char *custom = use_labels_arg ? labels->strings[ti] : NULL;
		char *label = p;

		if (custom && is_vertical) {
			/* the screen cuts Y labels to its margin */
			label = (char *)custom;
		} else if (custom) {
			/* cut, never dropped: the first room-1 characters */
			size_t bytes = blot_axis_label_prefix(custom, room ? room - 1 : 0);
			memcpy(p, custom, bytes);
			p[bytes] = 0;
			p += bytes + 1;
		} else {
			/* the layout above made sure it fits (the last
			 * resort for a tiny axis is no text) */
			int rc = blot_format_number(p, room, d_val);
			if (rc < 0)
				rc = 0;
			p[rc] = 0;
			g_assert_cmpuint(rc, <=, BLOT_AXIS_LABEL_MAX);
			p += rc + 1;
		}
		g_assert(p <= end);

		blot_axis_tick *tck = ticks + ti;
		tck->value = d_val;
		tck->label = label;

		if (s_val<axs->screen_length)
			axs->entries[s_val] = tck;
	}

	g_assert(p <= end);

	return axs;
}

void blot_axis_delete(blot_axis *axs)
{
	g_free(axs);
}
