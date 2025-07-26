/* vim: set noet sw=8 ts=8 tw=120: */
#include <glib.h>
#include <stdio.h>
#include <locale.h>

#include "blot.hpp"

#define DATA_COUNT 10000

#define DATA_X_MAX 10000
#define DATA_Y_MAX 10000

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

	std::vector<gint64> data;
	data.resize(DATA_COUNT);

	for (int i=0; i<DATA_COUNT; i++) {
		data[i] = i;
	}

	std::vector<gint32> line_x = { 0, DATA_Y_MAX };
	std::vector<gint32>  line_y = { DATA_X_MAX, 0 };

	/* configure the figure */

	Blot::Figure fig;

	/* add a scatter plot */

	fig.scatter(data, 9, "scatter");

	/* add a line plot */

	fig.line(line_x, line_y, 10, "line");

	/* render the plots */

	blot_render_flags flags
		= BLOT_RENDER_BRAILLE
		| BLOT_RENDER_LEGEND_BELOW
		| BLOT_RENDER_NO_X_AXIS
		| BLOT_RENDER_NO_Y_AXIS;

	Blot::Screen scr = fig.render(flags);

	/* print it to screen */

	gsize txt_size = 0;
	const wchar_t *txt = scr.get_text(txt_size);

	printf("%ls", txt);

	return 0;
}
