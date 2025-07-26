/* vim: set noet sw=8 ts=8 tw=120: */
#include <glib.h>
#include <stdio.h>
#include <locale.h>
#include <math.h>
#include "blot.hpp"

#define DATA_COUNT 100000
static std::vector<gint32> data_xs(DATA_COUNT);
static std::vector<double> sin_ys(DATA_COUNT);
static std::vector<gint32> cos_ys(DATA_COUNT);
static std::vector<gint64> tan_ys(DATA_COUNT);

#define DATA_X_MAX 100000
#define DATA_Y_MAX 100000

blot_color data_color = 9;

static bool signaled = false;
static void sighandler(int sig)
{
	signaled = true;
}

int main(void)
{
	setlocale(LC_CTYPE, "");

	signal(SIGINT, sighandler);

	unsigned offset = 0;
	double t_render_total=0, t_display_total=0;
	long iterations = 0;

again:

	/* build a dummy dataset */

	for (int i=0; i<DATA_COUNT; i++) {
		double x = (double)(i+offset) * 2 * M_PI / DATA_COUNT;

		double y = 1 + sin(x);

		data_xs[i] = (double)i * DATA_X_MAX / DATA_COUNT;
		sin_ys[i] = y * DATA_Y_MAX / 2;

		y = 1 + cos(x);
		cos_ys[i] = y * DATA_Y_MAX / 2;

		y = 1 + tan(x);
		tan_ys[i] = y * DATA_Y_MAX / 2;
	}

	/* start the time */

	double t_start = blot_double_time();

	/* configure the figure */

	Blot::Figure fig;

	fig.set_axis_color(8);

	fig.set_screen_size(80, 20);

	fig.set_x_limits(0.0, DATA_X_MAX);
	fig.set_y_limits(0.0, DATA_Y_MAX);

	/* add a scatter plot */

	fig.scatter( data_xs, sin_ys, data_color, "sin");

	/* add a scatter plot */

	fig.scatter(data_xs, cos_ys, data_color+1, "cos");

	/* add a scatter plot */

	fig.scatter(data_xs, tan_ys, data_color+2, "tan");

	/* render the plots */

	blot_render_flags flags
		= BLOT_RENDER_CLEAR
		| BLOT_RENDER_BRAILLE
		| BLOT_RENDER_DONT_INVERT_Y_AXIS
		| BLOT_RENDER_LEGEND_ABOVE
		| BLOT_RENDER_NO_X_AXIS
		| BLOT_RENDER_NO_Y_AXIS;

	Blot::Screen scr = fig.render(flags);

	/* print it to screen */

	gsize txt_size = 0;
	const wchar_t *txt = scr.get_text(txt_size);

	double t_render = blot_double_time();

	printf("%ls\n", txt);

	double t_end = blot_double_time();

	t_render_total += t_render-t_start;
	t_display_total += t_end-t_render;
	iterations ++;

	printf("last: render=%.6f show=%.6f\n"
	"mean: render=%.6f show=%.6f\n",
	t_render-t_start, t_end-t_render,
	t_render_total / iterations, t_display_total / iterations);
	usleep(50000);

	offset += DATA_COUNT/100;

	if (!signaled)
		goto again;

	return 0;
}
