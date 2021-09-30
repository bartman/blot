/* blot: screen represents the final render object */
/* vim: set noet sw=8 ts=8 tw=80: */
#pragma once

#include <glib.h>
#include <wchar.h>
#include "blot_types.h"
#include "blot_utils.h"

typedef struct blot_screen {
	blot_render_flags flags;
	unsigned cols, rows;
	gsize data_size, data_used;     // in number of gunichar

	wchar_t data[] __aligned64;    // must be at end of structure
} blot_screen;

/* create/delete */

extern blot_screen * blot_screen_new(unsigned cols, unsigned rows,
				     blot_render_flags flags, GError **);
extern void blot_screen_delete(blot_screen *scr);

/* render */

extern bool blot_screen_render(blot_screen *scr, unsigned count,
			      struct blot_canvas *const*cans,
			      GError **);

extern const wchar_t * blot_screen_get_text(const blot_screen *scr,
					     gsize *txt_size, GError**);
