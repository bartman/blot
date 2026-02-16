/* blot: various utility macros */
/* vim: set noet sw=8 ts=8 tw=120: */

#include "blot_utils.h"
#include <stdio.h>

unsigned blot_env_to_uint(const char *name, unsigned dflt)
{
	const char *txt = getenv(name);
	if (!txt || !*txt)
		return dflt;

	unsigned long num = strtoul(txt, NULL, 0);
	if (num == ULONG_MAX)
		return dflt;

	return num;
}

int blot_format_number(char *p, unsigned room, double d_val)
{
	static const char **ff, *formats[] = {
		"%.3f", "%.2f", "%.1f", "%g", "%.3g", "%3.g", NULL };
	for (ff = formats; *ff; ff++) {
		int rc = snprintf(p, (unsigned)room, *ff, d_val);
		if (rc>0 && rc < room)
			return rc;
	}
	return -1;
}
