/* blot: screen represents the final render object */
/* vim: set noet sw=8 ts=8 tw=120: */
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <wctype.h>
#include "blot_screen.h"
#include "blot_error.h"
#include "blot_canvas.h"
#include "blot_layer.h"
#include "blot_color.h"
#include "blot_axis.h"
#include "blot_utils.h"
#include "blot_text.h"

/* create/delete */

blot_screen * blot_screen_new(const blot_dimensions *dim,
			      const blot_margins *mrg,
			      blot_render_flags flags, GError **error)
{
	RETURN_ERRORx(!dim, NULL, error, EFAULT, "dimensions pointer is NULL");
	RETURN_ERRORx(!mrg, NULL, error, EFAULT, "margins pointer is NULL");

	gsize char_len;
	bool overflow = __builtin_mul_overflow((gsize)dim->cols, (gsize)dim->rows, &char_len);

	RETURN_ERRORx(!overflow && !char_len, NULL, error, EINVAL, "cannot create a zero sized screen");

	// add extra space for screen clear escape code
	if (flags & BLOT_RENDER_CLEAR)
		overflow |= __builtin_add_overflow(char_len, 1, &char_len);

	// assume every character is escaped (overkill)
	overflow |= __builtin_mul_overflow(char_len, 16, &char_len);

	gsize data_bytes, total_size;
	overflow |= __builtin_mul_overflow(char_len, sizeof(wchar_t), &data_bytes);
	overflow |= __builtin_add_overflow(sizeof(blot_screen), data_bytes, &total_size);
	RETURN_ERRORx(overflow, NULL, error, EOVERFLOW, "blot_screen %ux%u is too large",
		      dim->cols, dim->rows);

	/* data[] is __aligned64, so the whole screen must be (#16): g_malloc
	 * only guarantees 16 bytes.  posix_memalign() rather than
	 * g_aligned_alloc(), which needs glib 2.72; blot_screen_delete() frees
	 * it with free() to match. */
	void *mem = NULL;
	int rc = posix_memalign(&mem, __alignof__(blot_screen), total_size);
	RETURN_ERRORx(rc, NULL, error, rc, "new blot_screen [%zu]", char_len);
	blot_screen *scr = mem;

	scr->flags     = flags;
	scr->dim       = *dim;
	scr->mrg       = *mrg;
	scr->data_size = char_len;
	scr->data_used = 0;

	return scr;
}

void blot_screen_delete(blot_screen *scr)
{
	/* allocated by posix_memalign() in blot_screen_new() */
	free(scr);
}

/* render */

/* escape sequences for colour are written only when the caller didn't ask
 * for BLOT_RENDER_NO_COLOR and the terminal supports them */
static bool blot_screen_use_color(const blot_screen *scr)
{
	return have_color_support && !(scr->flags & BLOT_RENDER_NO_COLOR);
}

/* only layers with a label have a legend line */
static bool blot_screen_has_legend(const struct blot_layer *lay)
{
	return lay->label && *lay->label;
}

static unsigned blot_screen_legend_lines(unsigned count,
					 struct blot_layer *const*lays)
{
	unsigned lines = 0;

	for (unsigned ci=0; ci<count; ci++)
		if (blot_screen_has_legend(lays[ci]))
			lines ++;
	return lines;
}

static bool blot_screen_blank_lines(blot_screen *scr, unsigned lines,
				    GError **error)
{
	wchar_t *p = scr->data + scr->data_used,
		 *end = scr->data + scr->data_size;

	RETURN_ERRORx((gsize)(end - p) <= lines, false, error, ENOSPC,
		      "screen buffer too small for %u lines", lines);

	for (unsigned i=0; i<lines; i++)
		*(p++) = L'\n';

	*p = 0;
	scr->data_used = p - scr->data;
	return true;
}

/* one legend line, written so that it takes at most cols columns on the
 * terminal: escape sequences take none, a tab moves to the next multiple of
 * 8, anything else takes one column (two for wide characters).  The line
 * is cut only between items (#29): the label, then whole "key=value"
 * items, so that no value is ever cut.  A label too long for the line is
 * cut and ends in '~'. */
