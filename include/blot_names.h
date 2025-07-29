/* vim: set noet sw=8 ts=8 tw=120: */
#pragma once

#include <glib.h>

#include "blot_types.h"

static inline const char * blot_plot_type_to_string(blot_plot_type plot_type) {
	switch (plot_type) {
		case BLOT_SCATTER: return "scatter";
		case BLOT_LINE: return "line";
		case BLOT_BAR: return "bar";
		default: return NULL;
	}
}

