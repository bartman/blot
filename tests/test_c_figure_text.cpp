/* colour, legend, clear (#2, group S).  Colour tests compare complete
 * lines without trimming; in the expected text "\e" stands for ESC. */
#include "blot_test.hpp"

#include <cwchar>

using namespace blot_test;

namespace {

/* "\e" (backslash, e) in a golden becomes ESC */
std::wstring sgr(const std::wstring &s)
{
    std::wstring out;
    for (size_t i = 0; i < s.size(); i++) {
        if (s[i] == L'\\' && i + 1 < s.size() && s[i + 1] == L'e') {
            out += L'\x1b';
            i++;
        } else {
            out += s[i];
        }
    }
    return out;
}

Lines sgr(const Lines &lines)
{
    Lines out;
    for (const auto &l : lines)
        out.push_back(sgr(l));
    return out;
}

/* G1 as rendered: not trimmed */
Lines g1_untrimmed()
{
    Fig f;
    fix_a_scatter(f);
    Rendered r = render(f, F);
    EXPECT_TRUE(r.ok) << r.message;
    return r.lines;
}

const double S9_X[] = { 0, 1, 2, 3 }, S9_Y[] = { -2, 4, 1, 0.5 };

void s9_layers(Fig &f)
{
    f.size(40, 16);
    f.plot(BLOT_SCATTER, BLOT_DATA_DOUBLE, 4, S9_X, S9_Y, 3, "sc");
    f.plot(BLOT_BAR, BLOT_DATA_DOUBLE, 4, S9_X, S9_Y, 200, "a much longer label");
}

/* S11, S12: two labelled layers */
const gint32 TWO_X[] = { 0, 5, 10 }, TWO_Y1[] = { 0, 5, 10 }, TWO_Y2[] = { 10, 5, 0 };

void two_layers(Fig &f)
{
    f.size(40, 16);
    f.plot(BLOT_LINE, BLOT_DATA_INT32, 3, TWO_X, TWO_Y1, 1, "one");
    f.plot(BLOT_LINE, BLOT_DATA_INT32, 3, TWO_X, TWO_Y2, 2, "two");
}

const wchar_t LEGEND_MARK = 0x25D8;     /* ◘ */

} // namespace

TEST(Text, colour_sequences_single_layer)
{
    Fig f;
    fix_a_scatter(f);
    Rendered r = render(f, BLOT_RENDER_NO_UNICODE);
    ASSERT_TRUE(r.ok) << r.message;
    EXPECT_LINES_EQ(r.lines, sgr({
        L"\\e[38;5;7m       |                   \\e[38;5;1m*\\e[0m",
        L"\\e[38;5;7m       |                    \\e[0m",
        L"\\e[38;5;7m       |                    \\e[0m",
        L"\\e[38;5;7m 6.000 *                    \\e[0m",
        L"\\e[38;5;7m       |          \\e[38;5;1m*         \\e[0m",
        L"\\e[38;5;7m       |                    \\e[0m",
        L"\\e[38;5;7m 3.000 *                    \\e[0m",
        L"\\e[38;5;7m       |     \\e[38;5;1m*              \\e[0m",
        L"\\e[38;5;7m       |                    \\e[0m",
        L"\\e[38;5;7m 0.000 *\\e[38;5;1m*                   \\e[0m",
        L"\\e[38;5;7m       +*-----*------*------\\e[0m",
        L"\\e[38;5;7m        0.000 6.333  12.67  \\e[0m",
    }));
}

