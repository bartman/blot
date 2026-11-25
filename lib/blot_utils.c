/* blot: various utility macros */
/* vim: set noet sw=8 ts=8 tw=120: */

#include "blot_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>

/* the value of the environment variable name as an unsigned (decimal, or
 * octal/hex with a 0/0x prefix, surrounding blanks allowed), or dflt when it
 * is unset, empty, negative, not a number, has trailing garbage or does not
 * fit in an unsigned */
unsigned blot_env_to_uint(const char *name, unsigned dflt)
{
	const char *txt = getenv(name);
	if (!txt || !*txt)
		return dflt;

	const char *p = txt;
	while (isspace((unsigned char)*p))
		p++;
	/* strtoul() accepts a sign and negates the value */
	if (!isdigit((unsigned char)*p))
		return dflt;

	char *end = NULL;
	errno = 0;
	unsigned long num = strtoul(p, &end, 0);
	if (errno || end == p || num > UINT_MAX)
		return dflt;

	while (isspace((unsigned char)*end))
		end++;
	if (*end)
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
