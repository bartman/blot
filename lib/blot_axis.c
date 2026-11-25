/* blot: internal object that calculates axis tick/text placement */
/* vim: set noet sw=8 ts=8 tw=120: */
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <limits.h>
#include "blot_axis.h"
#include "blot_error.h"
#include "blot_layer.h"
#include "blot_types.h"
#include "blot_utils.h"
#include "blot_text.h"

/* Where the ticks go: tick ti is at about the value d_base + ti*d_jump +
 * data_min and the screen offset s_base + ti*s_jump; blot_axis_place_tick()
 * gives its exact value and cell. */
struct blot_axis_layout {
	unsigned count;
	double d_jump, s_jump;
	double d_base, s_base;
	double data_min, data_max;
	unsigned screen_length;
	unsigned y_room;	/* vertical: the room of every number label */
};

static void blot_axis_layout_init(struct blot_axis_layout *lay, unsigned tick_count,
				  unsigned screen_length, double data_min, double data_max,
				  bool align_zero)
{
	lay->count = tick_count;
	lay->data_min = data_min;
	lay->data_max = data_max;
	lay->screen_length = screen_length;

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

/* The cell a data point at v is drawn in; screen_length (off the axis)
 * if it isn't drawn. */
static unsigned blot_axis_cell_of(const struct blot_axis_layout *lay, double v)
{
	unsigned cell;
	if (!blot_layer_value_cell(v, lay->data_min, lay->data_max, lay->screen_length, &cell))
		return lay->screen_length;
	return cell;
}

/* The lowest value in cell `cell`, which v is just below (a rounding error
 * away), or v if there is no such value that close. */
static double blot_axis_raise_into_cell(const struct blot_axis_layout *lay, double v, unsigned cell)
{
	double width = (lay->data_max - lay->data_min) / lay->screen_length;
	double lo = v;
	double hi = fmin(lay->data_min + (cell + 0.5) * width, lay->data_max);

	if (blot_axis_cell_of(lay, hi) != cell)
		return v;

	/* lo is below the cell, hi is in it: halve the gap until they are
	 * neighbouring doubles */
	for (;;) {
		double mid = lo + (hi - lo) / 2;
		if (!(lo < mid && mid < hi))
			break;
		if (blot_axis_cell_of(lay, mid) == cell)
			hi = mid;
		else
			lo = mid;
	}

	/* a rounding error is far less than a millionth of a cell */
	return hi - v <= width * 1e-6 ? hi : v;
}

/* Tick ti: its value, within the limits, and its cell, which is the cell
 * blot_layer_value_cell() draws a data point with that value in, so a
 * point on a tick's value is drawn on the tick (#31).
 *
 * Its value and its screen offset are two roundings of one position, which
 * is often exactly on the lower edge of a cell: when the ticks are a whole
 * number of cells apart (5/7 on 21 cells over 0..1 is at 15), and for the
 * zero-aligned 0.  Either rounding can land just below that edge.  The
 * cell truncated from the offset was then one below its value's (0 at
 * 36.99999999999999 on 74 cells over -1..1), and the value's own cell
 * would leave the ticks unevenly spaced (0.71428571428571419 in cell 14,
 * and some of -1..2 on 111 cells, at 1, 4, 7.., in 3, 6..).  So when the
 * offset is on an edge, to within a rounding error, and the value is in
 * the cell below it, the value is raised to the lowest value of the cell
 * above, which differs from it by that rounding error alone: its label
 * stays the same.  Not 0, the zero-aligned tick: it is exact and must read
 * 0 (not 9e-16), so it is where 0 is drawn (row 49 of 83 over -10..6.6,
 * where 830 / 16.6 is 49.99999999999999). */
static double blot_axis_place_tick(const struct blot_axis_layout *lay, unsigned ti, unsigned *cell)
{
	double v = lay->d_base + (ti * lay->d_jump) + lay->data_min;

	/* the zero alignment can leave tick 0 an ulp below data_min */
	v = fmin(fmax(v, lay->data_min), lay->data_max);

	unsigned c = blot_axis_cell_of(lay, v);
	double s = lay->s_base + (ti * lay->s_jump);
	double edge = nearbyint(s);

	if (v != 0 && fabs(s - edge) < 1e-9 && c + 1 == edge) {
		v = blot_axis_raise_into_cell(lay, v, c + 1);
		c = blot_axis_cell_of(lay, v);
	}

	if (cell)
		*cell = c;
	return v;
}

static double blot_axis_tick_value(const struct blot_axis_layout *lay, unsigned ti)
{
	return blot_axis_place_tick(lay, ti, NULL);
}

static unsigned blot_axis_tick_cell(const struct blot_axis_layout *lay, unsigned ti)
{
	unsigned cell;
	blot_axis_place_tick(lay, ti, &cell);
	return cell;
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
	/* Y labels each have a row of the margin */
	if (lay->y_room)
		return lay->y_room;

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

/* The text of number label d_val in room: the formatter's, with the
 * precision the ticks need, within half a cell (half_cell) of the value
 * where the room allows (0.0012, not 0.001, for 0.0012 with a cell of
 * 2e-5; 1.4e-09, not 0.000, for 1.4e-9: #10). */
static int blot_axis_format(char *text, unsigned room, double d_val, double half_cell)
{
	return blot_format_number_within(text, room, d_val, half_cell);
}

/* The text of d_val in room, if it is a good label: one that fits, and
 * that names a value within half a cell (half_cell) of the tick (not 0.02
 * for 0.018036 in 4 columns, nor 2e+09 for 2.4e9).  When even the text
 * with all the room it could want is further off (a value that needs more
 * than 16 characters), the text may be no further off than that. */
static bool blot_axis_good_text(char *text, unsigned room, double d_val, double half_cell)
{
	if (blot_axis_format(text, room, d_val, half_cell) < 0)
		return false;

	if (!(fabs(strtod(text, NULL) - d_val) > half_cell))
		return true;

	/* the full text is off too: the shorter one may be no further off */
	char full[BLOT_AXIS_LABEL_MAX + 1];
	if (blot_axis_format(full, sizeof(full), d_val, half_cell) < 0)
		return true;
	return fabs(strtod(text, NULL) - d_val) <= fabs(strtod(full, NULL) - d_val) * (1 + 1e-9);
}

int blot_axis_number_label(const blot_axis *axs, char *text, unsigned room, double d_val)
{
	double half_cell = 0.5 * (axs->data_max - axs->data_min) / axs->screen_length;
	if (!blot_axis_good_text(text, room, d_val, half_cell))
		return -1;
	return strlen(text);
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
		double d_val = blot_axis_tick_value(lay, ti);
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
			int ra = blot_axis_format(a, sizeof(a), blot_axis_tick_value(lay, ti),
						  half_cell);
			int rb = blot_axis_format(b, sizeof(b), blot_axis_tick_value(lay, tj),
						  half_cell);
			if (ra >= 0 && rb >= 0 && strcmp(a, b))
				return BLOT_AXIS_DOES_NOT_FIT;
		}
	}
#undef TEXT

	return last_fits ? BLOT_AXIS_FITS : BLOT_AXIS_LAST_DOES_NOT_FIT;
}

