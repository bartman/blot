#include <glib.h>
#include "blot.h"

const gsize data_count = 10;
const gint64 data_xs[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
const gint64 data_ys[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
blot_color data_color = 9;
const char *data_label = "data";

#define FATAL_ERROR(error) ({ \
	if (unlikely (error)) \
		g_error("%s:%u: %s", __func__, __LINE__, (error)->message); \
})

int main(void)
{
	g_autoptr(GError) error = NULL;

	blot_figure *fig;

	fig = blot_figure_new(&error);
	FATAL_ERROR(error);

	blot_figure_set_axis_color(fig, 8, &error);
	FATAL_ERROR(error);

	blot_figure_set_screen_size(fig, 40, 20, &error);
	FATAL_ERROR(error);

	blot_figure_set_x_limits(fig, 0.0, 10.0, &error);
	FATAL_ERROR(error);
	blot_figure_set_y_limits(fig, 0.0, 10.0, &error);
	FATAL_ERROR(error);

	blot_figure_scatter(fig, BLOT_LAYER_INT64,
			    data_count, data_xs, data_ys,
			    data_color, data_label, &error);
	FATAL_ERROR(error);

	blot_render_flags flags = 0;
	//flags |= BLOT_RENDER_CLEAR;
	flags |= BLOT_RENDER_BRAILLE;

	blot_screen *scr = blot_figure_render(fig, flags, &error);
	FATAL_ERROR(error);

	gsize txt_size = 0;
	const char *txt = blot_screen_get_text(scr, &txt_size, &error);
	FATAL_ERROR(error);

	g_print("txt_size=%zu\n", txt_size);
	g_print("------------------------------------------------------------\n");
	write(0, txt, txt_size);
	g_print("------------------------------------------------------------\n");

	blot_screen_delete(scr);

	blot_figure_delete(fig);

	return 0;
}
