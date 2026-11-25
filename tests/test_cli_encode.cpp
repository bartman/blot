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

TEST(CliEncode, unknown_width_is_one_column)
{
	/* the C locale can't measure U+4E2D (wcwidth -1): one column, as
	 * the library's legend assumes */
	CType c("C");
	ASSERT_TRUE(c.ok);
	bool replaced = false;
	EXPECT_EQ(encode(L"a\u4E2Db", &replaced), "a?b");
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
