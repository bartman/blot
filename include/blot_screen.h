/* blot: screen represents the final render object */
/* vim: set noet sw=8 ts=8 tw=80: */
#pragma once

#include <glib.h>
#include <wchar.h>
#include "blot_types.h"
#include "blot_utils.h"

typedef struct blot_screen {
	blot_render_flags flags;
	blot_dimensions dim;
	blot_margins mrg;
	gsize data_size, data_used;     // in number of gunichar

	wchar_t data[] __aligned64;    // must be at end of structure
} blot_screen;

/* create/delete */

extern blot_screen * blot_screen_new(const blot_dimensions *dim,
				     const blot_margins *mrg,
				     blot_render_flags flags, GError **);
extern void blot_screen_delete(blot_screen *scr);

/* render */

extern bool blot_screen_render(blot_screen *scr,
			       const blot_xy_limits *lim,
			       const struct blot_axis * x_axs,
			       const struct blot_axis * y_axs,
			       unsigned count,
			       struct blot_layer *const*lays,
			       struct blot_canvas *const*cans,
			       GError **);

extern const wchar_t * blot_screen_get_text(const blot_screen *scr,
					     gsize *txt_size, GError**);
