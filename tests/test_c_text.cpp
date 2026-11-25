/* the display width of label text, the same in every locale (#18) */
#include "blot_test.hpp"
#include "blot_text.h"

#include <clocale>
#include <cwchar>
#include <gnu/libc-version.h>

using namespace blot_test;

namespace {

#define SET_LOCALE(loc) ASSERT_TRUE(setlocale(LC_ALL, (loc)) != NULL) << "the " << (loc) << " locale is missing"

/* the common blocks the built-in table must get right */
const struct { const char *name; wchar_t first, last; } BLOCKS[] = {
    { "Latin-1, Latin Extended, IPA", 0x00a0, 0x02ff },
    { "combining diacritics", 0x0300, 0x036f },
    { "Greek, Cyrillic", 0x0370, 0x052f },
    { "Hangul Jamo", 0x1100, 0x11ff },
    { "general punctuation", 0x2000, 0x206f },
    { "arrows, symbols, box drawing", 0x2190, 0x27bf },
    { "CJK radicals", 0x2e80, 0x2fdf },
    { "CJK symbols, kana", 0x3000, 0x30ff },
    { "CJK Extension A", 0x3400, 0x4dbf },
    { "CJK Unified Ideographs", 0x4e00, 0x9fff },
    { "Hangul syllables", 0xac00, 0xd7a3 },
    { "CJK compatibility ideographs", 0xf900, 0xfaff },
    { "halfwidth and fullwidth forms", 0xff00, 0xffef },
    { "emoji: pictographs, emoticons", 0x1f300, 0x1f64f },
    { "emoji: transport", 0x1f680, 0x1f6ff },
    { "emoji: supplemental", 0x1f900, 0x1f9ff },
    { "CJK Extension B", 0x20000, 0x2a6df },
};

/* Unicode 16 (glibc 2.40) changed these: the Yijing trigrams and
 * monograms became Wide, the halfwidth Hangul filler takes no column.  The
 * table is from glibc 2.41; deb12's glibc 2.36 has the old widths */
bool changed_in_unicode_16(wchar_t wc)
{
    return (wc >= 0x2630 && wc <= 0x2637) || (wc >= 0x268a && wc <= 0x268f) || wc == 0xffa0;
}

} // namespace

/* in C.UTF-8, the built-in table and wcwidth() agree on every character
 * of the common blocks glibc knows, but for the few Unicode 16 changed
 * when glibc is older */
TEST(TextWidth, builtin_agrees_with_utf8_locale)
{
    RUN_ISOLATED(30, {
        SET_LOCALE("C.UTF-8");
        for (const auto &b : BLOCKS) {
            unsigned known = 0, bad = 0;
            for (wchar_t wc = b.first; wc <= b.last; wc++) {
                int w = wcwidth(wc);
                if (w < 0)
                    continue;
                known++;
                EXPECT_EQ(blot_text_wcwidth(wc), w);
                if (changed_in_unicode_16(wc) && strverscmp(gnu_get_libc_version(), "2.40") < 0)
                    continue;
                if (blot_text_builtin_wcwidth(wc) != w && bad++ < 5)
                    ADD_FAILURE() << b.name << ": U+" << std::hex << (unsigned)wc << " wcwidth " << std::dec
                                  << w << ", built-in " << blot_text_builtin_wcwidth(wc);
            }
            EXPECT_EQ(bad, 0u) << b.name;
            EXPECT_GT(known, 0u) << b.name;
        }
    });
}

/* in the C locale, wcwidth() is -1 past ASCII: the built-in table is used */
TEST(TextWidth, c_locale_uses_builtin)
{
    RUN_ISOLATED(30, {
        SET_LOCALE("C");
        ASSERT_EQ(wcwidth(0x65e5), -1) << "the C locale measures CJK: nothing to test";
        for (const auto &b : BLOCKS)
            for (wchar_t wc = b.first; wc <= b.last; wc++)
                ASSERT_EQ(blot_text_wcwidth(wc), blot_text_builtin_wcwidth(wc))
                    << b.name << ": U+" << std::hex << (unsigned)wc;
    });
}