struct blot_legend_line {
	wchar_t *p, *end;
	unsigned col, cols;
	bool full;
};

static bool blot_legend_room(struct blot_legend_line *ll, size_t n,
			     GError **error)
{
	/* n characters, then the '\n' and the terminating 0 */
	RETURN_ERRORx((size_t)(ll->end - ll->p) < n + 2, false, error, ENOSPC,
		      "screen buffer too small for the legend");
	return true;
}

/* an escape sequence: no column */
static bool blot_legend_esc(struct blot_legend_line *ll, const char *esc,
			    GError **error)
{
	size_t n = strlen(esc);

	if (!blot_legend_room(ll, n, error))
		return false;
	for (size_t i=0; i<n; i++)
		*(ll->p++) = (unsigned char)esc[i];
	return true;
}

/* the column after wc, written at column col */
static unsigned blot_legend_next_col(unsigned col, wchar_t wc)
{
	if (wc == L'\t')
		return (col / 8 + 1) * 8;

	/* combining marks take no column, wide characters two, in every
	 * locale (#18); the label reader made the rest a '?' */
	int w = blot_text_wcwidth(wc);
	return col + (w >= 0 ? (unsigned)w : 1);
}

/* a visible character; one that doesn't fit ends the line */
static bool blot_legend_putc(struct blot_legend_line *ll, wchar_t wc,
			     GError **error)
{
	unsigned next;

	if (ll->full)
		return true;

	next = blot_legend_next_col(ll->col, wc);
	if (next > ll->cols) {
		ll->full = true;
		return true;
	}

	if (!blot_legend_room(ll, 1, error))
		return false;
	*(ll->p++) = wc;
	ll->col = next;
	return true;
}

static bool blot_legend_putws(struct blot_legend_line *ll, const wchar_t *ws,
			      GError **error)
{
	for (; *ws && !ll->full; ws++)
		if (!blot_legend_putc(ll, *ws, error))
			return false;
	return true;
}

/* one item of the details, its separator first: all of it, or nothing and
 * the line ends */
static bool blot_legend_item(struct blot_legend_line *ll, const wchar_t *ws,
			     GError **error)
{
	unsigned col = ll->col;

	if (ll->full)
		return true;
	for (const wchar_t *w = ws; *w && col <= ll->cols; w++)
		col = blot_legend_next_col(col, *w);
	if (col > ll->cols) {
		ll->full = true;
		return true;
	}
	return blot_legend_putws(ll, ws, error);
}

/* the next character of a label: UTF-8 in every locale (#18), read by
 * blot_text_read().  A control character (a newline, an escape...) or a
 * byte that isn't valid UTF-8 becomes '?', so the label stays on its one
 * line and can't send escape sequences; zero-width marks past
 * BLOT_TEXT_MAX_MARKS are dropped.  A tab is kept: it goes to its tab
 * stop, as before.  False at the end of the label */
static bool blot_legend_label_next(struct blot_text_reader *r, wchar_t *wcp)
{
	unsigned w;

	return blot_text_read(r, wcp, &w);
}

/* the label: all of it if it fits, else as much as fits with a '~' after
 * it, which ends the line */
static bool blot_legend_label(struct blot_legend_line *ll, const char *label,
			      GError **error)
{
	struct blot_text_reader r;
	wchar_t wc;
	unsigned col = ll->col, cols = ll->cols;

	if (ll->full)
		return true;

	/* does it all fit? */
	blot_text_reader_init(&r, label, true);
	while (col <= cols && blot_legend_label_next(&r, &wc))
		col = blot_legend_next_col(col, wc);
	bool cut = col > cols;

	if (cut) {
		if (ll->col >= cols) {
			ll->full = true;
			return true;
		}
		/* keep the last column for the '~' */
		ll->cols = cols - 1;
	}

	blot_text_reader_init(&r, label, true);
	while (!ll->full && blot_legend_label_next(&r, &wc))
		if (!blot_legend_putc(ll, wc, error))
			return false;

	if (cut) {
		ll->cols = cols;
		ll->full = false;
		if (!blot_legend_putc(ll, L'~', error))
			return false;
		ll->full = true;
	}
	return true;
}

