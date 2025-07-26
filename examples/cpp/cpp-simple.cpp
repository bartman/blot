#include <glib.h>
#include <stdio.h>
#include <locale.h>
#include "blot.hpp"

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

int main(void)
{
	setlocale(LC_CTYPE, "");

	/* build a dummy dataset */

    std::vector<double> scat_x(SCAT_COUNT);
    std::vector<double> scat_y(SCAT_COUNT);

	for (int i=0; i<SCAT_COUNT; i++) {
		double f = (double)i / (SCAT_COUNT-1);
		scat_x[i] = DATA_X_MIN + (f * DATA_X_RANGE);
		scat_y[i] = DATA_Y_MIN + (f * DATA_Y_RANGE);
	}

    std::vector<double> line_x = { DATA_X_MIN, DATA_Y_MAX };
    std::vector<double> line_y = { DATA_Y_MAX, DATA_Y_MIN };

	/* configure the figure */

    Blot::Figure fig;

	fig.set_axis_color(8);

	fig.set_screen_size(80, 40);

	fig.set_x_limits(DATA_X_MIN*2, DATA_X_MAX*2);
	fig.set_y_limits(DATA_Y_MIN*2, DATA_Y_MAX*2);

#if 1
	/* hack for now to add origin lines */

	/* plot X-axis origin */

    std::vector<gint32> xax = { DATA_X_MIN*2, DATA_X_MAX*2 };
    std::vector<gint32> xay = { 0, 0 };

	fig.line(xax, xay, 8, NULL);

	/* plot Y-axis origin */

    std::vector<gint32> yax = { 0, 0 };
    std::vector<gint32> yay = {DATA_Y_MIN*2, DATA_Y_MAX*2 };

	fig.line(yax, yay, 8, NULL);
#endif

	/* add a scatter plot */

	fig.scatter(scat_x, scat_y, 9, "scatter");

	/* add a line plot */

	fig.line(line_x, line_y, 10, "line");

	/* render the plots */

	blot_render_flags flags
        = BLOT_RENDER_BRAILLE
        | BLOT_RENDER_LEGEND_BELOW
        | BLOT_RENDER_DONT_INVERT_Y_AXIS;

    Blot::Screen scr = fig.render(flags);

	/* print it to screen */

	gsize txt_size = 0;
	const wchar_t *txt = scr.get_text(txt_size);

	printf("--------------------------------------------------------------------------------\n");
	printf("%ls", txt);
	printf("--------------------------------------------------------------------------------\n");

	return 0;
}
