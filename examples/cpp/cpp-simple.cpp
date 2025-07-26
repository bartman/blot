#include <glib.h>
#include <stdio.h>
#include <locale.h>
#include "blot.h"

#define DATA_X_MIN -1
#define DATA_X_MAX 1
#define DATA_X_RANGE (DATA_X_MAX - DATA_X_MIN)

#define DATA_Y_MIN -1
#define DATA_Y_MAX 1
#define DATA_Y_RANGE (DATA_Y_MAX - DATA_Y_MIN)

#define SCAT_COUNT 9
#define LINE_COUNT 2

#define SCREEN_WIDTH  80
#define SCREEN_HEIGHT 25

#define FATAL_ERROR(error) ({ \
	if (unlikely (error)) \
		g_error("%s:%u: %s", __func__, __LINE__, (error)->message); \
})

int main(void)
{
	g_autoptr(GError) error = NULL;

	setlocale(LC_CTYPE, "");

	/* build a dummy dataset */

	static double scat_x[SCAT_COUNT];
	static double scat_y[SCAT_COUNT];

	for (int i=0; i<SCAT_COUNT; i++) {
		double f = (double)i / (SCAT_COUNT-1);
		scat_x[i] = DATA_X_MIN + (f * DATA_X_RANGE);
		scat_y[i] = DATA_Y_MIN + (f * DATA_Y_RANGE);
	}

	static double line_x[LINE_COUNT] = { DATA_X_MIN, DATA_Y_MAX };
	static double line_y[LINE_COUNT] = { DATA_Y_MAX, DATA_Y_MIN };

	/* configure the figure */

	blot_figure *fig;

	fig = blot_figure_new(&error);
	FATAL_ERROR(error);

	blot_figure_set_axis_color(fig, 8, &error);
	FATAL_ERROR(error);

	blot_figure_set_screen_size(fig, 80, 40, &error);
	FATAL_ERROR(error);

	blot_figure_set_x_limits(fig, DATA_X_MIN*2, DATA_X_MAX*2, &error);
	FATAL_ERROR(error);
	blot_figure_set_y_limits(fig, DATA_Y_MIN*2, DATA_Y_MAX*2, &error);
	FATAL_ERROR(error);

#if 1
	/* hack for now to add origin lines */

	/* plot X-axis origin */

	gint32 xax[2] = { DATA_X_MIN*2, DATA_X_MAX*2 };
	gint32 xay[2] = { 0, 0 };

	blot_figure_line(fig, BLOT_DATA_INT32,
			 2, xax, xay,
			 8, NULL, &error);
	FATAL_ERROR(error);

	/* plot Y-axis origin */

	gint32 yax[2] = { 0, 0 };
	gint32 yay[2] = {DATA_Y_MIN*2, DATA_Y_MAX*2 };

	blot_figure_line(fig, BLOT_DATA_INT32,
			 2, yax, yay,
			 8, NULL, &error);
	FATAL_ERROR(error);
#endif

	/* add a scatter plot */

	blot_figure_scatter(fig, BLOT_DATA_DOUBLE,
			    SCAT_COUNT, scat_x, scat_y,
			    9, "scatter", &error);
	FATAL_ERROR(error);

	/* add a line plot */

	blot_figure_line(fig, BLOT_DATA_DOUBLE,
			    LINE_COUNT, line_x, line_y,
			    10, "line", &error);
	FATAL_ERROR(error);

	/* render the plots */

	blot_render_flags flags
        = BLOT_RENDER_BRAILLE
        | BLOT_RENDER_LEGEND_BELOW
        | BLOT_RENDER_DONT_INVERT_Y_AXIS;

	blot_screen *scr = blot_figure_render(fig, flags, &error);
	FATAL_ERROR(error);

	/* print it to screen */

	gsize txt_size = 0;
	const wchar_t *txt = blot_screen_get_text(scr, &txt_size, &error);
	FATAL_ERROR(error);

	printf("--------------------------------------------------------------------------------\n");
	printf("%ls", txt);
	printf("--------------------------------------------------------------------------------\n");

	blot_screen_delete(scr);

	blot_figure_delete(fig);

	return 0;
}
