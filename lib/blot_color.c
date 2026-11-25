/* blot: color handling */
/* vim: set noet sw=8 ts=8 tw=120: */
#include "blot_color.h"

bool have_color_support = true;

/* every 256-colour escape sequence, built at compile time */
#define FG(n) COL_FG_PREFIX #n COL_SUFFIX
#define BG(n) COL_BG_PREFIX #n COL_SUFFIX

static const char *const blot_fg_table[256] = {
	FG(0), FG(1), FG(2), FG(3), FG(4), FG(5), FG(6), FG(7),
	FG(8), FG(9), FG(10), FG(11), FG(12), FG(13), FG(14), FG(15),
	FG(16), FG(17), FG(18), FG(19), FG(20), FG(21), FG(22), FG(23),
	FG(24), FG(25), FG(26), FG(27), FG(28), FG(29), FG(30), FG(31),
	FG(32), FG(33), FG(34), FG(35), FG(36), FG(37), FG(38), FG(39),
	FG(40), FG(41), FG(42), FG(43), FG(44), FG(45), FG(46), FG(47),
	FG(48), FG(49), FG(50), FG(51), FG(52), FG(53), FG(54), FG(55),
	FG(56), FG(57), FG(58), FG(59), FG(60), FG(61), FG(62), FG(63),
	FG(64), FG(65), FG(66), FG(67), FG(68), FG(69), FG(70), FG(71),
	FG(72), FG(73), FG(74), FG(75), FG(76), FG(77), FG(78), FG(79),
	FG(80), FG(81), FG(82), FG(83), FG(84), FG(85), FG(86), FG(87),
	FG(88), FG(89), FG(90), FG(91), FG(92), FG(93), FG(94), FG(95),
	FG(96), FG(97), FG(98), FG(99), FG(100), FG(101), FG(102), FG(103),
	FG(104), FG(105), FG(106), FG(107), FG(108), FG(109), FG(110), FG(111),
	FG(112), FG(113), FG(114), FG(115), FG(116), FG(117), FG(118), FG(119),
	FG(120), FG(121), FG(122), FG(123), FG(124), FG(125), FG(126), FG(127),
	FG(128), FG(129), FG(130), FG(131), FG(132), FG(133), FG(134), FG(135),
	FG(136), FG(137), FG(138), FG(139), FG(140), FG(141), FG(142), FG(143),
	FG(144), FG(145), FG(146), FG(147), FG(148), FG(149), FG(150), FG(151),
	FG(152), FG(153), FG(154), FG(155), FG(156), FG(157), FG(158), FG(159),
	FG(160), FG(161), FG(162), FG(163), FG(164), FG(165), FG(166), FG(167),
	FG(168), FG(169), FG(170), FG(171), FG(172), FG(173), FG(174), FG(175),
	FG(176), FG(177), FG(178), FG(179), FG(180), FG(181), FG(182), FG(183),
	FG(184), FG(185), FG(186), FG(187), FG(188), FG(189), FG(190), FG(191),
	FG(192), FG(193), FG(194), FG(195), FG(196), FG(197), FG(198), FG(199),
	FG(200), FG(201), FG(202), FG(203), FG(204), FG(205), FG(206), FG(207),
	FG(208), FG(209), FG(210), FG(211), FG(212), FG(213), FG(214), FG(215),
	FG(216), FG(217), FG(218), FG(219), FG(220), FG(221), FG(222), FG(223),
	FG(224), FG(225), FG(226), FG(227), FG(228), FG(229), FG(230), FG(231),
	FG(232), FG(233), FG(234), FG(235), FG(236), FG(237), FG(238), FG(239),
	FG(240), FG(241), FG(242), FG(243), FG(244), FG(245), FG(246), FG(247),
	FG(248), FG(249), FG(250), FG(251), FG(252), FG(253), FG(254), FG(255),
};

static const char *const blot_bg_table[256] = {
	BG(0), BG(1), BG(2), BG(3), BG(4), BG(5), BG(6), BG(7),
	BG(8), BG(9), BG(10), BG(11), BG(12), BG(13), BG(14), BG(15),
	BG(16), BG(17), BG(18), BG(19), BG(20), BG(21), BG(22), BG(23),
	BG(24), BG(25), BG(26), BG(27), BG(28), BG(29), BG(30), BG(31),
	BG(32), BG(33), BG(34), BG(35), BG(36), BG(37), BG(38), BG(39),
	BG(40), BG(41), BG(42), BG(43), BG(44), BG(45), BG(46), BG(47),
	BG(48), BG(49), BG(50), BG(51), BG(52), BG(53), BG(54), BG(55),
	BG(56), BG(57), BG(58), BG(59), BG(60), BG(61), BG(62), BG(63),
	BG(64), BG(65), BG(66), BG(67), BG(68), BG(69), BG(70), BG(71),
	BG(72), BG(73), BG(74), BG(75), BG(76), BG(77), BG(78), BG(79),
	BG(80), BG(81), BG(82), BG(83), BG(84), BG(85), BG(86), BG(87),
	BG(88), BG(89), BG(90), BG(91), BG(92), BG(93), BG(94), BG(95),
	BG(96), BG(97), BG(98), BG(99), BG(100), BG(101), BG(102), BG(103),
	BG(104), BG(105), BG(106), BG(107), BG(108), BG(109), BG(110), BG(111),
	BG(112), BG(113), BG(114), BG(115), BG(116), BG(117), BG(118), BG(119),
	BG(120), BG(121), BG(122), BG(123), BG(124), BG(125), BG(126), BG(127),
	BG(128), BG(129), BG(130), BG(131), BG(132), BG(133), BG(134), BG(135),
	BG(136), BG(137), BG(138), BG(139), BG(140), BG(141), BG(142), BG(143),
	BG(144), BG(145), BG(146), BG(147), BG(148), BG(149), BG(150), BG(151),
	BG(152), BG(153), BG(154), BG(155), BG(156), BG(157), BG(158), BG(159),
	BG(160), BG(161), BG(162), BG(163), BG(164), BG(165), BG(166), BG(167),
	BG(168), BG(169), BG(170), BG(171), BG(172), BG(173), BG(174), BG(175),
	BG(176), BG(177), BG(178), BG(179), BG(180), BG(181), BG(182), BG(183),
	BG(184), BG(185), BG(186), BG(187), BG(188), BG(189), BG(190), BG(191),
	BG(192), BG(193), BG(194), BG(195), BG(196), BG(197), BG(198), BG(199),
	BG(200), BG(201), BG(202), BG(203), BG(204), BG(205), BG(206), BG(207),
	BG(208), BG(209), BG(210), BG(211), BG(212), BG(213), BG(214), BG(215),
	BG(216), BG(217), BG(218), BG(219), BG(220), BG(221), BG(222), BG(223),
	BG(224), BG(225), BG(226), BG(227), BG(228), BG(229), BG(230), BG(231),
	BG(232), BG(233), BG(234), BG(235), BG(236), BG(237), BG(238), BG(239),
	BG(240), BG(241), BG(242), BG(243), BG(244), BG(245), BG(246), BG(247),
	BG(248), BG(249), BG(250), BG(251), BG(252), BG(253), BG(254), BG(255),
};

const char *blot_color_fg(int col)
{
	return blot_fg_table[col & 0xFF];
}

const char *blot_color_bg(int col)
{
	return blot_bg_table[col & 0xFF];
}