/* write at most max_lines legend lines: the rows the margins reserved */
static bool blot_screen_can_legend(blot_screen *scr, unsigned count,
				   struct blot_layer *const*lays,
				   unsigned max_lines,
				   GError **error)
{
	struct blot_legend_line ll = {
		.p = scr->data + scr->data_used,
		.end = scr->data + scr->data_size,
		.cols = scr->dim.cols,
	};
	bool use_color = blot_screen_use_color(scr);
	unsigned lines = 0;

	for (int ci=0; ci<count && lines<max_lines; ci++) {
		const struct blot_layer *lay = lays[ci];

		if (!blot_screen_has_legend(lay))
			continue;

		/* same text as with colour, without the escape sequences */
		const char *colstr = use_color ? fg(lay->color) : "";
		const char *rststr = use_color ? COL_RESET : "";

		//wchar_t star = 0x2605; // does not show up in Terminus font
		wchar_t symbol = 0x25D8; // inverse bullet ◘
		/* the details: items, each with the separator before it.
		 * The line is cut between them, never inside one, so a value
		 * is shown whole or not at all; count and its "(+N
		 * non-finite)" are one item */
		wchar_t items[4][96];
		unsigned n_items = 0;

		if (!(scr->flags & BLOT_RENDER_LEGEND_DETAILS)) {
			/* no details */
		} else if (lay->summary.enabled) {
			/* the summary is of the points whose X and Y are
			 * finite; "(+N non-finite)" counts the others, and
			 * with none left there is no avg, min or max: "-" */
			const struct blot_layer_summary *sum = &lay->summary;
			size_t skipped = lay->count - sum->count;
			char tavg[32] = "-", tmin[32] = "-", tmax[32] = "-";
			char tskip[48] = "";

			if (sum->count) {
				if (blot_format_number(tavg, sizeof(tavg),
						       blot_layer_summary_yavg(sum)) <= 0)
					strcpy(tavg, "?");
				if (blot_format_number(tmin, sizeof(tmin), sum->ymin) <= 0)
					strcpy(tmin, "?");
				if (blot_format_number(tmax, sizeof(tmax), sum->ymax) <= 0)
					strcpy(tmax, "?");
			}
			if (skipped)
				snprintf(tskip, sizeof(tskip), " (+%zu non-finite)", skipped);

			int len = swprintf(items[0], G_N_ELEMENTS(items[0]), L"   \tavg=%s", tavg);
			RETURN_ERROR(len<0, false, error, "swprintf");
			len = swprintf(items[1], G_N_ELEMENTS(items[1]), L" min=%s", tmin);
			RETURN_ERROR(len<0, false, error, "swprintf");
			len = swprintf(items[2], G_N_ELEMENTS(items[2]), L" max=%s", tmax);
			RETURN_ERROR(len<0, false, error, "swprintf");
			len = swprintf(items[3], G_N_ELEMENTS(items[3]), L" count=%zu%s",
				       sum->count, tskip);
			RETURN_ERROR(len<0, false, error, "swprintf");
			n_items = 4;
		} else {
			int len = swprintf(items[0], G_N_ELEMENTS(items[0]),
					   L"   \tcount=%zu", lay->count);
			RETURN_ERROR(len<0, false, error, "swprintf");
			n_items = 1;
		}

		/* "<colour>◘ <reset> label<items>" */
		ll.col = 0;
		ll.full = false;
		bool ok = blot_legend_esc(&ll, colstr, error)
			&& blot_legend_putc(&ll, symbol, error)
			&& blot_legend_putc(&ll, L' ', error)
			&& blot_legend_esc(&ll, rststr, error)
			&& blot_legend_putc(&ll, L' ', error)
			&& blot_legend_label(&ll, lay->label, error);
		for (unsigned i=0; ok && i<n_items; i++)
			ok = blot_legend_item(&ll, items[i], error);
		RETURN_IF(!ok, false);

		/* blot_legend_room() kept room for these */
		*(ll.p++) = L'\n';
		lines ++;
	}

	*ll.p = 0;
	scr->data_used = ll.p - scr->data;
	return true;
}

