/* blot: terminal utilities */
/* vim: set noet sw=8 ts=8 tw=80: */
#pragma once

#include <glib.h>
#include <stdbool.h>

extern bool blot_terminal_get_size(int *cols, int *rows, GError **);