TEST(Text, axis_colour)
{
    const gint32 xs[] = { 3, 4, 5 }, ys[] = { 4, 4, 4 };
    Fig f;
    fix_a(f);
    ASSERT_TRUE(blot_figure_set_axis_color(f, 12, NULL));
    f.plot(BLOT_SCATTER, BLOT_DATA_INT32, 3, xs, ys, 2);
    Rendered r = render(f, BLOT_RENDER_NO_UNICODE);
    ASSERT_TRUE(r.ok) << r.message;
    ASSERT_EQ(r.lines.size(), 12u);
    const std::wstring start = sgr(L"\\e[38;5;12m"), end = sgr(L"\\e[0m");
    for (const auto &l : r.lines) {
        EXPECT_EQ(l.compare(0, start.size(), start), 0) << to_utf8(l);
        EXPECT_TRUE(l.size() >= end.size() && l.compare(l.size() - end.size(), end.size(), end) == 0)
            << to_utf8(l);
    }
    /* row 4 is line 5 */
    EXPECT_EQ(r.lines[5], sgr(L"\\e[38;5;12m       |   \\e[38;5;2m***              \\e[0m"));
}

TEST(Text, later_layer_wins_overlap)
{
    const gint32 ax[] = { 3, 4, 5 }, ay[] = { 4, 4, 4 };
    const gint32 bx[] = { 4, 4 }, by[] = { 4, 6 };
    Fig f(10, 10);
    f.lim(0, 9, 0, 9);
    f.plot(BLOT_SCATTER, BLOT_DATA_INT32, 3, ax, ay, 2);
    f.plot(BLOT_SCATTER, BLOT_DATA_INT32, 2, bx, by, 9);
    Rendered r = render(f, BLOT_RENDER_NO_UNICODE | BLOT_RENDER_NO_X_AXIS | BLOT_RENDER_NO_Y_AXIS);
    ASSERT_TRUE(r.ok) << r.message;
    Lines expected(10, std::wstring(10, L' '));
    expected[3] = sgr(L"    \\e[38;5;9m*     \\e[0m");
    expected[5] = sgr(L"   \\e[38;5;2m*\\e[38;5;9m*\\e[38;5;2m*    \\e[0m");
    EXPECT_LINES_EQ(r.lines, expected);
}

TEST(Text, no_color_has_no_escapes)
{
    for (blot_plot_type type : { BLOT_SCATTER, BLOT_LINE, BLOT_BAR }) {
        SCOPED_TRACE(type);
        Fig f;
        fix_a(f);
        f.plot(type, BLOT_DATA_INT32, 4, SC_X, SC_Y, 1, "pts");
        Rendered r = render(f, F);
        ASSERT_TRUE(r.ok) << r.message;
        EXPECT_FALSE(has_esc(r.text)) << to_utf8(r.text);
    }
}

TEST(Text, stripping_colour_gives_no_color_output)
{
    Fig f;
    fix_a_scatter(f);
    Rendered r = render(f, BLOT_RENDER_NO_UNICODE);
    ASSERT_TRUE(r.ok) << r.message;
    EXPECT_LINES_EQ(strip_sgr(r.lines), g1_untrimmed());
}

TEST(Text, clear_prefix)
{
    Fig f;
    fix_a_scatter(f);
    Rendered r = render(f, F | BLOT_RENDER_CLEAR);
    ASSERT_TRUE(r.ok) << r.message;
    const std::wstring clear = sgr(L"\\e[2J\\e[H");
    ASSERT_EQ(r.text.compare(0, clear.size(), clear), 0) << to_utf8(r.text.substr(0, 16));
    EXPECT_LINES_EQ(split_lines(r.text.substr(clear.size())), g1_untrimmed());
}

TEST(Text, text_size_matches_content)
{
    const blot_render_flags flag_sets[] = {
        BLOT_RENDER_NO_UNICODE, F, F | BLOT_RENDER_CLEAR, BLOT_RENDER_NO_COLOR,
        BLOT_RENDER_NO_UNICODE | BLOT_RENDER_NO_X_AXIS | BLOT_RENDER_NO_Y_AXIS,
    };
    for (auto flags : flag_sets) {
        SCOPED_TRACE(flags);
        Fig f;
        fix_a_scatter(f);
        Rendered r = render(f, flags);
        ASSERT_TRUE(r.ok) << r.message;
        EXPECT_EQ(r.text.size(), wcslen(r.text.c_str()));
        ASSERT_FALSE(r.text.empty());
        EXPECT_EQ(r.text.back(), L'\n');
    }
}

