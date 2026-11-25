/* colour, legend, clear (#2, group S).  Colour tests compare complete
 * lines without trimming; in the expected text "\e" stands for ESC. */
#include "blot_test.hpp"

#include <cfloat>
#include <clocale>
#include <cwchar>
#include <cstdlib>

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
    /* the minimum screen (#17); one cell per unit: x (or y) k is in column
     * (or row) k */
    Fig f(20, 12);
    f.lim(0, 20, 0, 12);
    f.plot(BLOT_SCATTER, BLOT_DATA_INT32, 3, ax, ay, 2);
    f.plot(BLOT_SCATTER, BLOT_DATA_INT32, 2, bx, by, 9);
    Rendered r = render(f, BLOT_RENDER_NO_UNICODE | BLOT_RENDER_NO_X_AXIS | BLOT_RENDER_NO_Y_AXIS);
    ASSERT_TRUE(r.ok) << r.message;
    Lines expected(12, std::wstring(20, L' '));
    /* row 6 is line 5, row 4 line 7 */
    expected[5] = sgr(L"    \\e[38;5;9m*               \\e[0m");
    expected[7] = sgr(L"   \\e[38;5;2m*\\e[38;5;9m*\\e[38;5;2m*              \\e[0m");
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
    /* 80 columns: the whole lines fit (#11 cuts them at the screen width) */
    f.size(80, 16);
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
    /* 40 columns: cut between items (#29) */
    EXPECT_LINES_EQ(legend, (Lines{
        L"◘  sc   \tavg=0.875 min=-2.000",
        L"◘  a much longer label",
    }));
}

TEST(Text, legend_keeps_screen_height)
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

TEST(Text, no_color_applies_to_legend)
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

TEST(Text, colour_support_disabled)
{
    RUN_ISOLATED(10, {
        have_color_support = false;
        Fig f;
        fix_a_scatter(f);
        Rendered r = render(f, BLOT_RENDER_NO_UNICODE);
        ASSERT_TRUE(r.ok) << r.message;
        EXPECT_FALSE(has_esc(r.text)) << to_utf8(r.lines[0]);

        /* the legend's colour and reset too (#8); 2 more rows keep the
         * 10-row plot area with one legend line above and one below */
        f.size(28, 14);
        Rendered l = render(f, BLOT_RENDER_NO_UNICODE | BLOT_RENDER_LEGEND_ABOVE | BLOT_RENDER_LEGEND_BELOW);
        ASSERT_TRUE(l.ok) << l.message;
        EXPECT_FALSE(has_esc(l.text)) << to_utf8(l.text);
        ASSERT_EQ(l.lines.size(), 14u);
        EXPECT_EQ(l.lines.front(), L"◘  pts");
        EXPECT_EQ(l.lines.back(), L"◘  pts");
    });
}

/* not in the design's list (#8): where the legend rows go.  The legend
 * takes the rows reserved for it, above the plot or below the X-axis, and
 * layers without a label reserve nothing */
TEST(Text, legend_rows_and_placement)
{
    const gint32 xs[] = { 0, 10 };
    const Lines legend = { L"◘  one", L"◘  two" };

    Fig f;
    two_layers(f);
    f.plot(BLOT_LINE, BLOT_DATA_INT32, 2, xs, xs, 3, NULL);
    f.plot(BLOT_LINE, BLOT_DATA_INT32, 2, xs, xs, 4, "");

    Rendered a = render(f, F | BLOT_RENDER_LEGEND_ABOVE);
    ASSERT_TRUE(a.ok) << a.message;
    ASSERT_EQ(a.lines.size(), 16u);
    EXPECT_EQ(a.mrg.top, 2u);
    EXPECT_EQ(a.mrg.bottom, 2u);
    EXPECT_LINES_EQ(Lines(a.lines.begin(), a.lines.begin() + 2), legend);
    EXPECT_EQ(a.lines[14].find(L"+"), a.mrg.left - 1) << "X-axis line: " << to_utf8(a.lines[14]);

    Rendered b = render(f, F | BLOT_RENDER_LEGEND_BELOW);
    ASSERT_TRUE(b.ok) << b.message;
    ASSERT_EQ(b.lines.size(), 16u);
    EXPECT_EQ(b.mrg.top, 0u);
    EXPECT_EQ(b.mrg.bottom, 4u);
    EXPECT_LINES_EQ(Lines(b.lines.end() - 2, b.lines.end()), legend);
    EXPECT_EQ(b.lines[12].find(L"+"), b.mrg.left - 1) << "X-axis line: " << to_utf8(b.lines[12]);
    /* same plot height as above: the plot rows and axes are the same */
    EXPECT_LINES_EQ(Lines(b.lines.begin(), b.lines.begin() + 14),
                    Lines(a.lines.begin() + 2, a.lines.end()));

    /* no X-axis: the legend below is the whole bottom margin */
    Rendered c = render(f, F | BLOT_RENDER_NO_X_AXIS | BLOT_RENDER_LEGEND_BELOW);
    ASSERT_TRUE(c.ok) << c.message;
    ASSERT_EQ(c.lines.size(), 16u);
    EXPECT_EQ(c.mrg.top, 0u);
    EXPECT_EQ(c.mrg.bottom, 2u);
    EXPECT_LINES_EQ(Lines(c.lines.end() - 2, c.lines.end()), legend);
    for (size_t i = 0; i < 14; i++) {
        EXPECT_FALSE(c.lines[i].empty()) << "line " << i << " is empty";
        EXPECT_EQ(c.lines[i].find(L"+"), std::wstring::npos) << "line " << i << ": " << to_utf8(c.lines[i]);
    }

    /* only unlabelled layers: the legend flags change nothing */
    Fig u;
    u.size(40, 16);
    u.plot(BLOT_LINE, BLOT_DATA_INT32, 3, TWO_X, TWO_Y1, 1, NULL);
    u.plot(BLOT_LINE, BLOT_DATA_INT32, 3, TWO_X, TWO_Y2, 2, "");
    Rendered n = render(u, F);
    Rendered l = render(u, F | BLOT_RENDER_LEGEND_ABOVE | BLOT_RENDER_LEGEND_BELOW);
    ASSERT_TRUE(n.ok && l.ok) << n.message << l.message;
    EXPECT_EQ(l.text, n.text);
    EXPECT_EQ(n.lines.size(), 16u);
}

/* not in the design's list (#8): a legend taller than the rows the
 * margins can give it (at most half the screen) is cut, not appended */
