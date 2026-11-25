/* #46: BLOT_RENDER_NO_UNICODE (blot --ascii) output is ASCII: every glyph
 * the screen and the canvas emit (axes, ticks, corner, plot marks and the
 * legend symbol), and the labels (a '?' per column for a character past
 * ASCII), checked on the multibyte bytes a program writes. */
#include "blot_test.hpp"

#include <clocale>
#include <cstdlib>
#include <cwctype>
#include <string>

using namespace blot_test;

namespace {

const gint32 XS[] = { 0, 3, 6, 9 };
const gint32 Y_SC[] = { 1, 8, 3, 6 }, Y_LN[] = { 0, 4, 9, 2 }, Y_BAR[] = { 5, 2, 7, 4 };

/* every plot type, each with a label (so a legend line), custom X labels,
 * both axes; the data stays alive for render().  The labels (UTF-8) have
 * characters past ASCII: one column (U+00E9), two (CJK, an emoji) and none
 * (a combining acute) */
void all_types(Fig &f)
{
    static char *labels[] = { (char *)"jan", (char *)"f\xc3\xa9v",
                              (char *)"\xe6\x97\xa5\xe6\x9c\xac", (char *)"e\xcc\x81x" };
    f.size(60, 24);
    f.lim(0, 9, 0, 9);
    ASSERT_TRUE(blot_figure_set_x_axis_labels(f, 4, labels, NULL));
    f.plot(BLOT_SCATTER, BLOT_DATA_INT32, 4, XS, Y_SC, 1, "scatter \xc3\xa9");
    f.plot(BLOT_LINE, BLOT_DATA_INT32, 4, XS, Y_LN, 2, "line \xe6\x97\xa5\xe6\x9c\xac");
    f.plot(BLOT_BAR, BLOT_DATA_INT32, 4, XS, Y_BAR, 3, "bar e\xcc\x81\xf0\x9f\x98\x80");
}

/* the text as the bytes a program writes in the current locale (as the CLI
 * does with printf("%ls")); "" if a character can't be converted */
std::string to_mb(const std::wstring &w, bool &ok)
{
    size_t n = wcstombs(NULL, w.c_str(), 0);
    ok = n != (size_t)-1;
    if (!ok)
        return "";
    std::string s(n, '\0');
    wcstombs(&s[0], w.c_str(), n + 1);
    return s;
}

/* each byte, then each character, is ASCII; the first one that isn't is
 * named */
void expect_ascii(const Rendered &r, const char *what)
{
    bool ok = false;
    std::string mb = to_mb(r.text, ok);
    EXPECT_TRUE(ok) << what << ": not representable in this locale:\n" << to_utf8(r.text);
    for (size_t i = 0; i < mb.size(); i++) {
        if ((unsigned char)mb[i] >= 0x80) {
            ADD_FAILURE() << what << ": byte " << i << " is 0x" << std::hex
                          << (unsigned)(unsigned char)mb[i] << ":\n" << to_utf8(r.text);
            break;
        }
    }
    /* one byte per character: nothing was dropped or expanded */
    if (ok) {
        EXPECT_EQ(mb.size(), r.text.size()) << what;
    }
    for (size_t i = 0; i < r.text.size(); i++) {
        if ((unsigned)r.text[i] >= 0x80) {
            ADD_FAILURE() << what << ": character " << i << " is U+" << std::hex
                          << (unsigned)r.text[i];
            break;
        }
    }
}

const blot_render_flags LEGENDS = BLOT_RENDER_LEGEND_ABOVE | BLOT_RENDER_LEGEND_BELOW
                                | BLOT_RENDER_LEGEND_DETAILS;

/* NO_UNICODE with each of the other flags that change what is drawn */
const blot_render_flags VARIANTS[] = {
    BLOT_RENDER_NONE,
    BLOT_RENDER_NO_COLOR,
    BLOT_RENDER_CLEAR,
    BLOT_RENDER_DONT_INVERT_Y_AXIS,
    BLOT_RENDER_NO_X_AXIS,
    BLOT_RENDER_NO_Y_AXIS,
};

void check_all_variants()
{
    Fig f;
    all_types(f);
    for (blot_render_flags v : VARIANTS) {
        for (blot_render_flags legend : { BLOT_RENDER_NONE, LEGENDS }) {
            blot_render_flags flags = BLOT_RENDER_NO_UNICODE | v | legend;
            Rendered r = render(f, flags);
            ASSERT_TRUE(r.ok) << r.message;
            std::string what = "flags " + std::to_string((unsigned)flags);
            expect_ascii(r, what.c_str());
        }
    }
}

} // namespace