TEST(Text, legend_above_line)
{
    Fig f;
    fix_a_scatter(f);
    f.size(28, 13);
    Rendered r = render(f, BLOT_RENDER_NO_UNICODE | BLOT_RENDER_LEGEND_ABOVE);
    ASSERT_TRUE(r.ok) << r.message;
    ASSERT_FALSE(r.lines.empty());
    EXPECT_EQ(r.lines[0], sgr(L"\\e[38;5;1m◘ \\e[0m pts"));
}

TEST(Text, legend_below_details)
{
    Fig f;
    s9_layers(f);
    Rendered r = render(f, BLOT_RENDER_NO_UNICODE | BLOT_RENDER_LEGEND_BELOW | BLOT_RENDER_LEGEND_DETAILS);
    ASSERT_TRUE(r.ok) << r.message;
    ASSERT_GE(r.lines.size(), 2u);
    Lines last(r.lines.end() - 2, r.lines.end());
    EXPECT_LINES_EQ(last, sgr(Lines{
        L"\\e[38;5;3m◘ \\e[0m sc   \tavg=0.875 min=-2.000 max=4.000 count=4",
        L"\\e[38;5;200m◘ \\e[0m a much longer label   \tavg=0.875 min=-2.000 max=4.000 count=4",
    }));
}

TEST(Text, legend_skips_unlabelled_layers)
{
    Fig f;
    s9_layers(f);
    f.plot(BLOT_LINE, BLOT_DATA_DOUBLE, 4, S9_X, S9_Y, 4, NULL);
    f.plot(BLOT_LINE, BLOT_DATA_DOUBLE, 4, S9_X, S9_Y, 5, "");
    Rendered r = render(f, BLOT_RENDER_NO_UNICODE | BLOT_RENDER_LEGEND_ABOVE | BLOT_RENDER_LEGEND_DETAILS);
    ASSERT_TRUE(r.ok) << r.message;
    Lines legend;
    for (const auto &l : r.lines)
        if (l.find(LEGEND_MARK) != std::wstring::npos)
            legend.push_back(strip_sgr(l));
    ASSERT_EQ(legend.size(), 2u);
    EXPECT_EQ(legend[0].find(L"◘  sc "), 0u) << to_utf8(legend[0]);
    EXPECT_EQ(legend[1].find(L"◘  a much longer label "), 0u) << to_utf8(legend[1]);
}

// BUG B11: https://git.jukie.net/bart/blot/-/work_items/8
TEST(Text, DISABLED_legend_keeps_screen_height)
{
    const blot_render_flags legends[] = {
        BLOT_RENDER_LEGEND_ABOVE, BLOT_RENDER_LEGEND_BELOW,
        BLOT_RENDER_LEGEND_ABOVE | BLOT_RENDER_LEGEND_BELOW,
    };
    for (auto legend : legends) {
        SCOPED_TRACE(legend);
        Fig f;
        two_layers(f);
        Rendered r = render(f, F | legend);
        ASSERT_TRUE(r.ok) << r.message;
        EXPECT_EQ(r.lines.size(), 16u);
        for (size_t i = 0; i < r.lines.size(); i++)
            EXPECT_FALSE(r.lines[i].empty()) << "line " << i << " is empty";
    }
}

// BUG B12: https://git.jukie.net/bart/blot/-/work_items/8
TEST(Text, DISABLED_no_color_applies_to_legend)
{
    Fig f;
    two_layers(f);
    Rendered r = render(f, F | BLOT_RENDER_LEGEND_ABOVE);
    ASSERT_TRUE(r.ok) << r.message;
    EXPECT_FALSE(has_esc(r.text)) << to_utf8(r.text);
    ASSERT_FALSE(r.lines.empty());
    const std::wstring &first = r.lines[0];
    EXPECT_EQ(first.find(LEGEND_MARK), 0u) << to_utf8(first);
    EXPECT_TRUE(first.size() >= 4 && first.compare(first.size() - 4, 4, L" one") == 0) << to_utf8(first);
}

