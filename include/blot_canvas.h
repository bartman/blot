/* blot: a canvas is a render of a single layer w/o colour coding */
/* vim: set noet sw=8 ts=8 tw=80: */
#pragma once

#include <glib.h>
#include "blot_types.h"
#include "blot_utils.h"

typedef struct blot_canvas {
	unsigned columns, rows;
	blot_render_flags flags;
	blot_color color;
	gsize data_size;

	char data[] __aligned64; // must be at end of structure
} blot_canvas;

/* create/delete */

extern blot_canvas * blot_canvas_new(unsigned columns, unsigned rows,
				     blot_render_flags flags, blot_color color,
				     GError **);
extern void blot_canvas_delete(blot_canvas *fig);

/* render */

static inline bool blot_canvas_set(blot_canvas *can, unsigned col, unsigned row, char ch)
{
	g_assert_nonnull(can);

	if (col >= can->columns)
		return false;

	if (row >= can->rows)
		return false;

	unsigned idx = (row * can->columns) + col;

	g_assert_cmpuint(idx, <, can->data_size);

	can->data[idx] = ch;
	return true;
}

static inline char blot_canvas_get(const blot_canvas *can, unsigned col, unsigned row)
{
	g_assert_nonnull(can);

	if (col >= can->columns)
		return 0;

	if (row >= can->rows)
		return 0;

	unsigned idx = (row * can->columns) + col;

	g_assert_cmpuint(idx, <, can->data_size);

	return can->data[idx];
}
