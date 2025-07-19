/* blot: terminal utilities */
/* vim: set noet sw=8 ts=8 tw=120: */
#include <sys/ioctl.h>
#include <unistd.h>

#include "blot_terminal.h"
#include "blot_error.h"
#include "blot_figure.h"

static blot_dimensions blot_fixed_dims = {};

bool blot_terminal_set_size(blot_dimensions dims, GError **error)
{
	RETURN_ERRORx(dims.cols>BLOT_MAX_COLS || dims.cols<BLOT_MIN_COLS,
	       false, error, ERANGE,
	       "invalid columns=%d specified", dims.cols);

	RETURN_ERRORx(dims.rows>BLOT_MAX_ROWS || dims.rows<BLOT_MIN_ROWS,
	       false, error, ERANGE,
	       "invalid rows=%d specified", dims.rows);

	blot_fixed_dims = dims;
	return true;
}

// caller should call blot_terminal_set_size() if the terminal size cannot be determine
bool blot_terminal_get_size(blot_dimensions *dims, GError **error)
{
	RETURN_ERROR_IF(!dims, false, error, EFAULT);

	if (blot_fixed_dims.cols && blot_fixed_dims.rows) {
		*dims = blot_fixed_dims;
		return true;
	}

	struct winsize w = {};
	int rc = ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
	RETURN_ERROR(rc<0,false,error,"failed to get terminal size");

	dims->cols = w.ws_col;
	dims->rows = w.ws_row;
	return true;
}