TEST(Text, legend_cut_to_reserved_rows)
{
    static const char *const labels[] = {
        "l0", "l1", "l2", "l3", "l4", "l5", "l6", "l7", "l8", "l9", "l10", "l11", "l12", "l13",
    };
    for (auto legend : { BLOT_RENDER_LEGEND_ABOVE, BLOT_RENDER_LEGEND_BELOW }) {
        SCOPED_TRACE(legend);
        Fig f;
        f.size(40, 26);
        for (unsigned i = 0; i < 14; i++)
            f.plot(BLOT_LINE, BLOT_DATA_INT32, 3, TWO_X, TWO_Y1, 1, labels[i]);
        Rendered r = render(f, F | legend);
        ASSERT_TRUE(r.ok) << r.message;
        EXPECT_EQ(r.lines.size(), 26u);
        unsigned marks = 0;
        for (const auto &l : r.lines)
            if (l.find(LEGEND_MARK) != std::wstring::npos)
                marks++;
        /* 14 labels; the margins take at most half the screen, 13
         * rows: 13 legend lines above, or 13 less the 2 X-axis rows below */
        EXPECT_EQ(marks, legend == BLOT_RENDER_LEGEND_ABOVE ? 13u : 11u);
    }
}

// B17, fixed in #11: https://git.jukie.net/bart/blot/-/work_items/11
TEST(Text, many_colour_changes)
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
    /* 24 columns: the whole line fits (#11 cuts it at the screen width) */
    const blot_dimensions dim = { 24, 12 }, use = { 24, 10 };
    const blot_margins mrg = { 0, 0, 2, 0 };
    const blot_render_flags flags = F | BLOT_RENDER_NO_X_AXIS | BLOT_RENDER_NO_Y_AXIS
        | BLOT_RENDER_LEGEND_ABOVE | BLOT_RENDER_LEGEND_DETAILS;

    blot_layer *lay = blot_layer_new(BLOT_SCATTER, BLOT_DATA_INT32, 3, xs, ys, 1, "pts", NULL);
    ASSERT_TRUE(lay != NULL);
    /* rendered without LEGEND_DETAILS: no summary */
    blot_canvas *can = blot_layer_render(lay, &lim, &use, F, NULL);
    ASSERT_TRUE(can != NULL);
    ASSERT_FALSE(lay->summary.enabled);
    blot_axis *xa = blot_axis_new(false, false, 7, 24, 0, 9, NULL, NULL);
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

namespace {

/* the columns a line takes on a terminal: escapes take none, a tab goes
 * to the next multiple of 8 */
unsigned display_width(const std::wstring &line)
{
    unsigned col = 0;
    for (wchar_t wc : strip_sgr(line))
        col = wc == L'\t' ? (col / 8 + 1) * 8 : col + 1;
    return col;
}

void expect_lines_fit(const Rendered &r, unsigned cols, unsigned rows)
{
    ASSERT_TRUE(r.ok) << r.message;
    EXPECT_EQ(r.lines.size(), rows);
    for (size_t i = 0; i < r.lines.size(); i++)
        EXPECT_LE(display_width(r.lines[i]), cols) << "line " << i << " |" << to_utf8(r.lines[i]) << "|";
}

const gint32 LEG_X[] = { 0, 5, 9 }, LEG_Y[] = { 0, 5, 9 };

} // namespace

/* #11 (note 34933): a label far longer than the screen buffer allows
 * (cols * rows * 16 characters) failed the render; it is cut at the screen
 * width, colour escapes not counted.  #29: the cut label ends in '~' */
TEST(Text, long_label_cut_to_width)
{
    const std::string label(2000, 'x');
    const blot_render_flags flags = BLOT_RENDER_NO_UNICODE | BLOT_RENDER_NO_X_AXIS | BLOT_RENDER_NO_Y_AXIS;
    for (auto legend : { BLOT_RENDER_LEGEND_ABOVE, BLOT_RENDER_LEGEND_BELOW }) {
        for (auto colour : { BLOT_RENDER_NONE, BLOT_RENDER_NO_COLOR }) {
            SCOPED_TRACE(testing::Message() << "legend=" << legend << " colour=" << colour);
            Fig f(20, 12);
            f.lim(0, 9, 0, 9);
            f.plot(BLOT_SCATTER, BLOT_DATA_INT32, 3, LEG_X, LEG_Y, 1, label.c_str());
            Rendered r = render(f, flags | legend | colour);
            expect_lines_fit(r, 20, 12);
            ASSERT_EQ(r.lines.size(), 12u);
            const std::wstring &l = legend == BLOT_RENDER_LEGEND_ABOVE ? r.lines.front() : r.lines.back();
            EXPECT_EQ(strip_sgr(l), L"◘  " + std::wstring(16, L'x') + L"~") << to_utf8(l);
            EXPECT_EQ(has_esc(r.text), colour == BLOT_RENDER_NONE);
        }
    }
}

/* #11 (note 34955): a LEGEND_DETAILS line wider than the screen wrapped on
 * the terminal; it is cut at the screen width, the tab counted to its stop.
 * #29: only between items, so no value is cut */
TEST(Text, legend_details_cut_to_width)
{
    Fig f;
    s9_layers(f);       /* 40 columns */
    Rendered r = render(f, F | BLOT_RENDER_LEGEND_BELOW | BLOT_RENDER_LEGEND_DETAILS);
    expect_lines_fit(r, 40, 16);
    ASSERT_EQ(r.lines.size(), 16u);
    EXPECT_LINES_EQ(Lines(r.lines.end() - 2, r.lines.end()), (Lines{
        L"◘  sc   \tavg=0.875 min=-2.000",
        L"◘  a much longer label",
    }));

    /* a label that reaches the tab stop: no item fits after it, and the
     * spaces and tab before the items go with them (#29) */
    Fig g(20, 14);
    g.lim(0, 9, 0, 9);
    g.plot(BLOT_SCATTER, BLOT_DATA_INT32, 3, LEG_X, LEG_Y, 1, "abcdefghijklm");
    Rendered d = render(g, F | BLOT_RENDER_LEGEND_ABOVE | BLOT_RENDER_LEGEND_DETAILS);
    expect_lines_fit(d, 20, 14);
    ASSERT_FALSE(d.lines.empty());
    EXPECT_EQ(d.lines.front(), L"◘  abcdefghijklm") << to_utf8(d.lines.front());
}

/* #11 (note 34955): a newline in a label added a line, and an escape
 * sequence in one went to the terminal; control characters become '?'.
 * A tab is kept (see legend_label_tab_kept) */
TEST(Text, legend_control_characters_replaced)
{
    for (auto legend : { BLOT_RENDER_LEGEND_ABOVE, BLOT_RENDER_LEGEND_BELOW }) {
        SCOPED_TRACE(legend);
        Fig f(40, 14);
        f.lim(0, 9, 0, 9);
        f.plot(BLOT_SCATTER, BLOT_DATA_INT32, 3, LEG_X, LEG_Y, 1, "a\nb\x1b[31mc\td\re\x7f");
        Rendered r = render(f, F | legend);
        expect_lines_fit(r, 40, 14);
        EXPECT_FALSE(has_esc(r.text)) << to_utf8(r.text);
        ASSERT_EQ(r.lines.size(), 14u);
        const std::wstring &l = legend == BLOT_RENDER_LEGEND_ABOVE ? r.lines.front() : r.lines.back();
        EXPECT_EQ(l, L"◘  a?b?[31mc\td?e?") << to_utf8(l);
    }
}

