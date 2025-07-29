/* vim: set noet sw=8 ts=8 tw=120: */
#pragma once

#include <glib.h>

#include "blot_compiler.h"

struct blot_figure;
struct blot_layer;
struct blot_canvas;
struct blot_screen;
struct blot_axis;

/* this allows us to use the same syntax for enums in C and C++ */
#ifdef __cplusplus
#define DEFINE_ENUM_OPERATORS_FOR(T) \
constexpr T operator|(T a, T b) { \
	return static_cast<T>(static_cast<unsigned int>(a) | static_cast<unsigned int>(b)); \
} \
constexpr T& operator|=(T &a, T &b) { \
	a = a | b; \
	return a; \
}
#else
#define DEFINE_ENUM_OPERATORS_FOR(T)
#endif

typedef enum blot_plot_type {
	BLOT_SCATTER,
	BLOT_LINE,
	BLOT_BAR,
	BLOT_PLOT_TYPE_MAX
} blot_plot_type;
DEFINE_ENUM_OPERATORS_FOR(blot_plot_type)

typedef enum blot_data_type {
	BLOT_DATA_X_MASK                = 0x000F,
	BLOT_DATA_X_INT16               = 0x0000,
	BLOT_DATA_X_INT32               = 0x0001,
	BLOT_DATA_X_INT64               = 0x0002,
	BLOT_DATA_X_FLOAT               = 0x0003,
	BLOT_DATA_X_DOUBLE              = 0x0004,

	BLOT_DATA_Y_MASK                = 0x00F0,
	BLOT_DATA_Y_INT16               = 0x0000,
	BLOT_DATA_Y_INT32               = 0x0010,
	BLOT_DATA_Y_INT64               = 0x0020,
	BLOT_DATA_Y_FLOAT               = 0x0030,
	BLOT_DATA_Y_DOUBLE              = 0x0040,

#define BLOT_DATA_(X,Y) (BLOT_DATA_X_##X | BLOT_DATA_Y_##Y)

	BLOT_DATA_INT16                 = BLOT_DATA_(INT16,  INT16),
	BLOT_DATA_INT32                 = BLOT_DATA_(INT32,  INT32),
	BLOT_DATA_INT64                 = BLOT_DATA_(INT64,  INT64),
	BLOT_DATA_FLOAT                 = BLOT_DATA_(FLOAT,  FLOAT),
	BLOT_DATA_DOUBLE                = BLOT_DATA_(DOUBLE, DOUBLE),

	BLOT_DATA_TYPE_MAX              = (BLOT_DATA_X_MASK | BLOT_DATA_Y_MASK)
} blot_data_type;
DEFINE_ENUM_OPERATORS_FOR(blot_data_type)
#define BLOT_DATA_TYPE(x,y) (blot_data_type)(((x) & BLOT_DATA_X_MASK) | ((y) & BLOT_DATA_Y_MASK))

typedef guint8 blot_color;

typedef enum blot_render_flags {
	BLOT_RENDER_NONE                = 0x00000000,
	BLOT_RENDER_CLEAR               = 0x00000001,   // insert screen reset character
	BLOT_RENDER_BRAILLE             = 0x00000002,   // default is to plot with '•' or '*'
	BLOT_RENDER_DONT_INVERT_Y_AXIS  = 0x00000004,   // default is to put origin on the bottom left
	BLOT_RENDER_NO_UNICODE          = 0x00000008,   // no unicode -- use '*'
	BLOT_RENDER_NO_COLOR            = 0x00000010,   // no color
	BLOT_RENDER_LEGEND_ABOVE        = 0x00000020,
	BLOT_RENDER_LEGEND_BELOW        = 0x00000040,
	BLOT_RENDER_NO_X_AXIS           = 0x00000080,
	BLOT_RENDER_NO_Y_AXIS           = 0x00000100,
	BLOT_RENDER_LEGEND_DETAILS      = 0x00000200,
} blot_render_flags;
DEFINE_ENUM_OPERATORS_FOR(blot_render_flags)

typedef struct blot_xy_limits {
	double x_min, x_max;
	double y_min, y_max;
} blot_xy_limits;

typedef struct blot_margins {
	unsigned left, right;
	unsigned top, bottom;
} blot_margins;

typedef struct blot_dimensions {
	unsigned cols, rows;
} blot_dimensions;

typedef struct blot_strv {
	size_t count;
	char **strings;
} blot_strv;
