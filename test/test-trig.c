#include <glib.h>
#include <stdio.h>
#include <locale.h>
#include <math.h>
#include "blot.h"

#define DATA_COUNT 1000
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

		/* number between -1 and 1 */
		x = (double)(i-(DATA_COUNT/2)) / (DATA_COUNT/2);

		/* number between -2π and 2π */
		x = x * 2 * M_PI;
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

	blot_figure_set_x_limits(fig, -7, 7, &error);
	FATAL_ERROR(error);
	blot_figure_set_y_limits(fig, -2, 2, &error);
	FATAL_ERROR(error);

#if 1
	/* hack for now to add origin lines */

	/* plot X-axis origin */

	gint32 xax[2] = { -7, 7 };
	gint32 xay[2] = { 0, 0 };

	blot_figure_line(fig, BLOT_DATA_INT32,
			 2, xax, xay,
			 8, NULL, &error);
	FATAL_ERROR(error);

	/* plot Y-axis origin */

	gint32 yax[2] = { 0, 0 };
	gint32 yay[2] = { -2, 2 };

	blot_figure_line(fig, BLOT_DATA_INT32,
			 2, yax, yay,
			 8, NULL, &error);
	FATAL_ERROR(error);
#endif

	/* add a scatter plot */

	blot_figure_scatter(fig, BLOT_DATA_DOUBLE,
			    DATA_COUNT, data_xs, sin_ys,
			    data_color, "sin", &error);
	FATAL_ERROR(error);

	/* add a scatter plot */

#if 1
	blot_figure_scatter(fig, BLOT_DATA_DOUBLE,
			    DATA_COUNT, data_xs, cos_ys,
			    data_color+1, "cos", &error);
	FATAL_ERROR(error);
#endif

	/* add a scatter plot */

#if 1
	blot_figure_scatter(fig, BLOT_DATA_DOUBLE,
			    DATA_COUNT, data_xs, tan_ys,
			    data_color+2, "tan", &error);
	FATAL_ERROR(error);
#endif

	/* render the plots */

	blot_render_flags flags = 0;
	flags |= BLOT_RENDER_BRAILLE;
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
