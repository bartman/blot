/* vim: set noet sw=8 ts=8 tw=80: */
#pragma once

#include <glib.h>
#include <stdint.h>
#include <stdbool.h>

struct blot_figure;
struct blot_layer;
struct blot_canvas;
struct blot_screen;

typedef enum blot_plot_type {
	BLOT_SCATTER,
	BLOT_PLOT_TYPE_MAX
} blot_plot_type;

typedef enum blot_data_type {
	BLOT_LAYER_INT32,
	BLOT_LAYER_INT64,
	BLOT_LAYER_FLOAT,
	BLOT_LAYER_DOUBLE,
	BLOT_DATA_TYPE_MAX
} blot_data_type;

typedef guint8 blot_color;

typedef enum blot_render_flags {
	BLOT_RENDER_NONE                 = 0x00000000,
	BLOT_RENDER_CLEAR                = 0x00000001,  // insert screen reset character
	BLOT_RENDER_BRAILLE              = 0x00000002,  // default is to plot with '•' or '*'
	BLOT_RENDER_DONT_INVERT_Y_AXIS   = 0x00000004,  // default is to put origin on the bottom left
	BLOT_RENDER_NO_UNICODE           = 0x00000008,  // no unicode -- use '*'
	BLOT_RENDER_NO_COLOR             = 0x00000010,  // no color
} blot_render_flags;

typedef struct blot_xy_limits {
	double x_min, x_max;
	double y_min, y_max;
} blot_xy_limits;
