/* blot: screen represents the final render object */
/* vim: set noet sw=8 ts=8 tw=80: */
#include "blot_screen.h"
#include "blot_error.h"
#include "blot_canvas.h"
#include "blot_color.h"

/* create/delete */

blot_screen * blot_screen_new(unsigned columns, unsigned rows,
			      blot_render_flags flags, GError **error)
{
	gsize data_size = columns * rows;

	// add extra space for screen clear escape code
	if (flags & BLOT_RENDER_CLEAR)
		data_size ++;

	// assume every character is escaped (overkill)
	data_size *= 16;

	gsize total_size = sizeof(blot_screen) + data_size;
	blot_screen *scr = g_malloc(total_size);
	RETURN_ERROR(!scr, NULL, error, "new blot_screen [%zu]", data_size);

	scr->flags = flags;
	scr->columns = columns;
	scr->rows = rows;
	scr->data_size = data_size;
	scr->data_used = 0;

	return scr;
}

void blot_screen_delete(blot_screen *scr)
{
	g_free(scr);
}

/* merge/render */

bool blot_screen_merge(blot_screen *scr, unsigned count,
		       struct blot_canvas *const*cans,
		       GError **error)
{
	RETURN_EFAULT_IF(scr==NULL, NULL, error);

	char *p = scr->data, *end = scr->data + scr->data_size;

	if (scr->flags & BLOT_RENDER_CLEAR) {
		gsize len = strlen(CLR_SCR);
		memcpy(p, CLR_SCR, len);
		p += len;
	}

	for (unsigned y=0; y<scr->rows; y++) {
		for (unsigned x=0; x<scr->columns; x++) {

			char top_ch = 0;
			blot_color top_col = 0;

			for (int ci=0; ci<count; ci++) {
				const struct blot_canvas *can = cans[ci];

				char ch = blot_canvas_get(can, x, y);
				if (!ch)
					continue;

				top_ch = ch;
				top_col = can->color;
			}

			if (top_ch) {
				// TODO: apply color
				(void)top_col;
				*(p++) = top_ch;
			} else {
				*(p++) = ' ';
			}

			g_assert_cmpuint((uintptr_t)p, <, (uintptr_t)end);
		}

		*(p++) = '\n';
		g_assert_cmpuint((uintptr_t)p, <, (uintptr_t)end);
	}

	*p = 0;
	scr->data_used = p - scr->data;
	return true;
}

const char * blot_screen_get_text(const blot_screen *scr,
				  gsize *txt_size, GError **error)
{
	RETURN_EFAULT_IF(scr==NULL, NULL, error);

	*txt_size = scr->data_used;
	return scr->data;
}
