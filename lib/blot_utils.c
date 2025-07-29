/* blot: various utility macros */
/* vim: set noet sw=8 ts=8 tw=120: */

#include "blot_utils.h"

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
