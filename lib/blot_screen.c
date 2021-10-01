/* blot: screen represents the final render object */
/* vim: set noet sw=8 ts=8 tw=80: */
#include <stdio.h>
#include "blot_screen.h"
#include "blot_error.h"
#include "blot_canvas.h"
#include "blot_layer.h"
#include "blot_color.h"
#include "blot_braille.h"

/* create/delete */

blot_screen * blot_screen_new(const blot_dimensions *dim,
			      const blot_margins *mrg,
			      blot_render_flags flags, GError **error)
{
	gsize char_len = dim->cols * dim->rows;

	// add extra space for screen clear escape code
	if (flags & BLOT_RENDER_CLEAR)
		char_len ++;

	// assume every character is escaped (overkill)
	char_len *= 16;

	gsize data_bytes = char_len * sizeof(wchar_t);
	gsize total_size = sizeof(blot_screen) + data_bytes;
	blot_screen *scr = g_malloc(total_size);
	RETURN_ERROR(!scr, NULL, error, "new blot_screen [%zu]", char_len);

	scr->flags     = flags;
	scr->dim       = *dim;
	scr->mrg       = *mrg;
	scr->data_size = char_len;
	scr->data_used = 0;

	return scr;
}

void blot_screen_delete(blot_screen *scr)
{
	g_free(scr);
}

/* render */

static bool blot_screen_can_legend(blot_screen *scr, unsigned count,
				   struct blot_layer *const*lays,
				   GError **error)
{
	wchar_t *p = scr->data + scr->data_used,
		 *end = scr->data + scr->data_size;

	for (int ci=0; ci<count; ci++) {
		const struct blot_layer *lay = lays[ci];
		const char *colstr = fg(lay->color);

		if (!lay->label)
			continue;

		wchar_t star = 0x2605;
		int len = swprintf(p, end-p, L"%s%lc %s %s\n",
				   colstr, star, COL_RESET,
				   lay->label);
		RETURN_IF(len<0, false);
		p += len;
	}

	*p = 0;
	scr->data_used = p - scr->data;
	return true;
}


static bool blot_screen_plot_cans(blot_screen *scr, blot_color axis_color,
				  const blot_xy_limits *lim,
				  unsigned count,
				  blot_canvas *const*cans,
				  GError **error)
{
	bool reset_after = false;
	wchar_t *p = scr->data + scr->data_used,
		 *end = scr->data + scr->data_size;
	int len, ti;

	/* determine the bounding box for canvases */
	unsigned dsp_top = scr->mrg.top;
	unsigned dsp_bot = scr->dim.rows - scr->mrg.bottom;
	unsigned dsp_hgt = dsp_bot - dsp_top;

	unsigned dsp_lft = scr->mrg.left;
	unsigned dsp_rgt = scr->dim.cols - scr->mrg.right;
	unsigned dsp_wdh = dsp_rgt - dsp_lft;

	bool draw_x_axis = !(scr->flags & BLOT_RENDER_NO_X_AXIS);
	bool draw_y_axis = !(scr->flags & BLOT_RENDER_NO_Y_AXIS);
	bool invert_y_axis = !(scr->flags & BLOT_RENDER_DONT_INVERT_Y_AXIS);

	/* prepare the X-axis ticks */

	const int x_tick_num = 5;
	unsigned x_tick_width = 0;
	unsigned x_tick_cols[x_tick_num];
	char x_tick_labs[x_tick_num][dsp_wdh];

	if (draw_x_axis) {
		/* we will compute the tick mark locations and values */

		double s_tick_jump = dsp_wdh / (x_tick_num-1);
		double v_tick_jump = (lim->x_max - lim->x_min) / (x_tick_num-1);

		for (ti=0; ti<x_tick_num; ti++) {
			unsigned ofs = s_tick_jump * ti;
			x_tick_cols[ti] = dsp_lft + 1 + ofs;
			double val = lim->x_min + (v_tick_jump * ti);

			snprintf(x_tick_labs[ti], dsp_wdh, "%.3f", val);
		}

		x_tick_width = s_tick_jump;
	}

	/* prepare the Y-axis ticks */

	const int y_tick_num = 5;
	unsigned y_tick_rows[y_tick_num];
	char y_tick_labs[y_tick_num][dsp_wdh];

	if (draw_y_axis) {
		/* we will compute the tick mark locations and values */

		double s_tick_jump = dsp_hgt / (y_tick_num-1);
		double v_tick_jump = (lim->y_max - lim->y_min) / (y_tick_num-1);

		for (ti=0; ti<y_tick_num; ti++) {
			unsigned ofs = s_tick_jump * ti;
			y_tick_rows[ti] = dsp_top + 1 + ofs;
			double val;

			if (invert_y_axis)
				val= lim->y_max - (v_tick_jump * ti);
			else
				val= lim->y_min + (v_tick_jump * ti);

			snprintf(y_tick_labs[ti], dsp_wdh, "%.3f", val);
		}
	}

	/* now draw */

	for (unsigned s_y=0; s_y<dsp_top; s_y++) {
		/* lines above the display area are just skipped */
		*(p++) = L'\n';
		g_assert_cmpuint((uintptr_t)p, <, (uintptr_t)end);
	}

	ti = 0;
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

		/* apply axis color */

		const char *colstr = fg(axis_color);
		len = swprintf(p, end-p, L"%s", colstr);
		p += len;
		reset_after = true;

		/* the next dsp_lft characters are the Y-axis label + line */

		if (ti<y_tick_num && s_y == y_tick_rows[ti]) {
			len = swprintf(p, end-p, L"%*s *",
				       dsp_lft-2, y_tick_labs[ti]);
			ti++;

		} else {
			len = swprintf(p, end-p, L"%*s |",
				       dsp_lft-2, "");
		}

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
				if (!(scr->flags & BLOT_RENDER_NO_COLOR)) {
					const char *colstr = fg(top_col);
					len = swprintf(p, end-p, L"%s", colstr);
					p += len;
					reset_after = true;
				}
				wch = top_cell;
			}

