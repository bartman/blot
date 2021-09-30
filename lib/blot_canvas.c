/* blot: a canvas is a render of a single layer w/o colour coding */
/* vim: set noet sw=8 ts=8 tw=80: */
#include <string.h>
#include "blot_canvas.h"
#include "blot_braille.h"
#include "blot_error.h"

/* create/delete */

blot_canvas * blot_canvas_new(unsigned _cols, unsigned _rows,
			      blot_render_flags flags, blot_color color,
			      GError **error)
{
	unsigned cols = _cols;
	unsigned rows = _rows;

	if (flags & BLOT_RENDER_BRAILLE) {
		/* when using braille, we must be allowed to use unicode */
		RETURN_ERROR(flags & BLOT_RENDER_NO_UNICODE, NULL, error,
			     "flags BLOT_RENDER_NO_UNICODE and "
			     "BLOT_RENDER_NO_UNICODE are exclusive");

		/* the font can handle higher density,
		 * that we will store in the bitmap */
		cols *= BRAILLE_GLYPH_COLS;
		rows *= BRAILLE_GLYPH_ROWS;
	}

	gsize bitmap_size = cols * rows;
	gsize bitmap_bytes = (bitmap_size + BLOT_CANVAS_BITMAP_CELL_SIZE - 1)
		/ BLOT_CANVAS_BITMAP_CELL_SIZE;

	gsize total_size = sizeof(blot_canvas) + bitmap_bytes;
	blot_canvas *can = g_malloc(total_size);
	RETURN_ERROR(!can, NULL, error, "new blot_canvas [%zu]", bitmap_size);

	can->cols         = cols;
	can->rows         = rows;
	can->flags        = flags;
	can->color        = color;
	can->bitmap_size  = bitmap_size;
	can->bitmap_bytes = bitmap_bytes;

	if (flags & BLOT_RENDER_BRAILLE) {
		can->braille.masks = (flags & BLOT_RENDER_DONT_INVERT_Y_AXIS)
			? braille_masks : braille_upsidedown_masks;

	} else {
		can->no_braille.plot_char = (flags & BLOT_RENDER_NO_UNICODE)
			?  L'*' : L'•';
	}

	memset(can->bitmap, 0, bitmap_bytes);

	return can;
}
void blot_canvas_delete(blot_canvas *can)
{
	g_free(can);
}

