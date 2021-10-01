#include <glib.h>
#include <stdio.h>
#include <locale.h>
#include <math.h>
#include "blot.h"

#define DATA_COUNT 100
static double data_xs[DATA_COUNT];
static double sin_ys[DATA_COUNT];
static double cos_ys[DATA_COUNT];
static double tan_ys[DATA_COUNT];

blot_color data_color = 9;

#define FATAL_ERROR(error) ({ \
	if (unlikely (error)) \
		g_error("%s:%u: %s", __func__, __LINE__, (error)->message); \
})

int main(void)
{
	g_autoptr(GError) error = NULL;

	setlocale(LC_CTYPE, "");

	/* build a dummy dataset */

	for (int i=0; i<DATA_COUNT; i++) {
		double x, y;

		x = (double)(i-(DATA_COUNT/2));
		x = x * 8 * M_PI / DATA_COUNT;
		data_xs[i] = x;

		y = sin(x);
		sin_ys[i] = y;

		y = cos(x);
		cos_ys[i] = y;

		y = tan(x);
		tan_ys[i] = y;

	}

	/* configure the figure */

	blot_figure *fig;

	fig = blot_figure_new(&error);
	FATAL_ERROR(error);

	blot_figure_set_axis_color(fig, 8, &error);
	FATAL_ERROR(error);

	blot_figure_set_screen_size(fig, 80, 40, &error);
	FATAL_ERROR(error);

#if 0
	blot_figure_set_x_limits(fig, -10, 10, &error);
	FATAL_ERROR(error);
#endif
	blot_figure_set_y_limits(fig, -2, 2, &error);
	FATAL_ERROR(error);

	/* add a scatter plot */

	blot_figure_scatter(fig, BLOT_DATA_DOUBLE,
			    DATA_COUNT, data_xs, sin_ys,
			    data_color, "sin", &error);
	FATAL_ERROR(error);

	/* add a scatter plot */

#if 0
	blot_figure_scatter(fig, BLOT_DATA_(INT32,DOUBLE),
			    DATA_COUNT, data_xs, cos_ys,
			    data_color+1, "cos", &error);
	FATAL_ERROR(error);
#endif

	/* add a scatter plot */

#if 0
	blot_figure_scatter(fig, BLOT_DATA_(INT32,DOUBLE),
			    DATA_COUNT, data_xs, tan_ys,
			    data_color+2, "tan", &error);
	FATAL_ERROR(error);
#endif

	/* render the plots */

	blot_render_flags flags = 0;
	flags |= BLOT_RENDER_BRAILLE;
	flags |= BLOT_RENDER_DONT_INVERT_Y_AXIS;
	flags |= BLOT_RENDER_LEGEND_ABOVE;

	blot_screen *scr = blot_figure_render(fig, flags, &error);
	FATAL_ERROR(error);

	/* print it to screen */

	gsize txt_size = 0;
	const wchar_t *txt = blot_screen_get_text(scr, &txt_size, &error);
	FATAL_ERROR(error);

	printf("%ls\n", txt);

	blot_screen_delete(scr);

	blot_figure_delete(fig);

	return 0;
}
