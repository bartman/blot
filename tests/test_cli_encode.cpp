/* the CLI's plot encoder (cli/plot_encode.hpp, #40): all of the text in
 * the locale's encoding, and a character the locale can't encode as as
 * many '?' as its display width, so the lines keep the width the
 * library laid out */
#include <gtest/gtest.h>

#include <clocale>
#include <sstream>
#include <string>
#include <vector>

#include "plot_encode.hpp"

#include "blot.h"

namespace {

/* sets LC_CTYPE for one test, and puts the old one back */
struct CType {
	std::string old;
	bool ok;
	explicit CType(const char *name) : old(setlocale(LC_CTYPE, nullptr)) {
		ok = setlocale(LC_CTYPE, name) != nullptr;
	}
	~CType() { setlocale(LC_CTYPE, old.c_str()); }
};

/* the widths a locale-independent table gives (#18): CJK is wide, a
 * combining acute accent takes no column, the rest one */
int table_width(wchar_t wc)
{
	if (wc == 0x0301)
		return 0;
	if (wc >= 0x4E00 && wc <= 0x9FFF)
		return 2;
	return 1;
}

std::string encode(const std::wstring &w, bool *replaced,
		   int (*width)(wchar_t) = plot_char_width)
{
	return plot_encode(w.c_str(), w.size(), replaced, width);
}

/* three lines the library laid out 10 columns wide (by table_width):
 * a wide label, a combining one and the legend symbol */
const std::wstring screen =
	L"\u4E2D\u6587abcdef\n"		/* 2 + 2 + 6 */
	L"e\u0301\u0301abcdefghi\n"	/* 1 + 0 + 0 + 9 */
	L"\u25D8 label123\n";		/* 1 + 1 + 8 */

std::vector<std::string> lines(const std::string &s)
{
	std::vector<std::string> v;
	std::istringstream in(s);
	for (std::string l; std::getline(in, l);)
		v.push_back(l);
	return v;
}

} // namespace

TEST(CliEncode, ascii_is_unchanged)
{
	CType c("C");
	ASSERT_TRUE(c.ok);
	bool replaced = true;
	EXPECT_EQ(encode(L"\033[2J  1.0 *---\n", &replaced), "\033[2J  1.0 *---\n");
	EXPECT_FALSE(replaced);
}

TEST(CliEncode, nothing_after_an_unencodable_character_is_lost)
{
	CType c("C");
	ASSERT_TRUE(c.ok);
	bool replaced = false;
	/* printf("%ls") stopped at the U+25D8 */
	EXPECT_EQ(encode(L"ab\u25D8 cd\nef", &replaced), "ab? cd\nef");
	EXPECT_TRUE(replaced);
}

TEST(CliEncode, question_marks_as_wide_as_the_character)
{
	CType c("C");
	ASSERT_TRUE(c.ok);
	bool replaced = false;
	EXPECT_EQ(encode(L"a\u4E2Db", &replaced, table_width), "a??b");
	EXPECT_TRUE(replaced);
	EXPECT_EQ(encode(L"e\u0301x", &replaced, table_width), "ex");
	EXPECT_EQ(encode(L"\u25D8x", &replaced, table_width), "?x");
}

TEST(CliEncode, width_is_the_librarys_in_every_locale)
{
	/* the C locale can't measure U+4E2D (wcwidth -1), but the library
	 * laid it out two columns wide (#18): two '?'s */
	CType c("C");
	ASSERT_TRUE(c.ok);
	bool replaced = false;
	EXPECT_EQ(encode(L"a\u4E2Db", &replaced), "a??b");
	EXPECT_EQ(encode(L"e\u0301x", &replaced), "ex");
}

TEST(CliEncode, unknown_width_is_one_column)
{
	/* U+0378 is unassigned: blot_text_wcwidth() is -1, so one column,
	 * as the library's legend assumes */
	CType c("C");
	ASSERT_TRUE(c.ok);
	bool replaced = false;
	EXPECT_EQ(encode(L"a\u0378b", &replaced), "a?b");
}

TEST(CliEncode, line_widths_are_kept_in_the_c_locale)
{
	CType c("C");
	ASSERT_TRUE(c.ok);
	bool replaced = false;
	auto out = encode(screen, &replaced, table_width);
	EXPECT_TRUE(replaced);
	auto v = lines(out);
	ASSERT_EQ(v.size(), 3u) << out;
	for (const auto &l : v)
		EXPECT_EQ(l.size(), 10u) << "'" << l << "'";
	EXPECT_EQ(v[0], "????abcdef");
	EXPECT_EQ(v[1], "eabcdefghi");
	EXPECT_EQ(v[2], "? label123");
}

