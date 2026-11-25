/* blot: various utility macros */
/* vim: set noet sw=8 ts=8 tw=120: */

#include "blot_utils.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

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

/* The text of d_val in fewer than room characters, the most precise that fits.
 *
 * Fixed point with 3, 2 or 1 decimals comes first, as before: axis labels
 * of one axis should look alike.  When none of those fits, it takes the most
 * precise of the integer (%.0f) and %.6g down to %.1g, on a tie the first of
 * those.  A text less precise than %.1g (0 for 0.5) isn't used.  So a value
 * keeps at least two significant digits when %.2g fits, and an integer is
 * printed in full when it fits.  From 2^53 on, the digits of %.0f past the
 * 16th or 17th are only binary noise, so it is skipped there.
 *
 * Returns the length, or -1 when nothing fits; nothing is written at or
 * past p[room].  No static state, so it's thread-safe. */
int blot_format_number(char *p, unsigned room, double d_val)
{
	static const char *const fixed[] = { "%.3f", "%.2f", "%.1f" };
	static const char *const others[] = {
		"%.0f", "%.6g", "%.5g", "%.4g", "%.3g", "%.2g", "%.1g" };
	int rc;

	for (size_t i = 0; i < sizeof(fixed)/sizeof(fixed[0]); i++) {
		rc = snprintf(p, room, fixed[i], d_val);
		if (rc>0 && (unsigned)rc < room)
			return rc;
	}

	/* nan and inf only have one text, and it didn't fit */
	if (!isfinite(d_val))
		return -1;

	/* the error of %.1g: no text may be further off than that */
	char one[16];
	snprintf(one, sizeof(one), "%.1g", d_val);
	double best_err = fabs(strtod(one, NULL) - d_val);
	const char *best = NULL;

	for (size_t i = 0; i < sizeof(others)/sizeof(others[0]); i++) {
		if (i == 0 && fabs(d_val) >= 0x1p53)
			continue;
		rc = snprintf(NULL, 0, others[i], d_val);
		if (rc<=0 || (unsigned)rc >= room)
			continue;
		snprintf(p, room, others[i], d_val);
		double err = fabs(strtod(p, NULL) - d_val);
		if (err <= best_err && (!best || err < best_err)) {
			best = others[i];
			best_err = err;
		}
	}
	if (!best)
		return -1;

	return snprintf(p, room, best, d_val);
}
