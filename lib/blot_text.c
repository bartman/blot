/* blot: the display width of label text, the same in every locale */
/* vim: set noet sw=8 ts=8 tw=120: */
/* wcwidth() needs _GNU_SOURCE, which the build defines */
#include <stdint.h>
#include <string.h>
#include <wchar.h>
#include <glib.h>
#include "blot_text.h"

struct blot_text_range {
	uint32_t first, last;
};

#include "blot_text_table.h"

static bool blot_text_in(const struct blot_text_range *r, size_t n, uint32_t c)
{
	size_t lo = 0, hi = n;

	if (!n || c < r[0].first || c > r[n-1].last)
		return false;
	while (lo < hi) {
		size_t mid = lo + (hi - lo) / 2;
		if (c > r[mid].last)
			lo = mid + 1;
		else if (c < r[mid].first)
			hi = mid;
		else
			return true;
	}
	return false;
}

int blot_text_builtin_wcwidth(wchar_t wc)
{
	uint32_t c = (uint32_t)wc;

	/* C0, DEL and C1 controls; surrogates and what isn't Unicode */
	if (c < 0x20 || (c >= 0x7f && c < 0xa0))
		return -1;
	if ((c >= 0xd800 && c <= 0xdfff) || c > 0x10ffff)
		return -1;
	if (c < 0x7f)
		return 1;
	if (blot_text_in(blot_text_zero, G_N_ELEMENTS(blot_text_zero), c))
		return 0;
	if (blot_text_in(blot_text_wide, G_N_ELEMENTS(blot_text_wide), c))
		return 2;
	return 1;
}

/* does the locale measure Unicode?  A UTF-8 locale does; the C locale
 * knows nothing past ASCII */
static bool blot_text_locale_measures(void)
{
	return wcwidth(0x4e00) == 2;
}

int blot_text_wcwidth(wchar_t wc)
{
	uint32_t c = (uint32_t)wc;

	if (c < 0x20 || (c >= 0x7f && c < 0xa0))
		return -1;
	if (c < 0x7f)
		return 1;

	int w = wcwidth(wc);
	if (w >= 0)
		return w;
	/* a locale that knows Unicode says it isn't printable */
	if (blot_text_locale_measures())
		return -1;
	return blot_text_builtin_wcwidth(wc);
}

size_t blot_text_next(const char *s, size_t left, bool keep_tab, wchar_t *wc)
{
	if (!left || !*s)
		return 0;

	/* -1 not UTF-8, -2 cut short: either way one byte, one '?' */
	gunichar u = g_utf8_get_char_validated(s, (gssize)left);
	if (u == (gunichar)-1 || u == (gunichar)-2) {
		*wc = L'?';
		return 1;
	}
	size_t n = (size_t)(g_utf8_next_char(s) - s);

	*wc = (wchar_t)u;
	if (!(keep_tab && u == '\t') && blot_text_wcwidth(*wc) < 0)
		*wc = L'?';
	return n;
}

size_t blot_text_prefix(const char *s, unsigned cols, unsigned *width)
{
	size_t left = strlen(s), bytes = 0;
	unsigned col = 0;
	wchar_t wc;
	size_t n;

	while ((n = blot_text_next(s + bytes, left - bytes, false, &wc))) {
		/* a '?' is 1 column, so w >= 0 */
		unsigned w = (unsigned)blot_text_wcwidth(wc);
		if (col + w > cols)
			break;
		col += w;
		bytes += n;
	}
	if (width)
		*width = col;
	return bytes;
}
