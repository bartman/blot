/* blot: terminal utilities */
/* vim: set noet sw=8 ts=8 tw=80: */
#pragma once

#include <glib.h>
#include "blot_types.h"

extern bool blot_terminal_get_size(unsigned *columns, unsigned *rows, GError **);

