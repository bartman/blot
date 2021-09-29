#include <glib.h>
#include "blot.h"

const gsize data_count = 10;
const guint64 data_xs[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
const guint64 data_ys[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
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

	blot_figure_set_screen_size(fig, 80, 25, &error);
	FATAL_ERROR(error);

	blot_figure_set_x_limits(fig, 0.0, 1000.0, &error);
	FATAL_ERROR(error);
	blot_figure_set_y_limits(fig, 0.0, 1000.0, &error);
	FATAL_ERROR(error);

	blot_figure_scatter(fig, BLOT_LAYER_INT64,
			    data_count, data_xs, data_ys,
			    data_color, data_label, &error);
	FATAL_ERROR(error);

	gsize txt_size = 0;
	char *txt = blot_figure_render(fig, BLOT_FIGURE_RENDER_NONE,
				       &txt_size, &error);
	FATAL_ERROR(error);

	g_print("txt_size=%zu\n", txt_size);
	write(0, txt, txt_size);

	blot_figure_render_free(fig, txt);

	blot_figure_delete(fig);

	return 0;
}