/* not in the design's list (#11): bytes that aren't text failed the render
 * with EILSEQ; each one becomes '?'.  #18: labels are UTF-8 in every locale
 * (the tests run in the C locale), so "café" is kept and only a byte that
 * isn't UTF-8 (Latin-1 é) is a '?' */
TEST(Text, legend_label_not_valid_text)
{
    Fig f(40, 13);
    f.lim(0, 9, 0, 9);
    f.plot(BLOT_SCATTER, BLOT_DATA_INT32, 3, LEG_X, LEG_Y, 1, "caf\xc3\xa9s caf\xe9s");
    Rendered r = render(f, F | BLOT_RENDER_LEGEND_ABOVE);
    expect_lines_fit(r, 40, 13);
    ASSERT_FALSE(r.lines.empty());
    EXPECT_EQ(r.lines.front(), L"◘  caf\u00e9s caf?s") << to_utf8(r.lines.front());
}

/* #11 (note 34955): a legend taller than the screen can give it failed the
 * render with EINVAL; it is cut so that the plot keeps BLOT_MIN_PLOT_ROWS
 * rows */
TEST(Text, tall_legend_is_cut)
{
    static const char *const labels[] = { "l0", "l1", "l2" };

    /* 12 rows (the minimum, #17) with the X-axis: no room for a legend */
    for (auto legend : { BLOT_RENDER_LEGEND_ABOVE, BLOT_RENDER_LEGEND_BELOW }) {
        SCOPED_TRACE(legend);
        Fig f(28, 12);
        f.lim(0, 9, 0, 9);
        f.plot(BLOT_SCATTER, BLOT_DATA_INT32, 3, LEG_X, LEG_Y, 1, "pts");
        Rendered r = render(f, F | legend);
        expect_lines_fit(r, 28, 12);
        EXPECT_EQ(r.plot_rows(), (unsigned)BLOT_MIN_PLOT_ROWS);
        EXPECT_EQ(r.text.find(LEGEND_MARK), std::wstring::npos) << to_utf8(r.text);
    }

    /* 16 rows, 3 labels, both legends: 4 rows left, 2 each */
    Fig f(28, 16);
    f.lim(0, 9, 0, 9);
    for (auto label : labels)
        f.plot(BLOT_SCATTER, BLOT_DATA_INT32, 3, LEG_X, LEG_Y, 1, label);
    Rendered r = render(f, F | BLOT_RENDER_LEGEND_ABOVE | BLOT_RENDER_LEGEND_BELOW);
    expect_lines_fit(r, 28, 16);
    ASSERT_EQ(r.lines.size(), 16u);
    EXPECT_EQ(r.plot_rows(), (unsigned)BLOT_MIN_PLOT_ROWS);
    EXPECT_EQ(r.mrg.top, 2u);
    EXPECT_EQ(r.mrg.bottom, 4u);
    EXPECT_LINES_EQ(Lines(r.lines.begin(), r.lines.begin() + 2), (Lines{ L"◘  l0", L"◘  l1" }));
    EXPECT_LINES_EQ(Lines(r.lines.end() - 2, r.lines.end()), (Lines{ L"◘  l0", L"◘  l1" }));

    /* one row more: the legend above gets it */
    f.size(28, 17);
    r = render(f, F | BLOT_RENDER_LEGEND_ABOVE | BLOT_RENDER_LEGEND_BELOW);
    expect_lines_fit(r, 28, 17);
    EXPECT_EQ(r.plot_rows(), (unsigned)BLOT_MIN_PLOT_ROWS);
    EXPECT_EQ(r.mrg.top, 3u);
    EXPECT_EQ(r.mrg.bottom, 4u);
}

/* #11 review: a tab in a label is kept, as before, and goes to its tab
 * stop; one whose tab stop is past the screen edge cuts the label, which
 * ends in '~' (#29) */
TEST(Text, legend_label_tab_kept)
{
    Fig f(40, 13);
    f.lim(0, 9, 0, 9);
    f.plot(BLOT_SCATTER, BLOT_DATA_INT32, 3, LEG_X, LEG_Y, 1, "a\tb");
    Rendered r = render(f, F | BLOT_RENDER_LEGEND_ABOVE);
    expect_lines_fit(r, 40, 13);
    ASSERT_FALSE(r.lines.empty());
    EXPECT_EQ(r.lines.front(), L"◘  a\tb") << to_utf8(r.lines.front());

    Fig g(20, 12);
    g.lim(0, 9, 0, 9);
    g.plot(BLOT_SCATTER, BLOT_DATA_INT32, 3, LEG_X, LEG_Y, 1, "abcdefghijklmno\tz");
    Rendered n = render(g, F | BLOT_RENDER_NO_X_AXIS | BLOT_RENDER_NO_Y_AXIS | BLOT_RENDER_LEGEND_ABOVE);
    expect_lines_fit(n, 20, 12);
    ASSERT_FALSE(n.lines.empty());
    EXPECT_EQ(n.lines.front(), L"◘  abcdefghijklmno~") << to_utf8(n.lines.front());
}

namespace {

/* the columns a line takes in the current locale (wcwidth), escapes and
 * tabs as in display_width() */
unsigned display_width_locale(const std::wstring &line)
{
    unsigned col = 0;
    for (wchar_t wc : strip_sgr(line)) {
        if (wc == L'\t') {
            col = (col / 8 + 1) * 8;
        } else {
            int w = wcwidth(wc);
            col += w >= 0 ? (unsigned)w : 1;
        }
    }
    return col;
}

/* the legend line of one labelled layer, rendered in C.UTF-8 on a screen
 * of cols x 12 without axes (the caller is an isolated child) */
std::wstring utf8_legend_line(const char *label, unsigned cols)
{
    Fig f(cols, 12);
    f.lim(0, 9, 0, 9);
    f.plot(BLOT_SCATTER, BLOT_DATA_INT32, 3, LEG_X, LEG_Y, 1, label);
    Rendered r = render(f, F | BLOT_RENDER_NO_X_AXIS | BLOT_RENDER_NO_Y_AXIS | BLOT_RENDER_LEGEND_ABOVE);
    EXPECT_TRUE(r.ok) << r.message;
    EXPECT_EQ(r.lines.size(), 12u);
    if (r.lines.empty())
        return L"";
    for (const auto &l : r.lines)
        EXPECT_LE(display_width_locale(l), cols) << "|" << to_utf8(l) << "|";
    return r.lines.front();
}

/* C.UTF-8 is built into glibc 2.35+ and shipped by Debian's libc-bin: a
 * missing locale is a failure, not a skip */
#define SET_UTF8_LOCALE() \
    ASSERT_TRUE(setlocale(LC_ALL, "C.UTF-8") != NULL) << "the C.UTF-8 locale is missing"

} // namespace

