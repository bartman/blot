/* blot: braille character encoding */
/* vim: set noet sw=8 ts=8 tw=120: */
#include "blot_braille.h"

guint8 braille_order_map[BRAILLE_GLYPH_SIZE] = {
	0, 3,
	1, 4,
	2, 5,
	6, 7,
};

guint8 braille_upsidedown_order_map[BRAILLE_GLYPH_SIZE] = {
	6, 7,
	2, 5,
	1, 4,
	0, 3,
};

guint8 braille_masks[BRAILLE_GLYPH_SIZE] = {
	0x01, 0x08,
	0x02, 0x10,
	0x04, 0x20,
	0x40, 0x80,
};

guint8 braille_upsidedown_masks[BRAILLE_GLYPH_SIZE] = {
	0x40, 0x80,
	0x04, 0x20,
	0x02, 0x10,
	0x01, 0x08,
};
