/* blot: screen represents the final render object */
/* vim: set noet sw=8 ts=8 tw=80: */
#include <stdio.h>
#include "blot_screen.h"
#include "blot_error.h"
#include "blot_canvas.h"
#include "blot_layer.h"
#include "blot_color.h"
#include "blot_braille.h"
#include "blot_axis.h"

/* create/delete */

blot_screen * blot_screen_new(const blot_dimensions *dim,
			      const blot_margins *mrg,
			      blot_render_flags flags, GError **error)
{
	gsize char_len = (gsize)dim->cols * (gsize)dim->rows;

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
		RETURN_ERROR(len<0, false, error, "swprintf");
		p += len;
	}

	*p = 0;
	scr->data_used = p - scr->data;
	return true;
}


static bool blot_screen_plot_cans(blot_screen *scr,
				  const blot_xy_limits *lim,
				  const blot_axis * x_axs,
				  const blot_axis * y_axs,
				  unsigned count,
				  blot_canvas *const*cans,
				  GError **error)
{
	bool reset_after = false;
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

	bool draw_x_axis = !(scr->flags & BLOT_RENDER_NO_X_AXIS);
	bool draw_y_axis = !(scr->flags & BLOT_RENDER_NO_Y_AXIS);
	bool invert_y_axis = !(scr->flags & BLOT_RENDER_DONT_INVERT_Y_AXIS);

	/* now draw */

	for (unsigned s_y=0; s_y<dsp_top; s_y++) {
		/* lines above the display area are just skipped */
		*(p++) = L'\n';
		g_assert_cmpuint((uintptr_t)p, <, (uintptr_t)end);
	}

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

		const char *colstr = fg(y_axs->color);
		len = swprintf(p, end-p, L"%s", colstr);
		RETURN_ERROR(len<0, false, error, "swprintf");
		p += len;
		reset_after = true;

		/* the next dsp_lft characters are the Y-axis label + line */

		const blot_axis_tick *ytick;
		ytick = blot_axis_get_tick_at(y_axs, c_y, error);

		if (ytick) {
			len = swprintf(p, end-p, L"%*s *",
				       dsp_lft-2, ytick->label);

		} else {
			len = swprintf(p, end-p, L"%*s |",
				       dsp_lft-2, "");
		}

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
				if (!(scr->flags & BLOT_RENDER_NO_COLOR)) {
					const char *colstr = fg(top_col);
					len = swprintf(p, end-p, L"%s", colstr);
					RETURN_ERROR(len<0, false, error, "swprintf");
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
			RETURN_ERROR(len<0, false, error, "swprintf");
			p += len;
			reset_after = false;
		}

		*(p++) = L'\n';
		g_assert_cmpuint((uintptr_t)p, <, (uintptr_t)end);
	}

	for (unsigned s_y=dsp_bot; s_y<scr->dim.rows; s_y++) {
		/* the bottom lines may contain the X-axis */
		if (!draw_x_axis)
			goto done_bot_line;

		const char *colstr = fg(x_axs->color);
		len = swprintf(p, end-p, L"%s", colstr);
		RETURN_ERROR(len<0, false, error, "swprintf");
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

		} else if (s_y > (dsp_bot+1) ) {
			/* there may be lines below the X-axis, they will be * blank */
			goto done_bot_line;
		}

		/* this is the X-axis label line */

		len = swprintf(p, end-p, L"%*s",
				   dsp_lft, "");
		RETURN_ERROR(len<0, false, error, "swprintf");
		p += len;

		for (unsigned c_x=0; c_x<dsp_wdh; c_x++) {
			const blot_axis_tick *xtick;
			xtick = blot_axis_get_tick_at(x_axs, c_x, error);
			if (!xtick) {
				*(p++) = L' ';
				continue;
			}

			len = swprintf(p, end-p, L"%s ", xtick->label);
			RETURN_ERROR(len<0, false, error, "swprintf");
			p += len;

			c_x += len-1;
		}

done_bot_line:
		if (reset_after) {
			len = swprintf(p, end-p, L"%s", COL_RESET);
			RETURN_ERROR(len<0, false, error, "swprintf");
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

	scr->data_used = 0;
	if (scr->flags & BLOT_RENDER_CLEAR) {
		int len = swprintf(scr->data, scr->data_size, L"%s", CLR_SCR);
		RETURN_ERROR(len<0, false, error, "swprintf");
		scr->data_used = len;
	}

	gboolean ok;

	if (scr->flags & BLOT_RENDER_LEGEND_ABOVE) {
		ok = blot_screen_can_legend(scr, count, lays, error);
		RETURN_IF(!ok, false);
	}

	ok = blot_screen_plot_cans(scr, lim, x_axs, y_axs, count, cans, error);
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
