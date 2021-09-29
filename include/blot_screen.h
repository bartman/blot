/* blot: screen represents the final render object */
/* vim: set noet sw=8 ts=8 tw=80: */
#pragma once

#include <glib.h>
#include "blot_types.h"
#include "blot_utils.h"

typedef struct blot_screen {
	blot_render_flags flags;
	unsigned columns, rows;
	gsize data_size, data_used;

	char data[] __aligned64; // must be at end of structure
} blot_screen;

/* create/delete */

extern blot_screen * blot_screen_new(unsigned columns, unsigned rows,
				     blot_render_flags flags, GError **);
extern void blot_screen_delete(blot_screen *scr);

/* merge/render */

extern bool blot_screen_merge(blot_screen *scr, unsigned count,
			      struct blot_canvas *const*cans,
			      GError **);

extern const char * blot_screen_get_text(const blot_screen *scr,
					 gsize *txt_size, GError**);