TEST(CliEncode, line_widths_are_kept_in_latin1)
{
	CType c("en_US.ISO-8859-1");
	if (!c.ok)
		GTEST_SKIP() << "no en_US.ISO-8859-1 locale";
	bool replaced = false;
	/* e-acute is in Latin-1: one byte, one column */
	auto out = encode(L"\u00E9\u4E2D\u25D8x", &replaced, table_width);
	EXPECT_EQ(out, "\xE9???x");
	auto scr = lines(encode(screen, &replaced, table_width));
	ASSERT_EQ(scr.size(), 3u);
	for (const auto &l : scr)
		EXPECT_EQ(l.size(), 10u) << "'" << l << "'";
}

TEST(CliEncode, utf8_keeps_every_character)
{
	CType c("C.UTF-8");
	if (!c.ok)
		GTEST_SKIP() << "no C.UTF-8 locale";
	bool replaced = true;
	EXPECT_EQ(encode(screen, &replaced),
		  "\u4E2D\u6587abcdef\ne\u0301\u0301abcdefghi\n\u25D8 label123\n");
	EXPECT_FALSE(replaced);
}

namespace {

/* a real render (Unicode, no colour) with a CJK and a combining label, in
 * the legend and as custom X labels, encoded with the default width
 * (blot_text_wcwidth()) in the locale of the test */
struct RealPlot {
	std::wstring text;
	unsigned cols = 50;
	RealPlot() {
		static const gint32 xs[] = { 0, 3, 6, 9 }, ys[] = { 1, 5, 2, 8 };
		static char *labels[] = { (char *)"\xe6\x97\xa5\xe6\x9c\xac", (char *)"e\xcc\x81",
					  (char *)"\xe4\xb8\xad", (char *)"ab" };
		blot_figure *fig = blot_figure_new(NULL);
		EXPECT_TRUE(blot_figure_set_screen_size(fig, cols, 20, NULL));
		EXPECT_TRUE(blot_figure_set_x_axis_labels(fig, 4, labels, NULL));
		EXPECT_TRUE(blot_figure_plot(fig, BLOT_LINE, BLOT_DATA_INT32, 4, xs, ys, 1,
					     "\xe6\x97\xa5\xe6\x9c\xac\xe8\xaa\x9e", NULL));
		EXPECT_TRUE(blot_figure_plot(fig, BLOT_SCATTER, BLOT_DATA_INT32, 4, xs, ys, 2,
					     "e\xcc\x81" "e\xcc\x81", NULL));
		blot_screen *scr = blot_figure_render(fig, (blot_render_flags)(BLOT_RENDER_NO_COLOR
					| BLOT_RENDER_LEGEND_BELOW), NULL);
		EXPECT_TRUE(scr != NULL);
		if (scr) {
			gsize n = 0;
			const wchar_t *t = blot_screen_get_text(scr, &n, NULL);
			text.assign(t, n);
			blot_screen_delete(scr);
		}
		blot_figure_delete(fig);
	}
};

/* each rendered line's width as the library counts it */
std::vector<unsigned> layout_widths(const std::wstring &text)
{
	std::vector<unsigned> v;
	unsigned w = 0;
	for (wchar_t wc : text) {
		if (wc == L'\n') {
			v.push_back(w);
			w = 0;
			continue;
		}
		int cw = blot_text_wcwidth(wc);
		w += cw >= 0 ? (unsigned)cw : 1;
	}
	return v;
}

void expect_real_plot_widths(const char *locale)
{
	CType c(locale);
	if (!c.ok)
		GTEST_SKIP() << "no " << locale << " locale";
	RealPlot p;
	ASSERT_FALSE(p.text.empty());
	bool replaced = false;
	auto v = lines(encode(p.text, &replaced));
	EXPECT_TRUE(replaced);
	auto want = layout_widths(p.text);
	ASSERT_EQ(v.size(), want.size());
	/* every byte is one column here: ASCII, '?', or a Latin-1 byte */
	for (size_t i = 0; i < v.size(); i++)
		EXPECT_EQ(v[i].size(), want[i]) << i << ": '" << v[i] << "'";
	/* the X label row is the whole width; the labels under their ticks */
	ASSERT_GE(v.size(), 4u);
	EXPECT_EQ(v[v.size() - 3].size(), p.cols) << v[v.size() - 3];
	EXPECT_NE(v[v.size() - 3].find("??"), std::string::npos) << v[v.size() - 3];
	/* the legend: U+25D8, then the labels, '?' per column */
	EXPECT_EQ(v[v.size() - 2], "?  ??????");
	EXPECT_EQ(v[v.size() - 1], "?  ee");
}

} // namespace

/* the CJK and combining labels of a real plot keep their exact widths
 * (#18 re-review, note 36606) */
TEST(CliEncode, real_plot_widths_c_locale)
{
	expect_real_plot_widths("C");
}

TEST(CliEncode, real_plot_widths_latin1)
{
	expect_real_plot_widths("en_US.ISO-8859-1");
}
