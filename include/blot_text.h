/* blot: the display width of label text, the same in every locale */
/* vim: set noet sw=8 ts=8 tw=120: */
#pragma once

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
 * glibc's C.UTF-8, so the two agree. */

/* The columns wc takes on a terminal: 0 (a combining mark), 1, or 2 (East
 * Asian Wide or Fullwidth, emoji); -1 for a control or a character that
 * isn't printable. */
BLOT_API int blot_text_wcwidth(wchar_t wc);

/* The same from the built-in table alone, whatever the locale. */
BLOT_API int blot_text_builtin_wcwidth(wchar_t wc);

/* The next character of the UTF-8 text s, of which left bytes remain:
 * sets *wc and returns the bytes it takes, or 0 at the end (no bytes left,
 * or a NUL).  A byte that isn't valid UTF-8 becomes one '?', and so does a
 * control or a character that isn't printable (a tab too, unless keep_tab),
 * so a label can't break its line or send escape sequences. */
BLOT_API size_t blot_text_next(const char *s, size_t left, bool keep_tab, wchar_t *wc);

/* The bytes of the longest start of the UTF-8 label s that takes at most
 * cols columns, characters as blot_text_next() reads them (a tab is a
 * '?'), with the zero-width marks that follow the last one.  *width
 * (unless NULL) is set to the columns it takes. */
BLOT_API size_t blot_text_prefix(const char *s, unsigned cols, unsigned *width);

BLOT_EXTERN_C_END