skip_cell:
			*(p++) = wch;
			g_assert_cmpuint((uintptr_t)p, <, (uintptr_t)end);
		}

		if (reset_after) {
			len = swprintf(p, end-p, L"%s", COL_RESET);
			p += len;
			reset_after = false;
		}

		*(p++) = L'\n';
		g_assert_cmpuint((uintptr_t)p, <, (uintptr_t)end);
	}

	ti = 0;
	for (unsigned s_y=dsp_bot; s_y<scr->dim.rows; s_y++) {
		/* the bottom lines may contain the X-axis */
		if (!draw_x_axis)
			goto done_bot_line;

		const char *colstr = fg(axis_color);
		len = swprintf(p, end-p, L"%s", colstr);
		p += len;
		reset_after = true;

		if (s_y == dsp_bot) {
			/* this line is the X-axis line */
			for (unsigned s_x=0; s_x<scr->dim.cols; s_x++) {
				if (s_x == (dsp_lft-1)) {
					*(p++) = L'+';
				} else if (s_x < dsp_lft) {
					*(p++) = L' ';
				} else if (s_x < dsp_rgt) {
					if (ti<x_tick_num && s_x == x_tick_cols[ti]) {
						*(p++) = L'*';
						ti++;
					} else {
						*(p++) = L'-';
					}
				}

			}
			goto done_bot_line;

		} else if (s_y > (dsp_bot+1) ) {
			/* there may be lines below the X-axis, they will be * blank */
			goto done_bot_line;
		}

		/* this is the X-axis label line */

		len = swprintf(p, end-p, L"%*s ",
				   dsp_lft-1, "");
		p += len;

		for (ti=0; ti<x_tick_num; ti++) {
			len = swprintf(p, end-p, L"%-*s",
					   x_tick_width, x_tick_labs[ti]);
			p += len;
		}

done_bot_line:
		if (reset_after) {
			len = swprintf(p, end-p, L"%s", COL_RESET);
			p += len;
			reset_after = false;
		}

		*(p++) = L'\n';
		g_assert_cmpuint((uintptr_t)p, <, (uintptr_t)end);
	}

	*p = 0;
	scr->data_used = p - scr->data;
	return true;
}

bool blot_screen_render(blot_screen *scr, blot_color axis_color,
			const blot_xy_limits *lim,
			unsigned count,
			struct blot_layer *const*lays,
			struct blot_canvas *const*cans,
			GError **error)
{
	RETURN_EFAULT_IF(scr==NULL, NULL, error);

	scr->data_used = 0;
	if (scr->flags & BLOT_RENDER_CLEAR) {
		int len = swprintf(scr->data, scr->data_size, L"%s", CLR_SCR);
		scr->data_used = len;
	}

	gboolean ok;

	if (scr->flags & BLOT_RENDER_LEGEND_ABOVE) {
		ok = blot_screen_can_legend(scr, count, lays, error);
		RETURN_IF(!ok, false);
	}

	ok = blot_screen_plot_cans(scr, axis_color, lim, count, cans, error);
	RETURN_IF(!ok, false);

	if (scr->flags & BLOT_RENDER_LEGEND_BELOW) {
		ok = blot_screen_can_legend(scr, count, lays, error);
		RETURN_IF(!ok, false);
	}

	return true;
}

const wchar_t * blot_screen_get_text(const blot_screen *scr,
				  gsize *txt_size, GError **error)
{
	RETURN_EFAULT_IF(scr==NULL, NULL, error);

	*txt_size = scr->data_used;
	return scr->data;
}