/* #11 review M1: the legend in a UTF-8 locale.  Valid multibyte text is
 * kept, wide characters take 2 columns and a combining mark none, C1
 * controls become '?', and an invalid byte becomes one '?' */
TEST(Text, legend_utf8_label_kept)
{
    RUN_ISOLATED(10, {
        SET_UTF8_LOCALE();
        EXPECT_EQ(utf8_legend_line("caf\xc3\xa9", 40), L"◘  caf\u00e9");
    });
}

TEST(Text, legend_utf8_wide_characters_cut)
{
    RUN_ISOLATED(10, {
        SET_UTF8_LOCALE();
        /* "◘  " is 3 columns, each CJK character 2, and a cut label
         * ends in '~' (#29): at 20 columns (the minimum width, #17), 8
         * characters and the '~' take 20 columns */
        const char *cjk = "\xe6\x97\xa5\xe6\x9c\xac\xe8\xaa\x9e\xe3\x83\x86\xe3\x82\xad"
                          "\xe3\x82\xb9\xe3\x83\x88\xe6\x97\xa5\xe6\x9c\xac\xe8\xaa\x9e";
        std::wstring l = utf8_legend_line(cjk, 20);
        EXPECT_EQ(l, L"◘  \u65e5\u672c\u8a9e\u30c6\u30ad\u30b9\u30c8\u65e5~") << to_utf8(l);
        EXPECT_EQ(display_width_locale(l), 20u);
        /* 21 columns: the 9th fits, but not with the '~' */
        l = utf8_legend_line(cjk, 21);
        EXPECT_EQ(l, L"◘  \u65e5\u672c\u8a9e\u30c6\u30ad\u30b9\u30c8\u65e5~") << to_utf8(l);
        EXPECT_EQ(display_width_locale(l), 20u);
        /* 22 columns: the 9th and the '~' */
        l = utf8_legend_line(cjk, 22);
        EXPECT_EQ(l, L"◘  \u65e5\u672c\u8a9e\u30c6\u30ad\u30b9\u30c8\u65e5\u672c~") << to_utf8(l);
        EXPECT_EQ(display_width_locale(l), 22u);
        /* 23 columns: the whole label, not cut */
        l = utf8_legend_line(cjk, 23);
        EXPECT_EQ(l, L"◘  \u65e5\u672c\u8a9e\u30c6\u30ad\u30b9\u30c8\u65e5\u672c\u8a9e") << to_utf8(l);
    });
}

TEST(Text, legend_utf8_combining_mark_takes_no_column)
{
    RUN_ISOLATED(10, {
        SET_UTF8_LOCALE();
        /* 17 x "e" + U+0301: 17 columns, the whole label fits in 20 */
        std::string label;
        std::wstring expected = L"◘  ";
        for (int i = 0; i < 17; i++) {
            label += "e\xcc\x81";
            expected += L"e\u0301";
        }
        std::wstring l = utf8_legend_line(label.c_str(), 20);
        EXPECT_EQ(l, expected) << to_utf8(l);
        EXPECT_EQ(display_width_locale(l), 20u);
    });
}

TEST(Text, legend_utf8_c1_control_replaced)
{
    RUN_ISOLATED(10, {
        SET_UTF8_LOCALE();
        /* U+009B is CSI, an escape sequence on some terminals */
        std::wstring l = utf8_legend_line("\xc2\x9b" "31m", 40);
        EXPECT_EQ(l, L"◘  ?31m") << to_utf8(l);
    });
}

TEST(Text, legend_utf8_invalid_byte_is_one_question_mark)
{
    RUN_ISOLATED(10, {
        SET_UTF8_LOCALE();
        std::wstring l = utf8_legend_line("caf\xc3\xa9 \xff ok", 40);
        EXPECT_EQ(l, L"◘  caf\u00e9 ? ok") << to_utf8(l);
    });
}

/* #18: labels are UTF-8 and their width is the same in every locale.  In
 * the C locale wcwidth() is -1 for anything past ASCII, so a wide
 * character was counted as one column; the C locale's mbrtowc() can't
 * read UTF-8 either, so a legend label became '?'s and a custom X label
 * failed the render (swprintf).  Each test runs the same checks in the C
 * locale and in C.UTF-8, each in an isolated child, on the minimum
 * screen.  Every line is measured by display_width_utf8(), which doesn't
 * use blot: glibc's own C.UTF-8 widths. */
namespace {

const char *const WIDE_LOCALES[] = { "C", "C.UTF-8" };

/* the columns a line takes on a UTF-8 terminal, whatever the process's
 * locale: glibc's wcwidth() in C.UTF-8 (through uselocale(), so the
 * locale of the test stays), escapes and tabs as in display_width().  A
 * character glibc can't measure counts 2, the most it could take */
unsigned display_width_utf8(const std::wstring &line)
{
    locale_t utf8 = newlocale(LC_CTYPE_MASK, "C.UTF-8", (locale_t)0);
    EXPECT_TRUE(utf8 != (locale_t)0) << "the C.UTF-8 locale is missing";
    if (utf8 == (locale_t)0)
        return ~0u;
    locale_t old = uselocale(utf8);
    unsigned col = 0;
    for (wchar_t wc : strip_sgr(line)) {
        if (wc == L'\t') {
            col = (col / 8 + 1) * 8;
        } else {
            int w = wcwidth(wc);
            col += w >= 0 ? (unsigned)w : 2;
        }
    }
    uselocale(old);
    freelocale(utf8);
    return col;
}

void expect_lines_fit_utf8(const Rendered &r, unsigned cols, unsigned rows)
{
    ASSERT_TRUE(r.ok) << r.message;
    EXPECT_EQ(r.lines.size(), rows);
    for (size_t i = 0; i < r.lines.size(); i++)
        EXPECT_LE(display_width_utf8(r.lines[i]), cols) << "line " << i << " |" << to_utf8(r.lines[i]) << "|";
}

#define SET_LOCALE(loc) ASSERT_TRUE(setlocale(LC_ALL, (loc)) != NULL) << "the " << (loc) << " locale is missing"

/* the legend line of one labelled layer on the minimum screen, without
 * axes (the caller is an isolated child) */
std::wstring min_legend_line(const char *label)
{
    Fig f(BLOT_MIN_COLS, BLOT_MIN_ROWS);
    f.lim(0, 9, 0, 9);
    f.plot(BLOT_SCATTER, BLOT_DATA_INT32, 3, LEG_X, LEG_Y, 1, label);
    Rendered r = render(f, F | BLOT_RENDER_NO_X_AXIS | BLOT_RENDER_NO_Y_AXIS | BLOT_RENDER_LEGEND_ABOVE);
    expect_lines_fit_utf8(r, BLOT_MIN_COLS, BLOT_MIN_ROWS);
    if (r.lines.empty())
        return L"";
    return r.lines.front();
}

/* the X axis and its label row of a bar plot with custom X labels on a
 * screen of the minimum width, and the legend below if it has one (the
 * caller is an isolated child) */
Lines min_x_rows(std::vector<const char *> labels, const char *legend = NULL)
{
    const gint32 ys[] = { 3, 1, 4, 1 };
    /* one more row for the legend: the plot keeps its minimum rows */
    unsigned rows = BLOT_MIN_ROWS + (legend ? 1 : 0);
    Fig f(BLOT_MIN_COLS, rows);
    EXPECT_TRUE(blot_figure_set_x_axis_labels(f, labels.size(), (char **)labels.data(), NULL));
    f.plot(BLOT_BAR, BLOT_DATA_INT32, 4, NULL, ys, 1, legend);
    Rendered r = render(f, F | (legend ? BLOT_RENDER_LEGEND_BELOW : BLOT_RENDER_NONE));
    expect_lines_fit_utf8(r, BLOT_MIN_COLS, rows);
    if (!r.ok)
        return {};
    unsigned ax = r.mrg.top + r.plot_rows();
    return Lines(r.lines.begin() + ax, r.lines.end());
}

} // namespace