/* an X label in room columns: as much of it as fits, then blanks to fill
 * them.  Its width is counted as the axis cut it (#18), so a wide
 * character takes two columns in every locale; the text is read as
 * UTF-8, not converted by the locale (#18: the C locale can't).  Zero-width
 * marks past BLOT_TEXT_MAX_MARKS are dropped, so a label is at most
 * BLOT_TEXT_MAX_MARKS + (BLOT_TEXT_MAX_MARKS + 1) * room characters, well
 * within the screen buffer's 16 per column. */
static bool blot_screen_x_label(wchar_t **pp, wchar_t *end, const char *label,
				unsigned room, GError **error)
{
	struct blot_text_reader r;
	wchar_t *p = *pp;
	unsigned col = 0, w;
	wchar_t wc;

	blot_text_reader_init(&r, label, false);
	while (blot_text_read(&r, &wc, &w)) {
		if (col + w > room)
			break;
		/* this one and the blanks, and one more for the '\n' */
		RETURN_ERRORx((size_t)(end - p) <= room - col + 1, false, error,
			      ENOSPC, "screen buffer too small for the X labels");
		*(p++) = wc;
		col += w;
	}
	RETURN_ERRORx((size_t)(end - p) <= room - col, false, error,
		      ENOSPC, "screen buffer too small for the X labels");
	for (; col < room; col++)
		*(p++) = L' ';
	*pp = p;
	return true;
}

/* the plot rows and the X-axis rows: the rows between the top and bottom
 * margins, then x_axis_rows rows of the bottom margin */