/* the table is glibc 2.41's: with that glibc, the built-in table and
 * C.UTF-8 wcwidth() are the same on every code point, the unprintable
 * ones (-1) too */
TEST(TextWidth, builtin_equals_glibc_2_41_everywhere)
{
    if (strcmp(gnu_get_libc_version(), "2.41"))
        GTEST_SKIP() << "the table is from glibc 2.41, this is " << gnu_get_libc_version();
    RUN_ISOLATED(60, {
        SET_LOCALE("C.UTF-8");
        unsigned bad = 0;
        for (wchar_t wc = 0xa0; wc <= 0x10ffff; wc++) {
            int w = (wc >= 0xd800 && wc <= 0xdfff) ? -1 : wcwidth(wc);
            if (blot_text_builtin_wcwidth(wc) != w && bad++ < 5)
                ADD_FAILURE() << "U+" << std::hex << (unsigned)wc << " wcwidth " << std::dec << w
                              << ", built-in " << blot_text_builtin_wcwidth(wc);
        }
        EXPECT_EQ(bad, 0u);
    });
}

TEST(TextWidth, examples_any_locale)
{
    for (const char *loc : { "C", "C.UTF-8" }) {
        SCOPED_TRACE(loc);
        RUN_ISOLATED(10, {
            SET_LOCALE(loc);
            const struct { wchar_t wc; int w; } EX[] = {
                { L'a', 1 }, { 0x00e9, 1 }, { 0x25d8, 1 }, { 0x2800, 1 },
                { 0x0301, 0 }, { 0x200b, 0 },
                { 0x65e5, 2 }, { 0x30c6, 2 }, { 0xac00, 2 }, { 0xd55c, 2 }, { 0xff21, 2 }, { 0x3000, 2 },
                { 0x1f600, 2 }, { 0x1f680, 2 }, { 0x20000, 2 },
                /* unassigned, noncharacters (#18 review Low-1) */
                { 0x0378, -1 }, { 0x0a44, -1 }, { 0x09fe, 0 } /* Bengali sandhi mark, next to unassigned U+09FF */, { 0xfffe, -1 }, { 0x40000, -1 },
                { 0x00, -1 }, { L'\t', -1 }, { L'\n', -1 }, { 0x1b, -1 }, { 0x7f, -1 }, { 0x9b, -1 },
            };
            for (const auto &e : EX)
                EXPECT_EQ(blot_text_wcwidth(e.wc), e.w) << "U+" << std::hex << (unsigned)e.wc;
        });
    }
}

/* labels are read as UTF-8 whatever the locale; what isn't text is '?' */
TEST(TextWidth, next_any_locale)
{
    for (const char *loc : { "C", "C.UTF-8" }) {
        SCOPED_TRACE(loc);
        RUN_ISOLATED(10, {
            SET_LOCALE(loc);
            auto read = [](const char *s, bool keep_tab) {
                std::wstring out;
                size_t left = strlen(s), n;
                wchar_t wc;
                while ((n = blot_text_next(s, left, keep_tab, &wc))) {
                    out += wc;
                    s += n;
                    left -= n;
                }
                return out;
            };
            EXPECT_EQ(read("caf\xc3\xa9 \xe6\x97\xa5 \xf0\x9f\x98\x80", false), L"café 日 \U0001f600");
            /* Latin-1, a cut sequence, an overlong '/', a surrogate: one
             * '?' per byte */
            EXPECT_EQ(read("\xe9|\xe6\x97|\xc0\xaf|\xed\xa0\x80", false), L"?|??|??|???");
            /* controls, C1 CSI */
            EXPECT_EQ(read("a\x1b[1m\n\r\x7f\xc2\x9b", false), L"a?[1m????");
            EXPECT_EQ(read("a\tb", false), L"a?b");
            EXPECT_EQ(read("a\tb", true), L"a\tb");
            wchar_t wc = L'x';
            EXPECT_EQ(blot_text_next("", 0, false, &wc), 0u);
            EXPECT_EQ(blot_text_next("ab", 0, false, &wc), 0u);
        });
    }
}