/* in a UTF-8 locale a non-ASCII glyph converts to bytes >= 0x80 */
TEST(Ascii, no_unicode_render_is_ascii_utf8_locale)
{
    RUN_ISOLATED(20, {
        ASSERT_TRUE(setlocale(LC_ALL, "C.UTF-8") != NULL) << "the C.UTF-8 locale is missing";
        check_all_variants();
    });
}

/* in the C locale a non-ASCII glyph can't be written at all */
TEST(Ascii, no_unicode_render_is_ascii_c_locale)
{
    RUN_ISOLATED(20, {
        ASSERT_TRUE(setlocale(LC_ALL, "C") != NULL);
        check_all_variants();
    });
}

/* the legend, axes and marks of that figure, with what each glyph is */
TEST(Ascii, no_unicode_glyphs)
{
    Fig f;
    all_types(f);
    Rendered r = render(f, F | LEGENDS);
    ASSERT_TRUE(r.ok) << r.message;
    ASSERT_EQ(r.lines.size(), 24u);

    /* legend above and below: '#', then the label and the details.  A
     * character past ASCII is a '?' per column: U+00E9 one, CJK and the
     * emoji two, the combining acute none */
    const Lines legend = {
        L"#  scatter ?   \tavg=4.500 min=1.000 max=8.000 count=4",
        L"#  line ????   \tavg=3.750 min=0.000 max=9.000 count=4",
        L"#  bar e??   \tavg=4.500 min=2.000 max=7.000 count=4",
    };
    EXPECT_LINES_EQ(Lines(r.lines.begin(), r.lines.begin() + 3), legend);
    EXPECT_LINES_EQ(Lines(r.lines.end() - 3, r.lines.end()), legend);

    /* the custom X labels, the same way, each under its tick */
    const std::wstring &xl = r.lines[r.lines.size() - 4];
    EXPECT_EQ(xl.substr(r.mrg.left), L"jan          f?v          ????         ex           ")
        << to_utf8(xl);

    /* the plot marks are '*', the axes '|', '-', '+' and '*' for a tick */
    std::wstring glyphs;
    for (size_t i = 3; i < r.lines.size() - 4; i++)
        for (wchar_t wc : r.lines[i])
            if (!iswalnum(wc) && wc != L' ' && wc != L'.' && glyphs.find(wc) == std::wstring::npos)
                glyphs += wc;
    for (wchar_t wc : glyphs)
        EXPECT_NE(std::wstring(L"*|-+").find(wc), std::wstring::npos)
            << "unexpected glyph U+" << std::hex << (unsigned)wc;
}

/* without NO_UNICODE the legend keeps its symbol, and the marks theirs */
TEST(Ascii, unicode_render_keeps_its_glyphs)
{
    Fig f;
    all_types(f);
    Rendered r = render(f, BLOT_RENDER_NO_COLOR | LEGENDS);
    ASSERT_TRUE(r.ok) << r.message;
    ASSERT_FALSE(r.lines.empty());
    EXPECT_EQ(r.lines.front().find(L"\u25d8  scatter \u00e9"), 0u) << to_utf8(r.lines.front());
    EXPECT_EQ(r.lines.back().find(L"\u25d8  bar e\u0301\U0001f600"), 0u) << to_utf8(r.lines.back());
    EXPECT_NE(r.text.find(L"\u65e5\u672c   "), std::wstring::npos) << to_utf8(r.text);
    EXPECT_NE(r.text.find(L'•'), std::wstring::npos) << to_utf8(r.text);
}
