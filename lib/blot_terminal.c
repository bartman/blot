/* blot: terminal utilities */
/* vim: set noet sw=8 ts=8 tw=120: */
#include <sys/ioctl.h>
#include <unistd.h>

#include "blot_terminal.h"
#include "blot_error.h"


bool blot_terminal_get_size(int *cols, int *rows, GError **error)
{
	struct winsize w;

	*cols = 0;
	*rows = 0;

	int rc = ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
	RETURN_ERROR(rc<0,false,error,"failed to get terminal size");

	*cols = w.ws_col;
	*rows = w.ws_row;
	return true;
}
