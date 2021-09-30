#include <glib.h>
#include <stdio.h>
#include <locale.h>
#include "blot.h"

#define DATA_COUNT 10000
static gint64 data_xs[DATA_COUNT];
static gint64 data_ys[DATA_COUNT];

#define DATA_X_MAX 10000
#define DATA_Y_MAX 10000

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

	for (int i=0; i<DATA_COUNT; i++) {
		data_xs[i] = i;
		data_ys[i] = i;
	}

	/* configure the figure */

	blot_figure *fig;

	fig = blot_figure_new(&error);
	FATAL_ERROR(error);

	/* add a scatter plot */

	blot_figure_scatter(fig, BLOT_LAYER_INT64,
			    DATA_COUNT, data_xs, data_ys,
			    9, "data", &error);
	FATAL_ERROR(error);

	/* render the plots */

	blot_render_flags flags = 0;
	flags |= BLOT_RENDER_BRAILLE;
	flags |= BLOT_RENDER_LEGEND_BELOW;

	blot_screen *scr = blot_figure_render(fig, flags, &error);
	FATAL_ERROR(error);

	/* print it to screen */

	gsize txt_size = 0;
	const wchar_t *txt = blot_screen_get_text(scr, &txt_size, &error);
	FATAL_ERROR(error);

	printf("%ls", txt);

	blot_screen_delete(scr);

	blot_figure_delete(fig);

	return 0;
}
