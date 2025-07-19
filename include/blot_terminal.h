/* blot: terminal utilities */
/* vim: set noet sw=8 ts=8 tw=120: */
#pragma once

#include <glib.h>
#include <stdbool.h>
#include "blot_types.h"

extern bool blot_terminal_set_size(blot_dimensions dims, GError **);
extern bool blot_terminal_get_size(blot_dimensions *dims, GError **);