TEST(Text, legend_wide_characters_any_locale)
{
    for (const char *loc : WIDE_LOCALES) {
        SCOPED_TRACE(loc);
        RUN_ISOLATED(10, {
            SET_LOCALE(loc);
            /* "◘  " is 3 columns and leaves 17; a cut label ends in '~'
             * (#29), so 16 for the label: 8 wide characters */
            /* CJK: 日本語テキスト日本語 */
            EXPECT_EQ(min_legend_line("\xe6\x97\xa5\xe6\x9c\xac\xe8\xaa\x9e\xe3\x83\x86\xe3\x82\xad"
                                      "\xe3\x82\xb9\xe3\x83\x88\xe6\x97\xa5\xe6\x9c\xac\xe8\xaa\x9e"),
                      L"◘  日本語テキスト日~");
            /* Hangul: 한국어 x 4 */
            EXPECT_EQ(min_legend_line("\xed\x95\x9c\xea\xb5\xad\xec\x96\xb4\xed\x95\x9c\xea\xb5\xad\xec\x96\xb4"
                                      "\xed\x95\x9c\xea\xb5\xad\xec\x96\xb4\xed\x95\x9c\xea\xb5\xad\xec\x96\xb4"),
                      L"◘  한국어한국어한국~");
            /* fullwidth forms: ＡＢＣＤＥＦＧＨＩＪ */
            EXPECT_EQ(min_legend_line("\xef\xbc\xa1\xef\xbc\xa2\xef\xbc\xa3\xef\xbc\xa4\xef\xbc\xa5"
                                      "\xef\xbc\xa6\xef\xbc\xa7\xef\xbc\xa8\xef\xbc\xa9\xef\xbc\xaa"),
                      L"◘  ＡＢＣＤＥＦＧＨ~");
            /* emoji: 😀🚀🎉👍🔥 twice */
            EXPECT_EQ(min_legend_line("\xf0\x9f\x98\x80\xf0\x9f\x9a\x80\xf0\x9f\x8e\x89\xf0\x9f\x91\x8d\xf0\x9f\x94\xa5"
                                      "\xf0\x9f\x98\x80\xf0\x9f\x9a\x80\xf0\x9f\x8e\x89\xf0\x9f\x91\x8d\xf0\x9f\x94\xa5"),
                      L"◘  \U0001f600\U0001f680\U0001f389\U0001f44d\U0001f525\U0001f600\U0001f680\U0001f389~");
            /* a wide character can't take the last column before the
             * '~': a日本語テキスト日本語 is cut after ト, at 18 */
            std::wstring l = min_legend_line("a\xe6\x97\xa5\xe6\x9c\xac\xe8\xaa\x9e\xe3\x83\x86\xe3\x82\xad"
                                             "\xe3\x82\xb9\xe3\x83\x88\xe6\x97\xa5\xe6\x9c\xac\xe8\xaa\x9e");
            EXPECT_EQ(l, L"◘  a日本語テキスト~");
            EXPECT_EQ(display_width_utf8(l), 19u);
            /* 8 wide characters fit whole, with no '~' */
            EXPECT_EQ(min_legend_line("\xe6\x97\xa5\xe6\x9c\xac\xe8\xaa\x9e\xe3\x83\x86\xe3\x82\xad"
                                      "\xe3\x82\xb9\xe3\x83\x88\xe6\x97\xa5"),
                      L"◘  日本語テキスト日");
        });
    }
}

TEST(Text, legend_combining_marks_any_locale)
{
    for (const char *loc : WIDE_LOCALES) {
        SCOPED_TRACE(loc);
        RUN_ISOLATED(10, {
            SET_LOCALE(loc);
            /* 17 x "e" + U+0301: 17 columns, the whole label fits */
            std::string label;
            std::wstring whole = L"◘  ";
            for (int i = 0; i < 17; i++) {
                label += "e\xcc\x81";
                whole += L"é";
            }
            std::wstring l = min_legend_line(label.c_str());
            EXPECT_EQ(l, whole);
            EXPECT_EQ(display_width_utf8(l), 20u);
            /* one more: cut after 16, the mark stays with its letter */
            label += "e\xcc\x81";
            l = min_legend_line(label.c_str());
            EXPECT_EQ(l, whole.substr(0, whole.size() - 2) + L"~");
            EXPECT_EQ(display_width_utf8(l), 20u);
        });
    }
}

/* the decision for the C locale (#18): labels are read as UTF-8, as in a
 * UTF-8 locale, not as '?'s; a byte that isn't UTF-8 is still one '?' */
TEST(Text, legend_utf8_label_any_locale)
{
    for (const char *loc : WIDE_LOCALES) {
        SCOPED_TRACE(loc);
        RUN_ISOLATED(10, {
            SET_LOCALE(loc);
            EXPECT_EQ(min_legend_line("caf\xc3\xa9"), L"◘  café");
            EXPECT_EQ(min_legend_line("caf\xe9 \xe6\x97 ok"), L"◘  caf? ?? ok");
            EXPECT_EQ(min_legend_line("\xc2\x9b" "31m"), L"◘  ?31m");
        });
    }
}

/* custom X labels (#9 cuts them to their room) are cut by columns.  On
 * the minimum screen the plot is 12 columns from column 8, so with 4
 * labels the ticks are 3 columns apart: 2 columns for each label and a
 * blank, 3 for the last one */
