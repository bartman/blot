#pragma once

#include <climits>
#include <cstdlib>
#include <cwchar>
#include <string>

/* the display width the library laid a character out with: wcwidth()
 * in the current locale, and 1 where that can't tell (-1), as
 * blot_legend_putc() in lib/blot_screen.c does.  (#18, !36, adds a
 * locale-independent width table to the library; this should use it
 * then) */
inline int plot_char_width(wchar_t wc)
{
	int w = wcwidth(wc);
	return w >= 0 ? w : 1;
}

/* the rendered text in the current locale's multibyte encoding, all of
 * it (#40).  A character the locale can't encode becomes as many '?' as
 * the columns it takes (none for a combining mark, two for a wide
 * one), so every line keeps the width the library laid out and the X
 * labels stay under their ticks.  *replaced tells whether that
 * happened.  width is the width function, for the tests */
inline std::string plot_encode(const wchar_t *txt, size_t len, bool *replaced,
			       int (*width)(wchar_t) = plot_char_width)
{
	std::string out;
	out.reserve(len * MB_CUR_MAX);

	char mb[MB_LEN_MAX];
	mbstate_t state{};
	*replaced = false;
	for (size_t i = 0; i < len && txt[i]; i++) {
		size_t n = wcrtomb(mb, txt[i], &state);
		if (n == (size_t)-1) {
			state = mbstate_t{};
			out.append((size_t)width(txt[i]), '?');
			*replaced = true;
			continue;
		}
		out.append(mb, n);
	}
	return out;
}
