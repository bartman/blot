/* blot: terminal utilities */
/* vim: set noet sw=8 ts=8 tw=120: */
#pragma once

#include <glib.h>
#include <stdbool.h>

#include "blot_compiler.h"
#include "blot_types.h"

BLOT_EXTERN_C_START

/* the terminal size, from BLOT_MIN_COLS x BLOT_MIN_ROWS to BLOT_MAX_COLS x
 * BLOT_MAX_ROWS (EINVAL otherwise, as blot_figure_set_screen_size(); see
 * blot_figure.h).  A figure without a screen size uses it less a 2-cell
 * border, but never less than the minimum. */
BLOT_API bool blot_terminal_set_size(blot_dimensions dims, GError **);
BLOT_API bool blot_terminal_get_size(blot_dimensions *dims, GError **);

BLOT_EXTERN_C_END

