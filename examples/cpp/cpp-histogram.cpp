/* vim: set noet sw=8 ts=8 tw=120: */
#include <glib.h>
#include <stdint.h>
#include <stdio.h>
#include <locale.h>
#include "blot.hpp"

#define DATA_COUNT 10
#define DATA_MAX   100

int main(void)
{
	setlocale(LC_CTYPE, "");
	srand(0);

	/* build a dummy dataset */

	std::vector<int32_t> data(DATA_COUNT);
	std::vector<std::string> xlabelstrs(DATA_COUNT);
	std::vector<const char *> xlabelptrs(DATA_COUNT);

	for (int i=0; i<DATA_COUNT; i++) {
		data[i] = rand() % DATA_MAX;

		/* let's make up some years */
		xlabelstrs[i] = std::to_string(2000-DATA_COUNT+i);
		xlabelptrs[i] = xlabelstrs[i].c_str();
	}

	/* configure the figure */

	Blot::Figure fig;
	fig.set_axis_color(8);
	fig.set_screen_size(80, 40);

	fig.set_x_axis_labels(xlabelptrs);

	/* add a bar plot */

	fig.bar(data, 9, "histogram");

	/* render the plots */

	blot_render_flags flags
		= BLOT_RENDER_BRAILLE
		| BLOT_RENDER_LEGEND_BELOW;

	Blot::Screen scr = fig.render(flags);

	/* print it to screen */

	gsize txt_size = 0;
	const wchar_t *txt = scr.get_text(txt_size);

	printf("%ls", txt);

	return 0;
}
