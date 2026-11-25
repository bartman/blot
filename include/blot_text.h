/* blot: the display width of label text, the same in every locale */
/* vim: set noet sw=8 ts=8 tw=120: */
#pragma once

/* Internal: for the library, its tests and blot's own CLI (cli/
 * plot_encode.hpp counts the '?'s for a character the locale can't encode
 * with blot_text_wcwidth()), not a stable API.  The functions are exported
 * (BLOT_API) so that the tests and the CLI, which link libblot.so, can
 * call them; they may change or go away in any release. */

#include <stdbool.h>
#include <stddef.h>
#include <wchar.h>

#include "blot_compiler.h"

BLOT_EXTERN_C_START

/* Labels (legend, custom X labels) are UTF-8 in every locale, and the
 * screen is wchar_t (UCS-4) text.  Their width is counted the same way in
 * every locale (#18): a locale that knows Unicode (UTF-8) measures a
 * character with wcwidth(); one that can't (the C locale, where wcwidth()
 * is -1 for anything past ASCII) gets the built-in table, generated from
 * glibc's C.UTF-8, so the two agree.
 *
 * A character blot can't show becomes one '?', which takes one column:
 * a byte that isn't UTF-8, a control, a code point that isn't a printable
 * character (unassigned).  A caller that replaces a character it can't
 * output (#40, #46) should keep the width blot_text_wcwidth() gives it:
 * one '?' per column, none for a zero-width one. */

/* The columns wc takes on a terminal: 0 (a combining mark), 1, or 2 (East
 * Asian Wide or Fullwidth, emoji); -1 for a control or a code point that
 * isn't a printable character. */
BLOT_API int blot_text_wcwidth(wchar_t wc);

/* The same from the built-in table alone, whatever the locale. */
BLOT_API int blot_text_builtin_wcwidth(wchar_t wc);

/* The next character of the UTF-8 text s, of which left bytes remain:
 * sets *wc and returns the bytes it takes, or 0 at the end (no bytes left,
 * or a NUL).  A byte that isn't valid UTF-8 becomes one '?', and so does a
 * control or a code point that isn't printable (a tab too, unless
 * keep_tab), so a label can't break its line or send escape sequences. */
BLOT_API size_t blot_text_next(const char *s, size_t left, bool keep_tab, wchar_t *wc);

/* At most this many zero-width marks are kept after a character (and at
 * the start of a label); more are dropped, so that a label stuffed with
 * combining marks can't fill the screen buffer. */
#define BLOT_TEXT_MAX_MARKS 4

/* Reads a label the way it is shown: blot_text_next(), less the marks
 * past BLOT_TEXT_MAX_MARKS. */
struct blot_text_reader {
	const char *s;		/* the rest of the label */
	size_t left;		/* its bytes */
	bool keep_tab;
	unsigned marks;		/* zero-width marks kept since the last character */
};

BLOT_API void blot_text_reader_init(struct blot_text_reader *r, const char *s, bool keep_tab);

/* The next character to show: sets *wc and *width (blot_text_wcwidth();
 * 0 for a kept tab) and returns true, or false at the end. */
BLOT_API bool blot_text_read(struct blot_text_reader *r, wchar_t *wc, unsigned *width);

/* The bytes of the longest start of the UTF-8 label s that takes at most
 * cols columns, characters as blot_text_read() shows them (a tab is a
 * '?'), with the zero-width marks that follow the last one.  *width
 * (unless NULL) is set to the columns it takes. */
BLOT_API size_t blot_text_prefix(const char *s, unsigned cols, unsigned *width);

BLOT_EXTERN_C_END
