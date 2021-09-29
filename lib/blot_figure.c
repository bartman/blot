/* blot: figure represents the top level object */
/* vim: set noet sw=8 ts=8 tw=80: */
#include "blot_figure.h"
#include "blot_error.h"
#include "blot_color.h"
#include "blot_layer.h"
#include "blot_terminal.h"

/* create/delete */

bool blot_figure_init(blot_figure *fig, GError **error)
{
	memset(fig, 0, sizeof(*fig));

	/* config */
	fig->axis_color = WHITE;

	return true;
}

blot_figure * blot_figure_new(GError **error)
{
	blot_figure *fig;

	fig = g_new(blot_figure, 1);
	RETURN_ERROR(!fig, NULL, error, "new blot_figure");

	bool ok = blot_figure_init(fig, error);
	RETURN_IF(!ok, NULL);

	return fig;
}

void blot_figure_delete(blot_figure *fig)
{
	g_free(fig);
}


/* configure */

bool blot_figure_set_axis_color(blot_figure *fig, blot_color color, GError **error)
{
	RETURN_EFAULT_IF(fig==NULL, false, error);
	fig->axis_color = color;
	return true;
}


bool blot_figure_set_screen_size(blot_figure *fig,
					unsigned columns, unsigned rows, GError **error)
{
	RETURN_EFAULT_IF(fig==NULL, false, error);

	fig->screen_size_set = true;
	fig->columns = columns;
	fig->rows = rows;
	return true;
}


bool blot_figure_set_x_limits(blot_figure *fig,
				     double x_min, double x_max, GError **error)
{
	RETURN_EFAULT_IF(fig==NULL, false, error);
	RETURN_ERRORx(x_min >= x_max, false, error, EINVAL,
		      "x_min >= x_max");

	fig->x_limits_set = true;
	fig->x_min = x_min;
	fig->x_max = x_max;
	return true;
}

bool blot_figure_set_y_limits(blot_figure *fig,
				     double y_min, double y_max, GError **error)
{
	RETURN_EFAULT_IF(fig==NULL, false, error);
	RETURN_ERRORx(y_min >= y_max, false, error, EINVAL,
		      "y_min >= y_max");

	fig->y_limits_set = true;
	fig->y_min = y_min;
	fig->y_max = y_max;
	return true;
}


/* add layers */

bool blot_figure_scatter(blot_figure *fig, blot_data_type data_type,
				size_t data_count, const void *data_xs, const void *data_ys,
				blot_color data_color, const char *data_label,
				GError **error)
{
	RETURN_EFAULT_IF(fig==NULL, false, error);

	blot_layer *lay = blot_layer_new(BLOT_SCATTER,
					 data_type,
					 data_count,
					 data_xs,
					 data_ys,
					 data_color,
					 data_label,
					 error);
	RETURN_IF(lay==NULL,false);

	return true;
}


/* render */

char * blot_figure_render(blot_figure *fig, enum blot_figure_render_flags flags,
				 gsize *txt_size, GError **error)
{
	RETURN_EFAULT_IF(fig==NULL, NULL, error);

	if (!fig->screen_size_set) {
		bool ok = blot_terminal_get_size(&fig->columns, &fig->rows, error);
		RETURN_IF(!ok, NULL);
	}

	RETURN_EINVAL_IF(fig->columns<BLOT_MIN_COLUMNS, false, error);
	RETURN_EINVAL_IF(fig->rows<BLOT_MIN_ROWS, false, error);





	blot_set_error_unix(error, EINVAL, "TODO");
	return NULL;
}

void blot_figure_render_free(blot_figure *fig, char *txt)
{
	g_free(txt);
}