static bool blot_screen_plot_cans(blot_screen *scr,
				  const blot_xy_limits *lim,
				  const blot_axis * x_axs,
				  const blot_axis * y_axs,
				  unsigned count,
				  blot_canvas *const*cans,
				  unsigned x_axis_rows,
				  GError **error)
{
	const int PREV_COLOR_UNUSED = -1;
	int prev_color = PREV_COLOR_UNUSED;
	wchar_t *p = scr->data + scr->data_used,
		 *end = scr->data + scr->data_size;
	int len;

	/* determine the bounding box for canvases */
	unsigned dsp_top = scr->mrg.top;
	unsigned dsp_bot = scr->dim.rows - scr->mrg.bottom;
	unsigned dsp_hgt = dsp_bot - dsp_top;

	unsigned dsp_lft = scr->mrg.left;
	unsigned dsp_rgt = scr->dim.cols - scr->mrg.right;
	unsigned dsp_wdh = dsp_rgt - dsp_lft;

	bool draw_y_axis = !(scr->flags & BLOT_RENDER_NO_Y_AXIS);
	bool invert_y_axis = !(scr->flags & BLOT_RENDER_DONT_INVERT_Y_AXIS);
	bool use_color = blot_screen_use_color(scr);

	/* now draw; the margins above and below are written by the caller */

	for (unsigned s_y=dsp_top; s_y<dsp_bot; s_y++) {
		/* this is where the canvases are merged */

		unsigned c_y;

		if (invert_y_axis) {
			/* y-axis is inverted on the screen */
			c_y = dsp_hgt - (s_y - dsp_top) - 1;

		} else {
			/* y-axis is not inverted on the screen */
			c_y = s_y - dsp_top;
		}

		unsigned s_x=0;
		if (!draw_y_axis)
			/* no Y-axis, just plot */
			goto plot_cells;

		/* apply Y-axis color */

		if (use_color && prev_color != y_axs->color) {
			const char *colstr = fg(y_axs->color);
			len = swprintf(p, end-p, L"%s", colstr);
			RETURN_ERROR(len<0, false, error, "swprintf");
			p += len;
			prev_color = y_axs->color;
		}

		/* the next dsp_lft characters are the Y-axis label + line */

		const blot_axis_tick *ytick;
		ytick = blot_axis_get_tick_at(y_axs, c_y, error);

		const char *ytick_label = ytick ? ytick->label : "";
		char axis_char = ytick ? '*' : '|';
		/* a label wider than the margin is formatted again for the
		 * room there is: cutting it would print a wrong number.  A
		 * figure's Y-axis has labels that fit (#10); for an axis of
		 * the caller's, the text follows the axis's rule, and when no
		 * text does (it would name a value too far from the tick) the
		 * label is left out. */
		char short_label[32];
		if (ytick && strlen(ytick_label) > dsp_lft-2) {
			unsigned room = MIN(dsp_lft-1, sizeof(short_label));
			if (blot_axis_number_label(y_axs, short_label, room, ytick->value) < 0)
				short_label[0] = 0;
			ytick_label = short_label;
		}
		/* the precision keeps the line at dsp_lft characters */
		len = swprintf(p, end-p, L"%*.*s %c",
			       dsp_lft-2, dsp_lft-2, ytick_label, axis_char);

		RETURN_ERROR(len<0, false, error, "swprintf");
		g_assert_cmpuint(len, ==, dsp_lft);
		p += len;

		s_x += dsp_lft;

plot_cells:
		for (; s_x<scr->dim.cols; s_x++) {

			wchar_t wch = L' ';

			if (s_x < dsp_lft || s_x >= dsp_rgt)
				goto skip_cell;

			unsigned c_x = s_x - dsp_lft;

			wchar_t top_cell = 0;
			blot_color top_col = 0;

			for (int ci=0; ci<count; ci++) {
				const struct blot_canvas *can = cans[ci];

				wchar_t cell = blot_canvas_get_cell(can, c_x, c_y);
				if (!cell)
					continue;

				top_cell = cell;
				top_col = can->color;
			}

			if (top_cell) {
				if (use_color && prev_color != top_col) {
					const char *colstr = fg(top_col);
					len = swprintf(p, end-p, L"%s", colstr);
					RETURN_ERROR(len<0, false, error, "swprintf");
					p += len;
					prev_color = top_col;
				}
				wch = top_cell;
			}

skip_cell:
			*(p++) = wch;
			g_assert_cmpuint((uintptr_t)p, <, (uintptr_t)end);
		}

		if (prev_color != PREV_COLOR_UNUSED) {
			len = swprintf(p, end-p, L"%s", COL_RESET);
			RETURN_ERROR(len<0, false, error, "swprintf");
			p += len;
			prev_color = PREV_COLOR_UNUSED;
		}

		*(p++) = L'\n';
		g_assert_cmpuint((uintptr_t)p, <, (uintptr_t)end);
	}

	for (unsigned s_y=dsp_bot; s_y<dsp_bot+x_axis_rows; s_y++) {
		/* the X-axis line, then its label line */
		if (use_color && prev_color != x_axs->color) {
			const char *colstr = fg(x_axs->color);
			len = swprintf(p, end-p, L"%s", colstr);
			RETURN_ERROR(len<0, false, error, "swprintf");
			p += len;
			prev_color = x_axs->color;
		}

		if (s_y == dsp_bot) {
			/* this line is the X-axis line */
			for (unsigned s_x=0; s_x<scr->dim.cols; s_x++) {
				if (s_x == (dsp_lft-1)) {
					*(p++) = L'+';
				} else if (s_x < dsp_lft) {
					*(p++) = L' ';
				} else if (s_x < dsp_rgt) {
					unsigned c_x = s_x - dsp_lft;
					const blot_axis_tick *xtick;
					xtick = blot_axis_get_tick_at(x_axs, c_x, error);
					if (xtick)
						*(p++) = L'*';
					else
						*(p++) = L'-';
				}
			}
			goto done_bot_line;
		}

		/* this is the X-axis label line */

		len = swprintf(p, end-p, L"%*s",
				   dsp_lft, "");
		RETURN_ERROR(len<0, false, error, "swprintf");
		p += len;

		/* each label starts under its tick and is cut one column
		 * before the next tick and at the edge, so the row is exactly
		 * dsp_wdh wide and no tick loses its label to its neighbour
		 * (the axis already made the labels fit) */
		for (unsigned c_x=0; c_x<dsp_wdh; ) {
			const blot_axis_tick *xtick;
			xtick = blot_axis_get_tick_at(x_axs, c_x, error);
			if (!xtick) {
				*(p++) = L' ';
				c_x ++;
				continue;
			}

			unsigned next = c_x + 1;
			while (next < dsp_wdh && !blot_axis_get_tick_at(x_axs, next, error))
				next ++;
			unsigned room = next - c_x - (next < dsp_wdh ? 1 : 0);
			if (!room)
				room = 1;

			if (!blot_screen_x_label(&p, end, xtick->label, room, error))
				return false;

			c_x += room;
		}

done_bot_line:
		if (prev_color != PREV_COLOR_UNUSED) {
			len = swprintf(p, end-p, L"%s", COL_RESET);
			RETURN_ERROR(len<0, false, error, "swprintf");
			p += len;
			prev_color = PREV_COLOR_UNUSED;
		}

		*(p++) = L'\n';
		g_assert_cmpuint((uintptr_t)p, <, (uintptr_t)end);
	}

	*p = 0;
	scr->data_used = p - scr->data;
	return true;
}