TEST(Text, x_labels_wide_characters_any_locale)
{
    for (const char *loc : WIDE_LOCALES) {
        SCOPED_TRACE(loc);
        RUN_ISOLATED(10, {
            SET_LOCALE(loc);
            /* 日本 한국 ＡＢ 😀🚀: one wide character each */
            EXPECT_LINES_EQ(min_x_rows({ "\xe6\x97\xa5\xe6\x9c\xac", "\xed\x95\x9c\xea\xb5\xad",
                                         "\xef\xbc\xa1\xef\xbc\xa2",
                                         "\xf0\x9f\x98\x80\xf0\x9f\x9a\x80" }),
                            Lines({ L"       +*--*--*--*--",
                                    L"        \u65e5 \ud55c \uff21 \U0001f600 " }));
            /* éééé (combining marks) is 4 columns, cut to 2; a日 is 3,
             * and 日 can't take the second column; 日b is cut to 日.  A
             * CJK legend below: 日本語テキスト日本 is cut to 8 and '~' */
            EXPECT_LINES_EQ(min_x_rows({ "e\xcc\x81" "e\xcc\x81" "e\xcc\x81" "e\xcc\x81",
                                         "a\xe6\x97\xa5", "\xe6\x97\xa5" "b", "zz" },
                                       "\xe6\x97\xa5\xe6\x9c\xac\xe8\xaa\x9e\xe3\x83\x86\xe3\x82\xad"
                                       "\xe3\x82\xb9\xe3\x83\x88\xe6\x97\xa5\xe6\x9c\xac"),
                            Lines({ L"       +*--*--*--*--",
                                    L"        e\u0301e\u0301 a  \u65e5 zz ",
                                    L"◘  \u65e5\u672c\u8a9e\u30c6\u30ad\u30b9\u30c8\u65e5~" }));
            /* 日本語テキスト日本 and 日本語テキスト日本語: cut to the same
             * text, so only the first gets a tick (#9); longer than the
             * axis, so it is cut to 11 of its 12 columns and marked: 5
             * wide characters and the '~' */
            EXPECT_LINES_EQ(min_x_rows({ "\xe6\x97\xa5\xe6\x9c\xac\xe8\xaa\x9e\xe3\x83\x86\xe3\x82\xad"
                                         "\xe3\x82\xb9\xe3\x83\x88\xe6\x97\xa5\xe6\x9c\xac",
                                         "\xe6\x97\xa5\xe6\x9c\xac\xe8\xaa\x9e\xe3\x83\x86\xe3\x82\xad"
                                         "\xe3\x82\xb9\xe3\x83\x88\xe6\x97\xa5\xe6\x9c\xac\xe8\xaa\x9e" }),
                            Lines({ L"       +*-----------",
                                    L"        \u65e5\u672c\u8a9e\u30c6\u30ad~ " }));
        });
    }
}

/* #22: the legend details of a layer with non-finite Y values.  The
 * summary takes the points whose X and Y are both finite: avg, min and
 * max are over them, count is how many there are, and "(+N non-finite)"
 * says how many points were left out.  With none left, avg, min and max
 * are "-".  No legend prints nan or inf */
namespace {

const double NF_X[] = { 0, 1, 2 };

std::wstring details_line(blot_plot_type t, size_t n, const double *xs, const double *ys)
{
    /* wide enough for the longest line: #11 cuts it at the screen width */
    Fig f(120, 14);
    f.lim(0, 9, 0, 9);
    f.plot(t, BLOT_DATA_DOUBLE, n, xs, ys, 1, "data");
    Rendered r = render(f, F | BLOT_RENDER_LEGEND_ABOVE | BLOT_RENDER_LEGEND_DETAILS);
    EXPECT_TRUE(r.ok) << r.message;
    if (r.lines.empty())
        return L"";
    for (const auto &l : r.lines) {
        std::string s = to_utf8(l);
        for (auto &c : s)
            c = (char)tolower((unsigned char)c);
        EXPECT_EQ(s.find("nan"), std::string::npos) << s;
        EXPECT_EQ(s.find("inf"), std::string::npos) << s;
    }
    return r.lines.front();
}

} // namespace

// BUG B22: https://git.jukie.net/bart/blot/-/work_items/22
TEST(Text, legend_details_nan)
{
    const double ys[] = { 1, NAN, 3 };
    for (auto t : { BLOT_SCATTER, BLOT_LINE }) {
        SCOPED_TRACE(t);
        EXPECT_EQ(details_line(t, 3, NF_X, ys),
                  L"◘  data   \tavg=2.000 min=1.000 max=3.000 count=2 (+1 non-finite)");
    }
}

TEST(Text, legend_details_inf)
{
    const double ys[] = { 1, INFINITY, 3 };
    EXPECT_EQ(details_line(BLOT_SCATTER, 3, NF_X, ys),
              L"◘  data   \tavg=2.000 min=1.000 max=3.000 count=2 (+1 non-finite)");
}

TEST(Text, legend_details_minus_inf)
{
    const double ys[] = { -INFINITY, 1, 3 };
    EXPECT_EQ(details_line(BLOT_SCATTER, 3, NF_X, ys),
              L"◘  data   \tavg=2.000 min=1.000 max=3.000 count=2 (+1 non-finite)");
}

TEST(Text, legend_details_non_finite_x)
{
    /* a point with a non-finite X isn't drawn, nor summarized */
    const double xs[] = { 0, NAN, 2 }, ys[] = { 1, 5, 3 };
    EXPECT_EQ(details_line(BLOT_SCATTER, 3, xs, ys),
              L"◘  data   \tavg=2.000 min=1.000 max=3.000 count=2 (+1 non-finite)");
}

TEST(Text, legend_details_all_non_finite)
{
    const double ys[] = { NAN, INFINITY, -INFINITY };
    EXPECT_EQ(details_line(BLOT_SCATTER, 3, NF_X, ys),
              L"◘  data   \tavg=- min=- max=- count=0 (+3 non-finite)");
}

TEST(Text, legend_details_sum_overflows)
{
    /* each value is finite, their sum is not */
    const double big[] = { 1e308, 1e308, 1e308 };
    EXPECT_EQ(details_line(BLOT_SCATTER, 3, NF_X, big),
              L"◘  data   \tavg=1e+308 min=1e+308 max=1e+308 count=3");

    const double mixed[] = { 1.5e308, 1.5e308, 0 };
    EXPECT_EQ(details_line(BLOT_SCATTER, 3, NF_X, mixed),
              L"◘  data   \tavg=1e+308 min=0.000 max=1.5e+308 count=3");

    /* the running sum overflows, then comes back: the mean is exactly 0 */
    const double sym[] = { DBL_MAX, DBL_MAX, -DBL_MAX, -DBL_MAX };
    const double sym_x[] = { 0, 1, 2, 3 };
    EXPECT_EQ(details_line(BLOT_SCATTER, 4, sym_x, sym),
              L"◘  data   \tavg=0.000 min=-1.79769e+308 max=1.79769e+308 count=4");
}

TEST(Text, legend_details_finite_unchanged)
{
    const double ys[] = { 1, 2, 4 };
    EXPECT_EQ(details_line(BLOT_SCATTER, 3, NF_X, ys),
              L"◘  data   \tavg=2.333 min=1.000 max=4.000 count=3");
}

/* #22 (review note 35657): the mean is of a compensated sum, so a small
 * value next to large ones isn't lost; a plain sum gives avg=0.000 */
