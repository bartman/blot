/* blot: braille character encoding */
/* vim: set noet sw=8 ts=8 tw=120: */
#pragma once

#include <glib.h>

#include "blot_compiler.h"

/*
 * http://www.alanwood.net/unicode/braille_patterns.html
 *
 * The braille character set encodes 8 dots in a font glyph based
 * at offset 0x2800 plus the mask at offset that is set in the glyph.
 *
 *    +------+------+
 *    | 0x01 | 0x08 |
 *    +------+------+
 *    | 0x02 | 0x10 |
 *    +------+------+
 *    | 0x04 | 0x20 |
 *    +------+------+
 *    | 0x40 | 0x80 |
 *    +------+------+
 *
 */

#define BRAILLE_GLYPH_BASE 0x2800

#define BRAILLE_GLYPH_ROWS 4
#define BRAILLE_GLYPH_COLS 2
#define BRAILLE_GLYPH_SIZE (BRAILLE_GLYPH_ROWS * BRAILLE_GLYPH_COLS)

#define BRAILLE_GLYPH_MAP_INDEX(x,y) ( ( (y) * BRAILLE_GLYPH_COLS) + (x) )

BLOT_EXTERN guint8 braille_order_map[BRAILLE_GLYPH_SIZE];
BLOT_EXTERN guint8 braille_upsidedown_order_map[BRAILLE_GLYPH_SIZE];

BLOT_EXTERN guint8 braille_masks[BRAILLE_GLYPH_SIZE];
BLOT_EXTERN guint8 braille_upsidedown_masks[BRAILLE_GLYPH_SIZE];
