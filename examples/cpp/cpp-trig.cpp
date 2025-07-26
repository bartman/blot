/* vim: set noet sw=8 ts=8 tw=120: */
#include <glib.h>
#include <stdio.h>
#include <locale.h>
#include <math.h>

#include <vector>
#include <array>

#include "blot.hpp"

#define DATA_COUNT 1000
static std::vector<double> data_xs(DATA_COUNT);
static std::vector<double> sin_ys(DATA_COUNT);
static std::vector<double> cos_ys(DATA_COUNT);
static std::vector<double> tan_ys(DATA_COUNT);

#define X_MIN -7
#define X_MAX 7
#define Y_MIN -2
#define Y_MAX 2

blot_color data_color = 9;

int main(void)
{
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

	Blot::Figure fig;

	fig.set_axis_color(8);

	fig.set_screen_size(80, 40);

	fig.set_x_limits(X_MIN, X_MAX);
	fig.set_y_limits(Y_MIN, Y_MAX);

#if 1
	/* hack for now to add origin lines */

	/* plot X-axis origin */

	std::array<std::vector<gint32>,5> xax;
	std::array<std::vector<gint32>,5> xay;

	for (int i=Y_MIN; i<=Y_MAX; i++) {
		xax[Y_MAX+i] = { X_MIN, X_MAX };
		xay[Y_MAX+i] = { i, i };

		fig.line(xax[Y_MAX+i], xay[Y_MAX+i], i==0 ? 15 : 8, NULL);
	}

	/* plot Y-axis origin */

	std::array<std::vector<gint32>,15> yax;
	std::array<std::vector<gint32>,15> yay;

	for (int i=X_MIN; i<=X_MAX; i++) {
		yax[X_MAX+i] = { i, i };
		yay[X_MAX+i] = { Y_MIN, Y_MAX };

		fig.line(yax[X_MAX+i], yay[X_MAX+i], i==0 ? 15 : 8, NULL);
	}
	/* plot Y-axis origin */

	std::array<std::vector<double>,15> ypx;
	std::array<std::vector<gint32>,15> ypy;

	for (int i=Y_MIN; i<=Y_MAX; i++) {
		ypx[Y_MAX+i] = { i * M_PI, i * M_PI };
		ypy[Y_MAX+i] = { Y_MIN, Y_MAX };

		fig.line(ypx[Y_MAX+i], ypy[Y_MAX+i], 15, NULL);
	}
#endif

	/* add a scatter plot */

	fig.scatter(data_xs, sin_ys, data_color, "sin");

	/* add a scatter plot */

#if 1
	fig.scatter(data_xs, cos_ys, data_color+1, "cos");
#endif

	/* add a scatter plot */

#if 1
	fig.scatter(data_xs, tan_ys, data_color+2, "tan");
#endif

	/* render the plots */

	blot_render_flags flags
		= BLOT_RENDER_BRAILLE
		| BLOT_RENDER_LEGEND_ABOVE;

	Blot::Screen scr = fig.render(flags);

	/* print it to screen */

	gsize txt_size = 0;
	const wchar_t *txt = scr.get_text(txt_size);

	printf("%ls\n", txt);

	return 0;
}
