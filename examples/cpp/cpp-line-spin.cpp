#include <glib.h>
#include <math.h>
#include <stdio.h>
#include <locale.h>

#include <vector>
#include <array>

#include "blot.hpp"

#define DATA_X_MAX 1000
#define DATA_Y_MAX 1000

#define LINE_COUNT 4
#define POINT_COUNT 4

static bool signaled = false;
static void sighandler(int sig)
{
	signaled = true;
}

int main(void)
{
	setlocale(LC_CTYPE, "");

	signal(SIGINT, sighandler);

	/* build axis lines */

    std::vector<gint32> xax(DATA_X_MAX*2);
    std::vector<gint32> xay(DATA_X_MAX*2);

	for (int ai=0; ai<(DATA_X_MAX*2); ai++) {
		xax[ai] = ai-DATA_X_MAX;
		xay[ai] = 0;
	}

    std::vector<gint32> yax(DATA_Y_MAX*2);
    std::vector<gint32> yay(DATA_Y_MAX*2);

	for (int ai=0; ai<(DATA_Y_MAX*2); ai++) {
		yax[ai] = 0;
		yay[ai] = ai-DATA_Y_MAX;
	}

	/* build a dummy dataset */

	char slab[LINE_COUNT][128];
	char llab[LINE_COUNT][128];
    std::array<std::vector<gint32>,LINE_COUNT> xs;
    std::array<std::vector<gint32>,LINE_COUNT> ys;
	for (int li=0; li<LINE_COUNT; li++) {
        xs[li].resize(POINT_COUNT);
        ys[li].resize(POINT_COUNT);
    }

	double xb = DATA_X_MAX / 2;
	double yb = DATA_Y_MAX / 2;

	/* learn the screen */
    Blot::Dimensions term;

	if (term.rows <= (7+LINE_COUNT))
		g_error("screen is not tall enough");

	double offset = 0, angle = 0, ainc = M_PI/LINE_COUNT;
	double t_render_total=0, t_display_total=0;
	long iterations = 0;

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

	/* start the time */

	double t_start = blot_double_time();

	/* configure the figure */

    Blot::Figure fig;

	fig.set_screen_size(term.cols, term.rows-7-LINE_COUNT);

	fig.set_x_limits(-DATA_X_MAX, DATA_X_MAX);
	fig.set_y_limits(-DATA_Y_MAX, DATA_Y_MAX);

	/* plot X-axis origin */

	fig.scatter(xax, xay, 8, NULL);

	/* plot Y-axis origin */

	fig.scatter(yax, yay, 8, NULL);

	/* add a line plot */

	for (int li=0; li<LINE_COUNT; li++) {

		/* plot as line */

		fig.line(xs[li], ys[li], 1+li, llab[li]);

		/* plot as scatter */

		fig.scatter(xs[li], ys[li], 9+li, slab[li]);

	}

	/* render the plots */

	blot_render_flags flags
        = BLOT_RENDER_BRAILLE
        | BLOT_RENDER_LEGEND_BELOW
        | BLOT_RENDER_CLEAR
        | BLOT_RENDER_NO_X_AXIS
        | BLOT_RENDER_NO_Y_AXIS;

    Blot::Screen scr = fig.render(flags);

	/* print it to screen */

	gsize txt_size = 0;
	const wchar_t *txt = scr.get_text(txt_size);

	double t_render = blot_double_time();

	printf("%ls", txt);

	double t_end = blot_double_time();

	t_render_total += t_render-t_start;
	t_display_total += t_end-t_render;
	iterations ++;

	printf("last: render=%.6f show=%.6f\n"
		"mean: render=%.6f show=%.6f\n",
		t_render-t_start, t_end-t_render,
		t_render_total / iterations, t_display_total / iterations);
	usleep(50000);

	offset += M_PI/18;

	if (!signaled)
		goto again;

    puts("\nDONE");

	return 0;
}
