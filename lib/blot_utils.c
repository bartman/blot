/* blot: various utility macros */
/* vim: set noet sw=8 ts=8 tw=120: */

#include "blot_utils.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdbool.h>

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

/* how far the text p is from d_val */
static double blot_format_err(const char *p, double d_val)
{
	return fabs(strtod(p, NULL) - d_val);
}

/* The text of d_val in fewer than room characters, the most precise that fits.
 *
 * Fixed point with 3, 2 or 1 decimals comes first, as before: axis labels
 * of one axis should look alike.  When none of those fits, it takes the most
 * precise of the integer (%.0f) and %.6g down to %.1g, on a tie the first of
 * those.  A text less precise than %.1g isn't used, fixed point included:
 * not 0 for 0.5, nor 0.000 for 0.0004 (#10), which keeps its digit as
 * 0.0004.  So a value keeps at least two significant digits when %.2g
 * fits, and an integer is printed in full when it fits.  From 2^53 on, the
 * digits of %.0f past the 16th or 17th are only binary noise, so it is
 * skipped there.  Zero is never negative: -0.0 is 0.000.
 *
 * Returns the length, or -1 when nothing fits; nothing is written at or
 * past p[room].  No static state, so it's thread-safe. */
int blot_format_number(char *p, unsigned room, double d_val)
{
	return blot_format_number_within(p, room, d_val, INFINITY);
}

/* blot_format_number(), but a text should also be within tol of d_val:
 * an axis label within half a cell of its tick (#10).
 *
 * A fixed point text that is further off isn't used (0.001 for 0.0014
 * when a cell is 2e-5).  When fixed point fits but isn't precise enough,
 * the precision comes from tol: the fewest significant digits, %.1g to
 * %.17g, within tol (0.0014, 1.4e-09).  When no fixed point fits (large
 * values), the most precise text of blot_format_number() if it is within
 * tol, as before (-8.016e+299), else again the fewest digits within tol
 * (1000000.0002).  When no text that fits is within tol, the most precise
 * that fits.  Returns as blot_format_number(). */
int blot_format_number_within(char *p, unsigned room, double d_val, double tol)
{
	static const char *const fixed[] = { "%.3f", "%.2f", "%.1f" };
	static const char *const others[] = {
		"%.0f", "%.6g", "%.5g", "%.4g", "%.3g", "%.2g", "%.1g" };
	int rc;

	/* never a negative zero; no tolerance is none at all */
	if (d_val == 0)
		d_val = 0;
	if (isnan(tol))
		tol = INFINITY;

	/* nan and inf only have one text */
	if (!isfinite(d_val)) {
		rc = snprintf(p, room, "%f", d_val);
		return rc>0 && (unsigned)rc < room ? rc : -1;
	}

	/* the error of %.1g: no text may be further off than that */
	char one[16];
	snprintf(one, sizeof(one), "%.1g", d_val);
	double one_err = blot_format_err(one, d_val);

	bool fixed_fits = false;
	for (size_t i = 0; i < sizeof(fixed)/sizeof(fixed[0]); i++) {
		rc = snprintf(p, room, fixed[i], d_val);
		if (rc>0 && (unsigned)rc < room) {
			double err = blot_format_err(p, d_val);
			if (err <= one_err && err <= tol)
				return rc;
			fixed_fits = true;
		}
	}

	/* the most precise of the integer and %.6g..%.1g */
	double best_err = one_err;
	const char *best = NULL;

	for (size_t i = 0; i < sizeof(others)/sizeof(others[0]); i++) {
		if (i == 0 && fabs(d_val) >= 0x1p53)
			continue;
		rc = snprintf(NULL, 0, others[i], d_val);
		if (rc<=0 || (unsigned)rc >= room)
			continue;
		snprintf(p, room, others[i], d_val);
		double err = blot_format_err(p, d_val);
		if (err <= best_err && (!best || err < best_err)) {
			best = others[i];
			best_err = err;
		}
	}

	if (best && (isinf(tol) || (!fixed_fits && best_err <= tol)))
		return snprintf(p, room, best, d_val);
	if (isinf(tol))
		return -1;

	/* the fewest significant digits within tol; on the way, the most
	 * precise that fits */
	int best_digits = 0;
	bool have = best != NULL;
	for (int digits = 1; digits <= 17; digits++) {
		rc = snprintf(NULL, 0, "%.*g", digits, d_val);
		if (rc<=0 || (unsigned)rc >= room)
			continue;
		snprintf(p, room, "%.*g", digits, d_val);
		double err = blot_format_err(p, d_val);
		if (err <= tol)
			return rc;
		if (err < best_err || (!have && err <= best_err)) {
			best_digits = digits;
			best_err = err;
			have = true;
		}
	}

	if (best_digits)
		return snprintf(p, room, "%.*g", best_digits, d_val);
	if (best)
		return snprintf(p, room, best, d_val);
	return -1;
}
