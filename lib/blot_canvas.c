/* blot: a canvas is a render of a single layer w/o colour coding */
/* vim: set noet sw=8 ts=8 tw=80: */
#include "blot_canvas.h"
#include "blot_error.h"

/* create/delete */

blot_canvas * blot_canvas_new(unsigned columns, unsigned rows,
			      blot_render_flags flags, blot_color color,
			      GError **error)
{

	gsize data_size = columns * rows;
	gsize total_size = sizeof(blot_canvas) + data_size;
	blot_canvas *can = g_malloc(total_size);
	RETURN_ERROR(!can, NULL, error, "new blot_canvas [%zu]", data_size);

	can->columns = columns;
	can->rows = rows;
	can->flags = flags;
	can->color = color;
	can->data_size = data_size;

	memset(can->data, ' ', data_size);

	return can;
}
void blot_canvas_delete(blot_canvas *can)
{
	g_free(can);
}

