#include <glib.h>
#include <locale.h>
#include <math.h>
#include "blot.h"

#define DATA_COUNT 100
static gint64 data_xs[DATA_COUNT];
static gint64 sin_ys[DATA_COUNT];
static gint64 cos_ys[DATA_COUNT];

#define DATA_X_MAX 1000
#define DATA_Y_MAX 1000

blot_color data_color = 9;
const char *data_label = "data";

#define FATAL_ERROR(error) ({ \
	if (unlikely (error)) \
		g_error("%s:%u: %s", __func__, __LINE__, (error)->message); \
})

int main(void)
{
	g_autoptr(GError) error = NULL;

	setlocale(LC_CTYPE, "");

	unsigned offset = 0;
again:

	/* build a dummy dataset */

	for (int i=0; i<DATA_COUNT; i++) {
		double x = (double)(i+offset) * 2 * M_PI / DATA_COUNT;

		double y = 1 + sin(x);

		data_xs[i] = i * DATA_X_MAX / DATA_COUNT;
		sin_ys[i] = y * DATA_Y_MAX / 2;

		y = 1 + cos(x);
		cos_ys[i] = y * DATA_Y_MAX / 2;
	}

	/* configure the figure */

	blot_figure *fig;

	fig = blot_figure_new(&error);
	FATAL_ERROR(error);

	blot_figure_set_axis_color(fig, 8, &error);
	FATAL_ERROR(error);

	blot_figure_set_screen_size(fig, 40, 20, &error);
	FATAL_ERROR(error);

	blot_figure_set_x_limits(fig, 0.0, DATA_X_MAX, &error);
	FATAL_ERROR(error);
	blot_figure_set_y_limits(fig, 0.0, DATA_Y_MAX, &error);
	FATAL_ERROR(error);

	/* add a scatter plot */

	blot_figure_scatter(fig, BLOT_LAYER_INT64,
			    DATA_COUNT, data_xs, sin_ys,
			    data_color, data_label, &error);
	FATAL_ERROR(error);

	/* add a scatter plot */

	blot_figure_scatter(fig, BLOT_LAYER_INT64,
			    DATA_COUNT, data_xs, cos_ys,
			    data_color, data_label, &error);
	FATAL_ERROR(error);

	/* render the plots */

	blot_render_flags flags = 0;
	//flags |= BLOT_RENDER_CLEAR;
	flags |= BLOT_RENDER_BRAILLE;

	blot_screen *scr = blot_figure_render(fig, flags, &error);
	FATAL_ERROR(error);

	/* print it to screen */

	gsize txt_size = 0;
	const gunichar *txt = blot_screen_get_text(scr, &txt_size, &error);
	FATAL_ERROR(error);

	g_print("%s%ls", CLR_SCR, txt);
	usleep(10000);

	blot_screen_delete(scr);

	blot_figure_delete(fig);

	offset ++;
	goto again;

	return 0;
}