TEST(TextWidth, prefix_any_locale)
{
    for (const char *loc : { "C", "C.UTF-8" }) {
        SCOPED_TRACE(loc);
        RUN_ISOLATED(10, {
            SET_LOCALE(loc);
            unsigned w = 99;
            /* a日本 : 1 + 2 + 2 */
            const char *s = "a\xe6\x97\xa5\xe6\x9c\xac";
            EXPECT_EQ(blot_text_prefix(s, 0, &w), 0u);
            EXPECT_EQ(w, 0u);
            EXPECT_EQ(blot_text_prefix(s, 2, &w), 1u);
            EXPECT_EQ(w, 1u);
            EXPECT_EQ(blot_text_prefix(s, 3, &w), 4u);
            EXPECT_EQ(w, 3u);
            EXPECT_EQ(blot_text_prefix(s, 4, &w), 4u);
            EXPECT_EQ(w, 3u);
            EXPECT_EQ(blot_text_prefix(s, 5, &w), 7u);
            EXPECT_EQ(w, 5u);
            EXPECT_EQ(blot_text_prefix(s, 50, NULL), 7u);
            /* a combining mark stays with its letter */
            EXPECT_EQ(blot_text_prefix("e\xcc\x81" "e\xcc\x81", 1, &w), 3u);
            EXPECT_EQ(w, 1u);
            /* marks past BLOT_TEXT_MAX_MARKS are dropped: a label of
             * them fits whole, not cut */
            std::string many = "e";
            for (int i = 0; i < 10; i++)
                many += "\xcc\x81";
            EXPECT_EQ(blot_text_prefix(many.c_str(), 1, &w), many.size());
            EXPECT_EQ(w, 1u);
            many += "x";
            EXPECT_EQ(blot_text_prefix(many.c_str(), 1, &w), 1u + 2 * BLOT_TEXT_MAX_MARKS);
            EXPECT_EQ(w, 1u);
            /* an invalid byte is a '?', one column */
            EXPECT_EQ(blot_text_prefix("\xff\xff\xff", 2, &w), 2u);
            EXPECT_EQ(w, 2u);
        });
    }
}

/* the reader keeps at most BLOT_TEXT_MAX_MARKS zero-width marks after a
 * character, and at the start (#18 review Low-2) */
TEST(TextWidth, reader_caps_marks_any_locale)
{
    for (const char *loc : { "C", "C.UTF-8" }) {
        SCOPED_TRACE(loc);
        RUN_ISOLATED(10, {
            SET_LOCALE(loc);
            auto shown = [](const std::string &s, bool keep_tab) {
                struct blot_text_reader r;
                std::wstring out;
                std::vector<unsigned> widths;
                wchar_t wc;
                unsigned w;
                blot_text_reader_init(&r, s.c_str(), keep_tab);
                while (blot_text_read(&r, &wc, &w)) {
                    out += wc;
                    widths.push_back(w);
                }
                return std::make_pair(out, widths);
            };
            std::string marks6;
            for (int i = 0; i < 6; i++)
                marks6 += "\xcc\x81";
            auto r = shown(marks6 + "a" + marks6 + "\xe6\x97\xa5" + marks6 + "\t" + marks6, true);
            const std::wstring m4 = L"\u0301\u0301\u0301\u0301";
            EXPECT_EQ(r.first, m4 + L"a" + m4 + L"\u65e5" + m4 + L"\t" + m4);
            EXPECT_EQ(r.second, (std::vector<unsigned>{ 0, 0, 0, 0, 1, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0 }));
            /* a tab not kept is a '?', one column */
            r = shown("a\tb", false);
            EXPECT_EQ(r.first, L"a?b");
            EXPECT_EQ(r.second, (std::vector<unsigned>{ 1, 1, 1 }));
        });
    }
}