// BUG B12: https://git.jukie.net/bart/blot/-/work_items/8
TEST(Text, DISABLED_colour_support_disabled)
{
    RUN_ISOLATED(10, {
        have_color_support = false;
        Fig f;
        fix_a_scatter(f);
        Rendered r = render(f, BLOT_RENDER_NO_UNICODE);
        ASSERT_TRUE(r.ok) << r.message;
        EXPECT_FALSE(has_esc(r.text)) << to_utf8(r.lines[0]);
    });
}

// BUG B17: https://git.jukie.net/bart/blot/-/work_items/11
TEST(Text, DISABLED_many_colour_changes)
{
    RUN_ISOLATED(60, {
        /* a checkerboard of two colours over all 1000x1000 cells */
        std::vector<gint32> ax, ay, bx, by;
        for (gint32 y = 0; y < 1000; y++) {
            for (gint32 x = 0; x < 1000; x++) {
                auto &xs = ((x + y) & 1) ? bx : ax;
                auto &ys = ((x + y) & 1) ? by : ay;
                xs.push_back(x);
                ys.push_back(y);
            }
        }
        Fig f(1000, 1000);
        f.lim(0, 999, 0, 999);
        f.plot(BLOT_SCATTER, BLOT_DATA_INT32, ax.size(), ax.data(), ay.data(), 1);
        f.plot(BLOT_SCATTER, BLOT_DATA_INT32, bx.size(), bx.data(), by.data(), 2);
        Rendered r = render(f, BLOT_RENDER_NO_UNICODE | BLOT_RENDER_NO_X_AXIS | BLOT_RENDER_NO_Y_AXIS);
        EXPECT_TRUE(r.ok) << r.message;
    });
}

/* not in the design's list: blot_figure_render() always fills the summary
 * when LEGEND_DETAILS is set, so the "count only" legend of a layer
 * without a summary is reachable only through blot_screen_render() */
TEST(Text, legend_details_without_summary)
{
    const gint32 xs[] = { 0, 5, 9 }, ys[] = { 0, 5, 9 };
    const blot_xy_limits lim = { 0, 9, 0, 9 };
    const blot_dimensions dim = { 10, 12 }, use = { 10, 10 };
    const blot_margins mrg = { 0, 0, 2, 0 };
    const blot_render_flags flags = F | BLOT_RENDER_NO_X_AXIS | BLOT_RENDER_NO_Y_AXIS
        | BLOT_RENDER_LEGEND_ABOVE | BLOT_RENDER_LEGEND_DETAILS;

    blot_layer *lay = blot_layer_new(BLOT_SCATTER, BLOT_DATA_INT32, 3, xs, ys, 1, "pts", NULL);
    ASSERT_TRUE(lay != NULL);
    /* rendered without LEGEND_DETAILS: no summary */
    blot_canvas *can = blot_layer_render(lay, &lim, &use, F, NULL);
    ASSERT_TRUE(can != NULL);
    ASSERT_FALSE(lay->summary.enabled);
    blot_axis *xa = blot_axis_new(false, false, 7, 10, 0, 9, NULL, NULL);
    blot_axis *ya = blot_axis_new(true, false, 7, 10, 0, 9, NULL, NULL);
    blot_screen *scr = blot_screen_new(&dim, &mrg, flags, NULL);
    ASSERT_TRUE(xa && ya && scr);

    GError *error = NULL;
    EXPECT_TRUE(blot_screen_render(scr, &lim, xa, ya, 1, &lay, &can, &error));
    EXPECT_TRUE(error == NULL);
    g_clear_error(&error);
    gsize size = 0;
    const wchar_t *txt = blot_screen_get_text(scr, &size, NULL);
    Lines lines = split_lines(std::wstring(txt, size));
    ASSERT_FALSE(lines.empty());
    EXPECT_EQ(strip_sgr(lines[0]), L"◘  pts   \tcount=3") << to_utf8(lines[0]);

    blot_screen_delete(scr);
    blot_axis_delete(xa);
    blot_axis_delete(ya);
    blot_canvas_delete(can);
    blot_layer_delete(lay);
}
