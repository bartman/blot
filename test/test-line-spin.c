#include <glib.h>
#include <math.h>
#include <stdio.h>
#include <locale.h>
#include "blot.h"

#define DATA_X_MAX 1000
#define DATA_Y_MAX 1000

#define LINE_COUNT 4
#define POINT_COUNT 4

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

	/* build axis lines */

	gint32 xax[DATA_X_MAX*2];
	gint32 xay[DATA_X_MAX*2];

	for (int ai=0; ai<(DATA_X_MAX*2); ai++) {
		xax[ai] = ai-DATA_X_MAX;
		xay[ai] = 0;
	}

	gint32 yax[DATA_Y_MAX*2];
	gint32 yay[DATA_Y_MAX*2];

	for (int ai=0; ai<(DATA_Y_MAX*2); ai++) {
		yax[ai] = 0;
		yay[ai] = ai-DATA_Y_MAX;
	}

	/* build a dummy dataset */

	char slab[LINE_COUNT][128];
	char llab[LINE_COUNT][128];
	gint32 xs[LINE_COUNT][POINT_COUNT];
	gint32 ys[LINE_COUNT][POINT_COUNT];

	double xb = DATA_X_MAX / 2;
	double yb = DATA_Y_MAX / 2;

	double offset = 0, angle = 0, ainc = M_PI/LINE_COUNT;
again:
	angle = offset;
	for (int li=0; li<LINE_COUNT; li++) {

		snprintf(slab[li], sizeof(slab[li]), "scatter-%u", li);
		snprintf(llab[li], sizeof(llab[li]), "line-%u", li);

		double xr = xb*cos(angle) - yb*sin(angle);
		double yr = yb*cos(angle) + xb*sin(angle);

		double x0 = xr;
		double y0 = yr;
		double x1 = -xr;
		double y1 = -yr;
		double dx = (x1-x0) / (POINT_COUNT-1);
		double dy = (y1-y0) / (POINT_COUNT-1);

		for (int pi=0; pi<POINT_COUNT; pi++) {

			double x = x0 + (pi * dx);
			double y = y0 + (pi * dy);

			xs[li][pi] = x;
			ys[li][pi] = y;
		}

		angle += ainc;
	}

	/* configure the figure */

	blot_figure *fig;

	fig = blot_figure_new(&error);
	FATAL_ERROR(error);

	blot_figure_set_x_limits(fig, -DATA_X_MAX, DATA_X_MAX, &error);
	FATAL_ERROR(error);
	blot_figure_set_y_limits(fig, -DATA_Y_MAX, DATA_Y_MAX, &error);
	FATAL_ERROR(error);

	/* plot X-axis origin */

	blot_figure_scatter(fig, BLOT_DATA_INT32,
			    DATA_X_MAX*2, xax, xay,
			    8, NULL, &error);
	FATAL_ERROR(error);

	/* plot Y-axis origin */

	blot_figure_scatter(fig, BLOT_DATA_INT32,
			    DATA_Y_MAX*2, yax, yay,
			    8, NULL, &error);
	FATAL_ERROR(error);

	/* add a line plot */

	for (int li=0; li<LINE_COUNT; li++) {

		/* plot as line */

		blot_figure_line(fig, BLOT_DATA_INT32,
				    POINT_COUNT, xs[li], ys[li],
				    1+li, llab[li], &error);
		FATAL_ERROR(error);

		/* plot as scatter */

		blot_figure_scatter(fig, BLOT_DATA_INT32,
				    POINT_COUNT, xs[li], ys[li],
				    9+li, slab[li], &error);
		FATAL_ERROR(error);

	}

	/* render the plots */

	blot_render_flags flags = 0;
	flags |= BLOT_RENDER_BRAILLE;
	flags |= BLOT_RENDER_LEGEND_BELOW;
	flags |= BLOT_RENDER_CLEAR;

	blot_screen *scr = blot_figure_render(fig, flags, &error);
	FATAL_ERROR(error);

	/* print it to screen */

	gsize txt_size = 0;
	const wchar_t *txt = blot_screen_get_text(scr, &txt_size, &error);
	FATAL_ERROR(error);

	printf("%ls", txt);

	blot_screen_delete(scr);

	blot_figure_delete(fig);

	usleep(50000);
	offset += M_PI/18;
	goto again;

	return 0;
}
