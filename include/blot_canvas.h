/* blot: a canvas is a render of a single layer w/o colour coding */
/* vim: set noet sw=8 ts=8 tw=80: */
#pragma once

#include <glib.h>
#include "blot_types.h"
#include "blot_utils.h"
#include "blot_braille.h"

typedef struct blot_canvas {
	blot_dimensions dim;
	blot_render_flags flags;
	blot_color color;

	union {
		struct {
			// only when braille is on
			guint8 *masks;          // braille_masks or braille_upsidedown_masks
		} braille;
		struct {
			// only when braille is off
			wchar_t plot_char;
		} no_braille;
	};

	gsize bitmap_size;              // number of bits available
	gsize bitmap_bytes;             // byte size of bitmap array
	guint8 bitmap[] __aligned64;    // must be at end of structure
} blot_canvas;

#define BLOT_CANVAS_BITMAP_CELL_SIZE 8

/* create/delete */

extern blot_canvas * blot_canvas_new(unsigned cols, unsigned rows,
				     blot_render_flags flags, blot_color color,
				     GError **);
extern void blot_canvas_delete(blot_canvas *fig);

/* render */

static inline bool blot_canvas_set(blot_canvas *can, unsigned col, unsigned row, bool val)
{
	g_assert_nonnull(can);

	if (col >= can->dim.cols)
		return false;

	if (row >= can->dim.rows)
		return false;

	if (can->flags & BLOT_RENDER_BRAILLE) {

		unsigned byte = ((row/4) * (can->dim.cols/2)) + (col/2);
		unsigned bit = ((row%4)*2) + (col%2);
		guint8   mask = can->braille.masks[bit];

		g_assert_cmpuint(byte, <, can->bitmap_bytes);

#if 0
		wchar_t wch = BRAILLE_GLYPH_BASE + mask;
		g_print("%s[%u,%u] = %u, bitmap[%u] & 0x%02x (%u) -> %lc\n",
			__func__, col, row, val, byte, mask, bit, wch);
#endif

		if (val)
			can->bitmap[byte] |= mask;
		else
			can->bitmap[byte] &= ~(mask);

	} else {
		unsigned idx = (row * can->dim.cols) + col;

		g_assert_cmpuint(idx, <, can->bitmap_size);

		unsigned byte = idx / BLOT_CANVAS_BITMAP_CELL_SIZE;
		unsigned bit  = idx % BLOT_CANVAS_BITMAP_CELL_SIZE;
		guint8   mask = 1<<bit;

		g_assert_cmpuint(byte, <, can->bitmap_bytes);

#if 0
		g_print("%s[%u,%u] = %u, idx=%u, bitmap[%u] & 0x%02x (%u)\n",
			__func__, col, row, val, idx, byte, mask, bit);
#endif

		if (val)
			can->bitmap[byte] |= mask;
		else
			can->bitmap[byte] &= ~(mask);
	}

	return true;
}

static inline bool blot_canvas_get(const blot_canvas *can, unsigned col, unsigned row)
{
	g_assert_nonnull(can);

	if (col >= can->dim.cols)
		return 0;

	if (row >= can->dim.rows)
		return 0;

	if (can->flags & BLOT_RENDER_BRAILLE) {

		unsigned byte = ((row/4) * (can->dim.cols/2)) + (col/2);
		unsigned bit = ((row%4)*2) + (col%2);
		guint8   mask = can->braille.masks[bit];

		g_assert_cmpuint(byte, <, can->bitmap_bytes);

		bool val = !!(can->bitmap[byte] & (mask));

#if 0
		if (val) {
			wchar_t wch = BRAILLE_GLYPH_BASE + mask;
			g_print("%s[%u,%u] = %u, bitmap[%u] & 0x%02x (%u) -> %lc\n",
				__func__, col, row, val, byte, mask, bit, wch);
		}
#endif

		return val;

	} else {
		unsigned idx = (row * can->dim.cols) + col;

		g_assert_cmpuint(idx, <, can->bitmap_size);

		unsigned byte = idx / BLOT_CANVAS_BITMAP_CELL_SIZE;
		unsigned bit  = idx % BLOT_CANVAS_BITMAP_CELL_SIZE;
		unsigned mask = 1<<bit;

		g_assert_cmpuint(byte, <, can->bitmap_bytes);

		bool val = !!(can->bitmap[byte] & (mask));

#if 0
		if (val)
			g_print("%s[%u,%u] = %u, idx=%u, bitmap[%u] & 0x%02x (%u)\n",
				__func__, col, row, val, idx, byte, mask, bit);
#endif

		return val;
	}
}

static inline wchar_t blot_canvas_get_cell(const blot_canvas *can,
					    unsigned cell_col, unsigned cell_row)
{
	g_assert_nonnull(can);

	if (can->flags & BLOT_RENDER_BRAILLE) {
		unsigned max_cell_cols = can->dim.cols/BRAILLE_GLYPH_COLS;
		unsigned max_cell_rows = can->dim.rows/BRAILLE_GLYPH_ROWS;

		if (cell_col >= max_cell_cols)
			return 0;

		if (cell_row >= max_cell_rows)
			return 0;

		unsigned idx = (cell_row * max_cell_cols) + cell_col;

		g_assert_cmpuint(idx, <, can->bitmap_bytes);

		unsigned char val = can->bitmap[idx];
		if (likely (!val))
			return 0;

		wchar_t wch = BRAILLE_GLYPH_BASE + can->bitmap[idx];
		return wch;

	} else {
		bool val = blot_canvas_get(can, cell_col, cell_row);

		return val ? can->no_braille.plot_char : 0;
	}
}