/* The bytes of the longest start of a UTF-8 label that takes at most n
 * columns, wide characters two in every locale (#18). */
static size_t blot_axis_label_prefix(const char *label, size_t n)
{
	return blot_text_prefix(label, (unsigned)min_t(size_t, n, UINT_MAX), NULL);
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
 * (2024-01-15 and 2024-01-16 both 2024-01): the smallest step from there
 * whose labels aren't is taken.  With a step of the label count only the
 * first label has a tick, which can't be ambiguous; a '~' marks it when
 * even that one is cut, longer than the axis. */
static struct blot_axis_custom blot_axis_custom_layout(const struct blot_axis_layout *lay,
						       const blot_strv *labels,
						       unsigned screen_length)
{
	struct blot_axis_custom cus = { .step = 1, .mark_cuts = false };

	while (cus.step < lay->count && floor(cus.step * lay->s_jump) < 2)
		cus.step ++;

	while (cus.step < lay->count &&
	       !blot_axis_custom_distinct(lay, labels, cus.step, screen_length))
		cus.step ++;

	if (cus.step >= lay->count && labels->strings[0]) {
		unsigned room = blot_axis_custom_room(lay, 0, cus.step, screen_length);
		cus.mark_cuts = blot_axis_cut_label(NULL, labels->strings[0], room, false)
			< strlen(labels->strings[0]);
	}
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
	return blot_axis_new_with_room(is_vertical, is_visible, color, screen_length,
				       data_min, data_max, labels, BLOT_AXIS_Y_LABEL_COLS, error);
}

blot_axis * blot_axis_new_with_room(bool is_vertical, bool is_visible,
				    blot_color color,
				    unsigned screen_length,
				    double data_min, double data_max,
				    const blot_strv *labels,
				    unsigned y_label_cols,
				    GError **error)
{
	RETURN_ERRORx(screen_length == 0, NULL, error, EINVAL, "screen_length cannot be zero");
	/* NaN or inf limits (EINVAL), or limits so far apart that
	 * data_max - data_min is inf (ERANGE), would make NaN or inf tick
	 * values and labels (#6); the same errors as the figure's setters */
	RETURN_ERRORx(!isfinite(data_min) || !isfinite(data_max),
		      NULL, error, EINVAL, "axis limits %g..%g must be finite", data_min, data_max);
	RETURN_ERRORx(data_min >= data_max, NULL, error, EINVAL, "data_min must be less than data_max");
	RETURN_ERRORx(!isfinite(data_max - data_min),
		      NULL, error, ERANGE, "axis limits %g..%g are further apart than the largest double",
		      data_min, data_max);

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
	lay.y_room = is_vertical ? min_t(unsigned, y_label_cols, BLOT_AXIS_Y_LABEL_COLS) + 1 : 0;

	/* Number labels must say the value of their own tick to within half
	 * a cell, in their room: horizontal ones are written side by side, so
	 * they must fit between the ticks; vertical ones each have a row of
	 * the margin, y_label_cols wide (#10).  When the zero alignment left
	 * the last label too little room for that, drop that one tick: the
	 * others keep their place.  When other labels don't fit, use fewer
	 * ticks.  One horizontal tick at the start of the axis always has
	 * room (BLOT_MIN_COLS); on a tiny axis or margin a label that still
	 * doesn't fit is left empty, never a misleading number, and the render
	 * never fails on it. */

	if (!use_labels_arg) {
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

		unsigned s_val;
		double   d_val = blot_axis_place_tick(&lay, ti, &s_val);

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
			/* the layout above made sure it is good (the last
			 * resort for a tiny axis is no text) */
			unsigned room = !is_vertical && use_labels_arg
				? min_t(unsigned, BLOT_AXIS_LABEL_MAX + 1,
					blot_axis_custom_room(&lay, ti, cus.step, screen_length))
				: blot_axis_number_room(&lay, ti, screen_length);
			double half_cell = 0.5 * (data_max - data_min) / screen_length;
			int rc = blot_axis_good_text(p, room, d_val, half_cell) ? (int)strlen(p) : -1;
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
