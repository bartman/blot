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
	/* the same range as blot_figure_set_screen_size() (blot_figure.h, #17) */
	RETURN_ERRORx(dims.cols<BLOT_MIN_COLS || dims.cols>BLOT_MAX_COLS, false, error, EINVAL,
		      "cols=%u is outside %u..%u", dims.cols, BLOT_MIN_COLS, BLOT_MAX_COLS);
	RETURN_ERRORx(dims.rows<BLOT_MIN_ROWS || dims.rows>BLOT_MAX_ROWS, false, error, EINVAL,
		      "rows=%u is outside %u..%u", dims.rows, BLOT_MIN_ROWS, BLOT_MAX_ROWS);

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

	/* COLUMNS and LINES are used (and kept) when both are set to a
	 * number: one outside the range blot_terminal_set_size() accepts is
	 * clamped into it, as the tty size is.  Unset, 0, garbage or a value
	 * too large for an unsigned: ask the tty */
	unsigned cols = blot_env_to_uint("COLUMNS", 0);
	unsigned rows = blot_env_to_uint("LINES", 0);

	if (cols && rows) {
		dims->cols = min_t(unsigned, max_t(unsigned, cols, BLOT_MIN_COLS), BLOT_MAX_COLS);
		dims->rows = min_t(unsigned, max_t(unsigned, rows, BLOT_MIN_ROWS), BLOT_MAX_ROWS);
		blot_fixed_dims = *dims;
		return true;
	}

	struct winsize w = {};
	int rc = ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
	RETURN_ERROR(rc<0,false,error,"failed to get terminal size");

	dims->cols = w.ws_col;
	dims->rows = w.ws_row;
	return true;
}