bool blot_screen_render(blot_screen *scr,
			const blot_xy_limits *lim,
			const blot_axis * x_axs,
			const blot_axis * y_axs,
			unsigned count,
			struct blot_layer *const*lays,
			struct blot_canvas *const*cans,
			GError **error)
{
	RETURN_EFAULT_IF(scr==NULL, NULL, error);

	/* the plot is what the margins leave; they must fit in the screen,
	 * and the Y-axis labels need at least the axis and a space */
	RETURN_ERRORx((guint64)scr->mrg.top + scr->mrg.bottom > scr->dim.rows,
		      false, error, EINVAL,
		      "margins top=%u bottom=%u exceed rows=%u",
		      scr->mrg.top, scr->mrg.bottom, scr->dim.rows);
	RETURN_ERRORx((guint64)scr->mrg.left + scr->mrg.right > scr->dim.cols,
		      false, error, EINVAL,
		      "margins left=%u right=%u exceed cols=%u",
		      scr->mrg.left, scr->mrg.right, scr->dim.cols);
	RETURN_ERRORx(!(scr->flags & BLOT_RENDER_NO_Y_AXIS) && scr->mrg.left < 2,
		      false, error, EINVAL,
		      "left margin %u is too narrow for the Y-axis", scr->mrg.left);

	scr->data_used = 0;
	if (scr->flags & BLOT_RENDER_CLEAR) {
		int len = swprintf(scr->data, scr->data_size, L"%s", CLR_SCR);
		RETURN_ERROR(len<0, false, error, "swprintf");
		scr->data_used = len;
	}

	gboolean ok;

	/* The text is exactly dim.rows lines.  The top margin holds the
	 * legend above (padded with blank lines); the bottom margin holds the
	 * X-axis line and its labels, then the legend below (blank lines
	 * between them pad it).  A legend longer than the rows reserved for it
	 * is cut. */
	unsigned legend = blot_screen_legend_lines(count, lays);
	unsigned top = scr->mrg.top, bottom = scr->mrg.bottom;

	unsigned x_axis_rows = (scr->flags & BLOT_RENDER_NO_X_AXIS)
		? 0 : min_t(unsigned, 2, bottom);
	unsigned above = (scr->flags & BLOT_RENDER_LEGEND_ABOVE)
		? min_t(unsigned, legend, top) : 0;
	unsigned below = (scr->flags & BLOT_RENDER_LEGEND_BELOW)
		? min_t(unsigned, legend, bottom - x_axis_rows) : 0;

	ok = blot_screen_can_legend(scr, count, lays, above, error);
	RETURN_IF(!ok, false);

	ok = blot_screen_blank_lines(scr, top - above, error);
	RETURN_IF(!ok, false);

	ok = blot_screen_plot_cans(scr, lim, x_axs, y_axs, count, cans,
				   x_axis_rows, error);
	RETURN_IF(!ok, false);

	ok = blot_screen_blank_lines(scr, bottom - x_axis_rows - below, error);
	RETURN_IF(!ok, false);

	ok = blot_screen_can_legend(scr, count, lays, below, error);
	RETURN_IF(!ok, false);

	return true;
}

const wchar_t * blot_screen_get_text(const blot_screen *scr,
				  gsize *txt_size, GError **error)
{
	RETURN_EFAULT_IF(scr==NULL, NULL, error);

	*txt_size = scr->data_used;
	return scr->data;
}
