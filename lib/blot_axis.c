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

/* The room blot_format_number() takes for a horizontal label at a tick in
 * screen cell `cell`: the columns it may use, plus one.  A label ends one
 * blank column before the next labelled tick (next_cell), or at the edge of
 * the axis when there is none (next_cell >= screen_length). */
static unsigned blot_axis_room_to(unsigned cell, unsigned next_cell, unsigned screen_length)
{
	if (next_cell < screen_length)
		return next_cell > cell ? next_cell - cell : 0;
	return cell < screen_length ? screen_length - cell + 1 : 0;
}

/* The room of number label ti: also no more than the tick spacing, rounded
 * down (as always), and never more than BLOT_AXIS_LABEL_MAX characters. */
static unsigned blot_axis_number_room(const struct blot_axis_layout *lay, unsigned ti,
				      unsigned screen_length)
{
	unsigned cell = blot_axis_tick_cell(lay, ti);
	unsigned next = ti + 1 < lay->count ? blot_axis_tick_cell(lay, ti + 1) : screen_length;
	unsigned room = blot_axis_room_to(cell, next, screen_length);

	room = min_t(unsigned, room, floor(lay->s_jump));
	return min_t(unsigned, room, BLOT_AXIS_LABEL_MAX + 1);
}

/* How well the horizontal number labels of a layout fit. */
enum blot_axis_fit {
	BLOT_AXIS_FITS,
	BLOT_AXIS_LAST_DOES_NOT_FIT,	/* only the last label has no good text */
	BLOT_AXIS_DOES_NOT_FIT,
};

/* The text of d_val in room, if it is a good label: one that fits, and
 * that doesn't name a value more than half a cell (half_cell) away from
 * the tick when all the room it could want would have done better (0.02
 * for 0.018036 in 4 columns, 2e+09 for 2.4e9).  A text that is off even
 * with all the room, like 0.000 for 1e-9, is the formatter's business. */
static bool blot_axis_good_text(char *text, unsigned room, double d_val, double half_cell)
{
	if (blot_format_number(text, room, d_val) < 0)
		return false;

	if (!(fabs(strtod(text, NULL) - d_val) > half_cell))
		return true;

	char full[BLOT_AXIS_LABEL_MAX + 1];
	if (blot_format_number(full, sizeof(full), d_val) < 0)
		return true;
	return !(fabs(strtod(full, NULL) - d_val) <= half_cell);
}

/* Every label must have a good text, and no two may be the same text only
 * because the room made them shorter (-2e+05 for both -200100 and -166700).
 * Labels that are the same even with all the room they could want are the
 * formatter's business, not the layout's. */