TEST(Text, legend_details_compensated_sum)
{
    const double ys[] = { 1e17, 1, -1e17 };
    EXPECT_EQ(details_line(BLOT_SCATTER, 3, NF_X, ys),
              L"◘  data   \tavg=0.333 min=-100000000000000000.000 max=100000000000000000.000 count=3");

    /* the same past the overflow, on the scaled sum */
    const double big[] = { 1e308, 1, 1e308, -1e308, -1e308 };
    const double big_x[] = { 0, 1, 2, 3, 4 };
    EXPECT_EQ(details_line(BLOT_SCATTER, 5, big_x, big),
              L"◘  data   \tavg=0.200 min=-1e+308 max=1e+308 count=5");
}

TEST(Text, legend_details_compensated_long_series)
{
    /* 3000 points: 1000 times 1e16, 1, -1e16; a plain sum loses
     * every 1 (ulp(1e16) is 2), the true mean is 1/3 */
    std::vector<double> xs, ys;
    for (int i = 0; i < 1000; i++) {
        for (double y : { 1e16, 1.0, -1e16 }) {
            xs.push_back(xs.size());
            ys.push_back(y);
        }
    }
    EXPECT_EQ(details_line(BLOT_SCATTER, ys.size(), xs.data(), ys.data()),
              L"◘  data   \tavg=0.333 min=-10000000000000000.000 max=10000000000000000.000 count=3000");

    /* 10000 points: 2500 times 1e16, 0.5, -1e16, 0.5; a plain sum
     * loses the 0.5 after each 1e16, the true mean is 0.25 */
    xs.clear();
    ys.clear();
    for (int i = 0; i < 2500; i++) {
        for (double y : { 1e16, 0.5, -1e16, 0.5 }) {
            xs.push_back(xs.size());
            ys.push_back(y);
        }
    }
    EXPECT_EQ(details_line(BLOT_SCATTER, ys.size(), xs.data(), ys.data()),
              L"◘  data   \tavg=0.250 min=-10000000000000000.000 max=10000000000000000.000 count=10000");
}

/* #29: a legend line too wide for the screen was cut at any character,
 * so a value could be cut too: max=1.5e+308 read max=1.5e+3, and the
 * "(+N non-finite)" after count=N could go without a sign.  The line is
 * now cut only between items: the symbol and the label, then whole
 * "key=value" items, count=N and its suffix as one.  A label too long
 * for the screen is cut and ends in '~' */
namespace {

/* the legend line of one layer, Y values ys at X 0, 1, ..., on a screen
 * of cols x 12 without axes */
std::wstring cut_line(const char *label, std::vector<double> ys, unsigned cols,
                      blot_render_flags flags = F | BLOT_RENDER_LEGEND_DETAILS)
{
    std::vector<double> xs;
    for (size_t i = 0; i < ys.size(); i++)
        xs.push_back(i);
    Fig f(cols, 12);
    f.lim(0, 9, 0, 9);
    f.plot(BLOT_SCATTER, BLOT_DATA_DOUBLE, ys.size(), xs.data(), ys.data(), 1, label);
    Rendered r = render(f, flags | BLOT_RENDER_NO_X_AXIS | BLOT_RENDER_NO_Y_AXIS | BLOT_RENDER_LEGEND_ABOVE);
    expect_lines_fit(r, cols, 12);
    if (r.lines.empty())
        return L"";
    return r.lines.front();
}

/* the items after the tab: "key=value", with a "(+N non-finite)" kept
 * on the count before it */
std::vector<std::wstring> detail_items(const std::wstring &line)
{
    std::vector<std::wstring> items;
    size_t tab = line.find(L'\t');
    if (tab == std::wstring::npos)
        return items;
    std::wstring d = line.substr(tab + 1), tok;
    size_t start = 0;
    while (start <= d.size()) {
        size_t sp = d.find(L' ', start);
        tok = d.substr(start, sp == std::wstring::npos ? std::wstring::npos : sp - start);
        bool open = !items.empty() && items.back().find(L'(') != std::wstring::npos
            && items.back().back() != L')';
        if (!items.empty() && (open || tok[0] == L'('))
            items.back() += L" " + tok;
        else
            items.push_back(tok);
        if (sp == std::wstring::npos)
            break;
        start = sp + 1;
    }
    return items;
}

} // namespace

TEST(Text, legend_cut_between_items)
{
    const std::vector<double> ys = { 1, 2, 4 };
    /* "◘  data   " is 10 columns, the tab goes to 16, then avg ends in
     * 25, min in 35, max in 45, count in 53 */
    EXPECT_EQ(cut_line("data", ys, 53), L"◘  data   \tavg=2.333 min=1.000 max=4.000 count=3");
    EXPECT_EQ(cut_line("data", ys, 52), L"◘  data   \tavg=2.333 min=1.000 max=4.000");
    EXPECT_EQ(cut_line("data", ys, 45), L"◘  data   \tavg=2.333 min=1.000 max=4.000");
    EXPECT_EQ(cut_line("data", ys, 44), L"◘  data   \tavg=2.333 min=1.000");
    EXPECT_EQ(cut_line("data", ys, 26), L"◘  data   \tavg=2.333");
    EXPECT_EQ(cut_line("data", ys, 25), L"◘  data   \tavg=2.333");
    EXPECT_EQ(cut_line("data", ys, 24), L"◘  data");
    EXPECT_EQ(cut_line("data", ys, 20), L"◘  data");     /* the minimum width */
}

TEST(Text, legend_cut_never_splits_a_number)
{
    /* avg ends in 26, min in 36, max in 49, count in 57: 47 columns
     * printed "max=1.5e+3", which reads as 1500 */
    const std::vector<double> big = { 1.5e308, 1.5e308, 0 };
    EXPECT_EQ(cut_line("data", big, 57), L"◘  data   \tavg=1e+308 min=0.000 max=1.5e+308 count=3");
    EXPECT_EQ(cut_line("data", big, 49), L"◘  data   \tavg=1e+308 min=0.000 max=1.5e+308");
    EXPECT_EQ(cut_line("data", big, 48), L"◘  data   \tavg=1e+308 min=0.000");
    EXPECT_EQ(cut_line("data", big, 47), L"◘  data   \tavg=1e+308 min=0.000");

    /* avg ends in 25, min in 43: 32 columns printed "min=-1" */
    const std::vector<double> sym = { DBL_MAX, DBL_MAX, -DBL_MAX, -DBL_MAX };
    EXPECT_EQ(cut_line("data", sym, 32), L"◘  data   \tavg=0.000");
    EXPECT_EQ(cut_line("data", sym, 42), L"◘  data   \tavg=0.000");
    EXPECT_EQ(cut_line("data", sym, 43), L"◘  data   \tavg=0.000 min=-1.79769e+308");
}

