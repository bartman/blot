/* blot: terminal utilities */
/* vim: set noet sw=8 ts=8 tw=80: */
#include <sys/ioctl.h>
#include <stdio.h>
#include <unistd.h>

#include "blot_terminal.h"
#include "blot_error.h"


bool blot_terminal_get_size(unsigned *columns, unsigned *rows, GError **error)
{
	struct winsize w;
	int rc = ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
	RETURN_ERROR(rc<0,false,error,"failed to get terminal size");

	*columns = w.ws_col;
	*rows = w.ws_row;
	return true;
}