static enum blot_axis_fit blot_axis_numbers_fit(const struct blot_axis_layout *lay,
						unsigned screen_length,
						double data_min, double data_max)
{
	double half_cell = 0.5 * (data_max - data_min) / screen_length;
	g_autofree char *text = g_malloc((size_t)lay->count * (BLOT_AXIS_LABEL_MAX + 1));
	bool last_fits = true;

#define TEXT(ti) (text + (size_t)(ti) * (BLOT_AXIS_LABEL_MAX + 1))
	for (unsigned ti=0; ti<lay->count; ti++) {
		double d_val = blot_axis_tick_value(lay, ti, data_min);
		unsigned room = blot_axis_number_room(lay, ti, screen_length);
		if (blot_axis_good_text(TEXT(ti), room, d_val, half_cell))
			continue;
		if (ti + 1 < lay->count)
			return BLOT_AXIS_DOES_NOT_FIT;
		last_fits = false;
	}

	unsigned checked = last_fits ? lay->count : lay->count - 1;
	for (unsigned ti=0; ti<checked; ti++) {
		for (unsigned tj=ti+1; tj<checked; tj++) {
			if (strcmp(TEXT(ti), TEXT(tj)))
				continue;

			char a[BLOT_AXIS_LABEL_MAX + 1], b[BLOT_AXIS_LABEL_MAX + 1];
			int ra = blot_format_number(a, sizeof(a), blot_axis_tick_value(lay, ti, data_min));
			int rb = blot_format_number(b, sizeof(b), blot_axis_tick_value(lay, tj, data_min));
			if (ra >= 0 && rb >= 0 && strcmp(a, b))
				return BLOT_AXIS_DOES_NOT_FIT;
		}
	}
#undef TEXT

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

/* Custom X labels: which get a tick, and how they are cut. */
struct blot_axis_custom {
	unsigned step;		/* every step'th label gets a tick */
	bool mark_cuts;		/* a cut label ends in '~' */
};

#define BLOT_AXIS_CUT_MARK '~'

/* The room of custom label ti (every step'th has a tick): the columns up
 * to the next labelled tick, less a blank, or to the edge; whole labels
 * when they fit, no BLOT_AXIS_LABEL_MAX. */
static unsigned blot_axis_custom_room(const struct blot_axis_layout *lay, unsigned ti,
				      unsigned step, unsigned screen_length)
{
	unsigned cell = blot_axis_tick_cell(lay, ti);
	unsigned next = ti + step < lay->count ? blot_axis_tick_cell(lay, ti + step) : screen_length;
	return blot_axis_room_to(cell, next, screen_length);
}

/* Cut a custom label to room-1 columns into out (NULL: only measure);
 * returns the bytes written, without the NUL.  A cut label that is marked
 * ends in BLOT_AXIS_CUT_MARK, which takes one of the columns. */
static size_t blot_axis_cut_label(char *out, const char *label, unsigned room, bool mark)
{
	size_t cols = room ? room - 1 : 0;
	size_t bytes = blot_axis_label_prefix(label, cols);
	bool cut = bytes < strlen(label);

	if (cut && mark && cols)
		bytes = blot_axis_label_prefix(label, cols - 1);
	if (out) {
		memcpy(out, label, bytes);
		if (cut && mark && cols)
			out[bytes++] = BLOT_AXIS_CUT_MARK;
		out[bytes] = 0;
	} else if (cut && mark && cols) {
		bytes ++;
	}
	return bytes;
}

/* A cut label is ambiguous when another label starts with the same text
 * (2024-01 for 2024-01-15 next to 2024-01-16, or 2024-01 next to a whole
 * 2024-01-): the reader can't tell which it is.  With this step, is every
 * cut label that gets a tick unambiguous among those that do?  Labels that
 * are the same text in full don't count. */
static bool blot_axis_custom_distinct(const struct blot_axis_layout *lay, const blot_strv *labels,
				      unsigned step, unsigned screen_length)
{
	for (unsigned ti=0; ti<lay->count; ti+=step) {
		const char *a = labels->strings[ti];
		if (!a)
			continue;
		unsigned ra = blot_axis_custom_room(lay, ti, step, screen_length);
		size_t na = blot_axis_cut_label(NULL, a, ra, false);
		if (na == strlen(a))
			continue;

		for (unsigned tj=0; tj<lay->count; tj+=step) {
			const char *b = labels->strings[tj];
			if (tj == ti || !b || !strcmp(a, b))
				continue;
			if (!strncmp(a, b, na))
				return false;
		}
	}
	return true;
}

/* Custom labels each get a tick, unless there are so many that the ticks
 * would be closer than two columns (one character and a blank): then
 * only every step'th label gets one, starting with the first, so the ticks
 * are evenly spaced again.  Cut labels must not become ambiguous
 * (2024-01-15 and 2024-01-16 both 2024-01): the step is raised, up to
 * twice that, until they aren't; if that is not enough, the first step is
 * kept and every cut label is marked with a '~'. */
static struct blot_axis_custom blot_axis_custom_layout(const struct blot_axis_layout *lay,
						       const blot_strv *labels,
						       unsigned screen_length)
{
	struct blot_axis_custom cus = { .step = 1, .mark_cuts = false };

	while (cus.step < lay->count && floor(cus.step * lay->s_jump) < 2)
		cus.step ++;

	unsigned base = cus.step;
	for (unsigned step=base; step<=2*base && step<=lay->count; step++) {
		if (blot_axis_custom_distinct(lay, labels, step, screen_length)) {
			cus.step = step;
			return cus;
		}
	}

	cus.mark_cuts = true;
	return cus;
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
	 * fit between the ticks, and say the value of their own tick to
	 * within half a cell.  When the zero alignment left the last label
	 * too little room for that, drop that one tick: the others keep their
	 * place.  When other labels don't fit, use fewer ticks.  One tick at
	 * the start of the axis always has room (BLOT_MIN_COLS); on a tiny
	 * axis a label that still doesn't fit is left empty, the render never
	 * fails on it. */

	if (!is_vertical && !use_labels_arg) {
		for (;;) {
			enum blot_axis_fit fit = blot_axis_numbers_fit(&lay, screen_length,
								       data_min, data_max);
			if (fit == BLOT_AXIS_FITS || lay.count == 1)
				break;
			if (fit == BLOT_AXIS_LAST_DOES_NOT_FIT)
				lay.count --;
			else
				blot_axis_layout_init(&lay, lay.count - 1, screen_length,
						      data_min, data_max, true);
		}

		if (blot_axis_numbers_fit(&lay, screen_length, data_min, data_max) != BLOT_AXIS_FITS)
			blot_axis_layout_init(&lay, 1, screen_length,
					      data_min, data_max, false);
	}

	struct blot_axis_custom cus = { .step = 1, .mark_cuts = false };
	if (use_labels_arg && !is_vertical)
		cus = blot_axis_custom_layout(&lay, labels, screen_length);

	/* compute the size of the labels */

	/* numbers, or the copies of the custom labels cut to their room
	 * (and a cut mark) */
	size_t string_bytes = 128 + (BLOT_AXIS_LABEL_MAX+1) * tick_count;
	if (use_labels_arg) {
		for (unsigned li=0; li<labels->count; li++)
			if (labels->strings[li])
				string_bytes += strlen(labels->strings[li]) + 1;
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

	/* ticks without a label (dropped, or skipped by the step) stay zero */
	memset(ticks, 0, tick_count * sizeof(*ticks));

	/* populate the entries */

	for (unsigned ti=0; ti<lay.count; ti+=cus.step) {

		double   d_val = blot_axis_tick_value(&lay, ti, data_min);
		unsigned s_val = blot_axis_tick_cell(&lay, ti);

		const char *custom = use_labels_arg ? labels->strings[ti] : NULL;
		char *label = p;

		if (custom && is_vertical) {
			/* the screen cuts Y labels to its margin */
			label = (char *)custom;
		} else if (custom) {
			/* cut, never dropped */
			unsigned room = blot_axis_custom_room(&lay, ti, cus.step, screen_length);
			p += blot_axis_cut_label(p, custom, room, cus.mark_cuts) + 1;
		} else {
			/* the layout above made sure it fits (the last
			 * resort for a tiny axis is no text) */
			unsigned room = BLOT_AXIS_LABEL_MAX;
			if (!is_vertical)
				room = use_labels_arg
					? min_t(unsigned, BLOT_AXIS_LABEL_MAX + 1,
						blot_axis_custom_room(&lay, ti, cus.step, screen_length))
					: blot_axis_number_room(&lay, ti, screen_length);
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