TEST(Text, legend_cut_drops_suffix_with_count)
{
    /* max ends in 45, " count=2 (+1 non-finite)" in 69: 53 columns
     * printed "count=2" without its suffix */
    const std::vector<double> ys = { 1, NAN, 3 };
    EXPECT_EQ(cut_line("data", ys, 69), L"◘  data   \tavg=2.000 min=1.000 max=3.000 count=2 (+1 non-finite)");
    EXPECT_EQ(cut_line("data", ys, 68), L"◘  data   \tavg=2.000 min=1.000 max=3.000");
    EXPECT_EQ(cut_line("data", ys, 53), L"◘  data   \tavg=2.000 min=1.000 max=3.000");
}

TEST(Text, legend_long_label_cut_with_mark)
{
    const std::vector<double> ys = { 1, 2, 4 };
    const char *label = "a much longer label for the legend";    /* 34: ends in 37 */
    for (auto flags : { F, F | BLOT_RENDER_LEGEND_DETAILS }) {
        SCOPED_TRACE(flags);
        /* the minimum width: 16 characters and the '~' */
        EXPECT_EQ(cut_line(label, ys, 20, flags), L"◘  a much longer la~");
        EXPECT_EQ(cut_line(label, ys, 30, flags), L"◘  a much longer label for th~");
        EXPECT_EQ(cut_line(label, ys, 36, flags), L"◘  a much longer label for the lege~");
        /* the label fits exactly: not cut, no '~' */
        EXPECT_EQ(cut_line(label, ys, 37, flags), L"◘  a much longer label for the legend");
    }
    /* "   " goes to 40, the tab to 48, avg ends in 57 */
    EXPECT_EQ(cut_line(label, ys, 56), L"◘  a much longer label for the legend");
    EXPECT_EQ(cut_line(label, ys, 57), L"◘  a much longer label for the legend   \tavg=2.333");
}

TEST(Text, legend_cut_with_colour)
{
    /* the escapes take no column: the same cuts as without colour */
    const std::vector<double> ys = { 1, 2, 4 };
    const blot_render_flags colour = BLOT_RENDER_NO_UNICODE | BLOT_RENDER_LEGEND_DETAILS;
    EXPECT_EQ(cut_line("data", ys, 36, colour), sgr(L"\\e[38;5;1m◘ \\e[0m data   \tavg=2.333 min=1.000"));
    EXPECT_EQ(cut_line("data", ys, 24, colour), sgr(L"\\e[38;5;1m◘ \\e[0m data"));
    EXPECT_EQ(cut_line("a much longer label for the legend", ys, 20, colour),
              sgr(L"\\e[38;5;1m◘ \\e[0m a much longer la~"));
}

TEST(Text, legend_cut_with_tabs)
{
    const std::vector<double> ys = { 1, 2, 4 };
    /* "◘  a" is 4 columns, its tab goes to 8, "b   " to 12, the next tab
     * to 16; avg ends in 25, min in 35 */
    EXPECT_EQ(cut_line("a\tb", ys, 34), L"◘  a\tb   \tavg=2.333");
    EXPECT_EQ(cut_line("a\tb", ys, 35), L"◘  a\tb   \tavg=2.333 min=1.000");
    /* a label cut after its tab: "◘  ab" is 5, the tab goes to 8, then
     * 11 characters to 19 and the '~' */
    EXPECT_EQ(cut_line("ab\tcdefghijklmnopqrstu", ys, 20), L"◘  ab\tcdefghijklm~");
    EXPECT_EQ(cut_line("ab\tcdefghijklmnopqrstu", ys, 22), L"◘  ab\tcdefghijklmno~");
    /* "◘  abcdefghijklmn" is 17, its tab goes to 24, the z ends in 25: it
     * fits in 25; in 24 the tab doesn't fit with the '~' and the label is
     * cut before it */
    EXPECT_EQ(cut_line("abcdefghijklmn\tz", ys, 25), L"◘  abcdefghijklmn\tz");
    EXPECT_EQ(cut_line("abcdefghijklmn\tz", ys, 24), L"◘  abcdefghijklmn~");
}

/* a combining mark stays with its base character at a '~' cut */
TEST(Text, legend_utf8_combining_mark_at_cut)
{
    RUN_ISOLATED(10, {
        SET_UTF8_LOCALE();
        /* 18 x "e" + U+0301 take 18 columns, 21 with "◘  ": at the
         * minimum width 16 pairs and the '~' are kept */
        std::string label;
        std::wstring expected = L"◘  ";
        for (int i = 0; i < 18; i++)
            label += "e\xcc\x81";
        for (int i = 0; i < 16; i++)
            expected += L"e\u0301";
        expected += L"~";
        std::wstring l = utf8_legend_line(label.c_str(), 20);
        EXPECT_EQ(l, expected) << to_utf8(l);
        EXPECT_EQ(display_width_locale(l), 20u);
    });
}

/* at every width, the line is the label and as many whole items as fit,
 * and each visible value is the full value */
TEST(Text, legend_cut_values_are_whole)
{
    const std::vector<std::vector<double>> series = {
        { 1, 2, 4 },
        { 1.5e308, 1.5e308, 0 },
        { DBL_MAX, DBL_MAX, -DBL_MAX, -DBL_MAX },
        { 1, NAN, 3 },
        { NAN, INFINITY, -INFINITY },
        { 1e17, 1, -1e17 },
    };
    for (const auto &ys : series) {
        const std::wstring full = cut_line("data", ys, 200);
        const std::vector<std::wstring> items = detail_items(full);
        SCOPED_TRACE(to_utf8(full));
        ASSERT_EQ(items.size(), 4u);
        EXPECT_EQ(full, L"◘  data   \t" + items[0] + L" " + items[1] + L" " + items[2] + L" " + items[3]);

        for (unsigned cols = BLOT_MIN_COLS; cols <= display_width(full) + 2; cols++) {
            SCOPED_TRACE(cols);
            const std::wstring l = cut_line("data", ys, cols);

            /* the longest line of whole items that fits */
            std::wstring expected = L"◘  data";
            for (size_t k = 0; k < items.size(); k++) {
                std::wstring next = expected + (k ? L" " : L"   \t") + items[k];
                if (display_width(next) > cols)
                    break;
                expected = next;
            }
            EXPECT_EQ(l, expected) << to_utf8(l);

            /* each visible key=value is that of the full line */
            std::vector<std::wstring> seen = detail_items(l);
            ASSERT_LE(seen.size(), items.size());
            for (size_t k = 0; k < seen.size(); k++) {
                EXPECT_EQ(seen[k], items[k]);
                size_t eq = seen[k].find(L'=');
                ASSERT_NE(eq, std::wstring::npos) << to_utf8(seen[k]);
                std::string v = to_utf8(seen[k].substr(eq + 1)), fv = to_utf8(items[k].substr(items[k].find(L'=') + 1));
                if (v == "-" || seen[k].compare(0, 6, L"count=") == 0) {
                    EXPECT_EQ(v, fv);
                    continue;
                }
                char *end = NULL;
                double d = strtod(v.c_str(), &end);
                EXPECT_EQ(*end, '\0') << v;
                EXPECT_EQ(d, strtod(fv.c_str(), NULL)) << v << " vs " << fv;
            }
        }
    }
}
