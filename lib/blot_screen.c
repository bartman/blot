/* blot: screen represents the final render object */
/* vim: set noet sw=8 ts=8 tw=80: */
#include "blot_screen.h"
#include "blot_error.h"
#include "blot_canvas.h"
#include "blot_layer.h"
#include "blot_color.h"
#include "blot_braille.h"

/* create/delete */

blot_screen * blot_screen_new(unsigned cols, unsigned rows,
			      blot_render_flags flags, GError **error)
{
	gsize char_len = cols * rows;

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
	scr->cols      = cols;
	scr->rows      = rows;
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


static bool blot_screen_plot_cans(blot_screen *scr, unsigned count,
				   struct blot_canvas *const*cans,
				   GError **error)
{
	bool reset_after = false;
	wchar_t *p = scr->data + scr->data_used,
		 *end = scr->data + scr->data_size;

	for (unsigned iy=0; iy<scr->rows; iy++) {

		/* y-axis is inverted on the screen */
		unsigned y = (scr->flags & BLOT_RENDER_DONT_INVERT_Y_AXIS)
			? iy : scr->rows - 1 - iy;

		wchar_t wch;
		for (unsigned x=0; x<scr->cols; x++) {

			wchar_t top_cell = 0;
			blot_color top_col = 0;

			for (int ci=0; ci<count; ci++) {
				const struct blot_canvas *can = cans[ci];

				wchar_t cell = blot_canvas_get_cell(can, x, y);
				if (!cell)
					continue;

				top_cell = cell;
				top_col = can->color;
			}

			wch = L' ';
			if (top_cell) {
				if (!(scr->flags & BLOT_RENDER_NO_COLOR)) {
					const char *colstr = fg(top_col);
					int len = swprintf(p, end-p, L"%s", colstr);
					p += len;
					reset_after = true;
				}
				wch = top_cell;
			}

			*(p++) = wch;
			g_assert_cmpuint((uintptr_t)p, <, (uintptr_t)end);
		}


		if (reset_after) {
			int len = swprintf(p, end-p, L"%s", COL_RESET);
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

bool blot_screen_render(blot_screen *scr, unsigned count,
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

	ok = blot_screen_plot_cans(scr, count, cans, error);
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
