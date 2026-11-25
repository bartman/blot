/* whole-figure goldens (#2, group G): every test compares the complete,
 * right-trimmed output of blot_figure_render() */
#include "blot_test.hpp"

using namespace blot_test;

namespace {

void expect_golden(blot_figure *fig, blot_render_flags flags, const Lines &golden)
{
    Rendered r = render(fig, flags);
    ASSERT_TRUE(r.ok) << r.message;
    EXPECT_LINES_EQ(rtrim(r.lines), golden);
}

} // namespace

TEST(Golden, scatter_with_axes)
{
    Fig f;
    fix_a_scatter(f);
    expect_golden(f, F, G1);
}

TEST(Golden, scatter_dont_invert_y)
{
    Fig f;
    fix_a_scatter(f);
    expect_golden(f, F | BLOT_RENDER_DONT_INVERT_Y_AXIS, {
        L" 0.000 **",
        L"       |",
        L"       |     *",
        L" 3.000 *",
        L"       |",
        L"       |          *",
        L" 6.000 *",
        L"       |",
        L"       |",
        L"       |                   *",
        L"       +*-----*------*------",
        L"        0.000 6.333  12.67",
    });
}

TEST(Golden, scatter_no_x_axis)
{
    Fig f;
    fix_a_scatter(f);
    expect_golden(f, F | BLOT_RENDER_NO_X_AXIS, {
        L"       |                   *",
        L"       |",
        L" 7.200 *",
        L"       |",
        L" 5.400 *",
        L"       |          *",
        L"       |",
        L" 3.600 *",
        L"       |",
        L" 1.800 *     *",
        L"       |",
        L" 0.000 **",
    });
}

TEST(Golden, scatter_no_y_axis)
{
    Fig f;
    fix_a_scatter(f);
    expect_golden(f, F | BLOT_RENDER_NO_Y_AXIS, {
        L"                           *",
        L"",
        L"",
        L"",
        L"              *",
        L"",
        L"",
        L"       *",
        L"",
        L"*",
        L"*--------*--------*---------",
        L"0.000    6.333    12.667",
    });
}

TEST(Golden, scatter_unicode_bullet)
{
    /* G1 with every data mark a bullet; the axis characters stay ASCII */
    Lines golden = G1;
    for (size_t i = 0; i < 10; i++)
        for (size_t c = 8; c < golden[i].size(); c++)
            if (golden[i][c] == L'*')
                golden[i][c] = L'•';

    Fig f;
    fix_a_scatter(f);
    expect_golden(f, BLOT_RENDER_NO_COLOR, golden);
}

TEST(Golden, line_with_axes)
{
    const gint32 xs[] = { 0, 19 }, ys[] = { 0, 9 };
    Fig f;
    fix_a(f);
    f.plot(BLOT_LINE, BLOT_DATA_INT32, 2, xs, ys);
    expect_golden(f, F, {
        L"       |                   *",
        L"       |                 **",
        L"       |               **",
        L" 6.000 *             **",
        L"       |           **",
        L"       |         **",
        L" 3.000 *       **",
        L"       |     **",
        L"       |   **",
        L" 0.000 ****",
        L"       +*-----*------*------",
        L"        0.000 6.333  12.67",
    });
}

TEST(Golden, bar_with_axes)
{
    const gint32 xs[] = { 2, 8, 14 }, ys[] = { 3, 9, 5 };
    Fig f;
    fix_a(f);
    f.plot(BLOT_BAR, BLOT_DATA_INT32, 3, xs, ys);
    expect_golden(f, F, {
        L"       |        **",
        L"       |        **",
        L"       |        **",
        L" 6.000 *        **",
        L"       |        **    **",
        L"       |        **    **",
        L" 3.000 *  **    **    **",
        L"       |  **    **    **",
        L"       |  **    **    **",
        L" 0.000 *  **    **    **",
        L"       +*-----*------*------",
        L"        0.000 6.333  12.67",
    });
}

/* G8..G11, braille: screen 29x12, limits 0..39, the plot is 20x10 cells =
 * 40x40 dots, one dot per integer (mrg.left 9) */
namespace {
void fix_braille(Fig &f)
{
    f.size(29, 12);
    f.lim(0, 39, 0, 39);
}
const blot_render_flags BRL = BLOT_RENDER_NO_COLOR | BLOT_RENDER_BRAILLE;
const gint32 BS_X[] = { 0, 39, 20, 1, 0 };
const gint32 BS_Y[] = { 0, 39, 20, 0, 1 };
} // namespace

TEST(Golden, braille_scatter)
{
    Fig f;
    fix_braille(f);
    f.plot(BLOT_SCATTER, BLOT_DATA_INT32, 5, BS_X, BS_Y);
    expect_golden(f, BRL, {
        L"        |                   ⠈",
        L"        |",
        L"        |",
        L" 26.000 *",
        L"        |          ⡀",
        L"        |",
        L" 13.000 *",
        L"        |",
        L"        |",
        L"  0.000 *⣄",
        L"        +*-----*------*------",
        L"         0.000 13.00  26.00",
    });
}

TEST(Golden, braille_scatter_dont_invert)
{
    Fig f;
    fix_braille(f);
    f.plot(BLOT_SCATTER, BLOT_DATA_INT32, 5, BS_X, BS_Y);
    expect_golden(f, BRL | BLOT_RENDER_DONT_INVERT_Y_AXIS, {
        L"  0.000 *⠋",
        L"        |",
        L"        |",
        L" 13.000 *",
        L"        |",
        L"        |          ⠁",
        L" 26.000 *",
        L"        |",
        L"        |",
        L"        |                   ⢀",
        L"        +*-----*------*------",
        L"         0.000 13.00  26.00",
    });
}

TEST(Golden, braille_line)
{
    const gint32 xs[] = { 0, 39 }, ys[] = { 0, 39 };
    Fig f;
    fix_braille(f);
    f.plot(BLOT_LINE, BLOT_DATA_INT32, 2, xs, ys);
    expect_golden(f, BRL, {
        L"        |                  ⡠⠊",
        L"        |                ⡠⠊",
        L"        |              ⡠⠊",
        L" 26.000 *            ⡠⠊",
        L"        |          ⡠⠊",
        L"        |        ⡠⠊",
        L" 13.000 *      ⡠⠊",
        L"        |    ⡠⠊",
        L"        |  ⡠⠊",
        L"  0.000 *⡠⠊",
        L"        +*-----*------*------",
        L"         0.000 13.00  26.00",
    });
}

TEST(Golden, braille_bar)
{
    const gint32 xs[] = { 4, 20 }, ys[] = { 39, 7 };
    Fig f;
    fix_braille(f);
    f.plot(BLOT_BAR, BLOT_DATA_INT32, 2, xs, ys);
    expect_golden(f, BRL, {
        L"        |  ⣿",
        L"        |  ⣿",
        L"        |  ⣿",
        L" 26.000 *  ⣿",
        L"        |  ⣿",
        L"        |  ⣿",
        L" 13.000 *  ⣿",
        L"        |  ⣿",
        L"        |  ⣿       ⣿",
        L"  0.000 *  ⣿       ⣿",
        L"        +*-----*------*------",
        L"         0.000 13.00  26.00",
    });
}

TEST(Golden, no_axes_three_plot_types)
{
    const gint32 xs[] = { 0, 5, 9 }, ys[] = { 0, 5, 9 };
    const blot_render_flags flags = F | BLOT_RENDER_NO_X_AXIS | BLOT_RENDER_NO_Y_AXIS;
    const struct {
        blot_plot_type type;
        Lines golden;
    } cases[] = {
        { BLOT_SCATTER, {
            L"         *",
            L"",
            L"",
            L"",
            L"     *",
            L"",
            L"",
            L"",
            L"",
            L"*",
        } },
        { BLOT_LINE, {
            L"         *",
            L"        *",
            L"       *",
            L"      *",
            L"     *",
            L"    *",
            L"   *",
            L"  *",
            L" *",
            L"*",
        } },
        { BLOT_BAR, {
            L"         *",
            L"         *",
            L"         *",
            L"         *",
            L"     **  *",
            L"     **  *",
            L"     **  *",
            L"     **  *",
            L"     **  *",
            L"**   **  *",
        } },
    };
    for (const auto &c : cases) {
        SCOPED_TRACE(c.type);
        Fig f(10, 10);
        f.lim(0, 9, 0, 9);
        f.plot(c.type, BLOT_DATA_INT32, 3, xs, ys);
        expect_golden(f, flags, c.golden);
    }
}

/* G13..G20, G22 have sloped lines: their interior cells depend on how
 * blot_canvas_draw_line() rounds; regenerate them (and check by eye) if
 * that changes (B4, B5 in #4).  With auto limits they also pin where a line
 * maps y (round(v * (rows-1) / range), Edge.DISABLED_point_on_tick_value_
 * lands_on_tick_line) */

TEST(Golden, auto_limits_line)
{
    /* pins the 0.1% padding: the limits become -0.01 .. 10.01 */
    const gint32 xs[] = { 0, 5, 10 }, ys[] = { 0, 5, 10 };
    Fig f(40, 16);
    f.plot(BLOT_LINE, BLOT_DATA_INT32, 3, xs, ys);
    expect_golden(f, F, {
        L"        |                              *",
        L"        |                            **",
        L"  8.016 *                         ***",
        L"        |                       **",
        L"        |                    ***",
        L"  6.012 *                  **",
        L"        |               ***",
        L"        |             **",
        L"  4.008 *           **",
        L"        |         **",
        L"        |       **",
        L"  2.004 *     **",
        L"        |   **",
        L"  0.000 ****",
        L"        +*---------*---------*----------",
        L"         0.000     3.340     6.680",
    });
}

TEST(Golden, auto_limits_offset_range)
{
    /* 9.99 .. 20.01, no zero alignment */
    const double xs[] = { 10, 20 }, ys[] = { 10, 20 };
    Fig f(40, 16);
    f.plot(BLOT_LINE, BLOT_DATA_DOUBLE, 2, xs, ys);
    expect_golden(f, F, {
        L"        |                              *",
        L"        |                            **",
        L" 18.006 *                          **",
        L"        |                        **",
        L"        |                     ***",
        L" 16.002 *                   **",
        L"        |                 **",
        L"        |              ***",
        L" 13.998 *            **",
        L"        |          **",
        L"        |       ***",
        L" 11.994 *     **",
        L"        |   **",
        L"  9.990 ****",
        L"        +*---------*---------*----------",
        L"         9.990     13.330    16.670",
    });
}

TEST(Golden, zero_crossing_both_axes)
{
    /* zero alignment: a tick labelled exactly 0.000 on each axis.  The
     * empty top row is B5 in #4 (the line misses its last cell). */
    const double xs[] = { -0.3, 0.7 }, ys[] = { -0.3, 0.7 };
    Fig f(60, 20);
    f.plot(BLOT_LINE, BLOT_DATA_DOUBLE, 2, xs, ys);
    expect_golden(f, F, {
        L"        |",
        L"        |                                                ***",
        L"  0.573 *                                             ***",
        L"        |                                          ***",
        L"  0.429 *                                       ***",
        L"        |                                    ***",
        L"        |                                 ***",
        L"  0.286 *                              ***",
        L"        |                           ***",
        L"        |                        ***",
        L"  0.143 *                     ***",
        L"        |                  ***",
        L"  0.000 *               ***",
        L"        |            ***",
        L"        |         ***",
        L" -0.143 *      ***",
        L"        |   ***",
        L" -0.286 ****",
        L"        +-----*---------*---------*---------*---------*-----",
        L"              -0.200    0.000     0.200     0.401     0.601",
    });
}

TEST(Golden, all_zero_data)
{
    /* regression for !1: Y becomes -0.1 .. 0.1 */
    const gint32 xs[] = { 1, 2, 3 }, ys[] = { 0, 0, 0 };
    Fig f(40, 14);
    f.plot(BLOT_LINE, BLOT_DATA_INT32, 3, xs, ys);
    expect_golden(f, F, {
        L"        |",
        L"  0.080 *",
        L"        |",
        L"  0.040 *",
        L"        |",
        L"  0.000 ********************************",
        L"        |",
        L"        |",
        L" -0.040 *",
        L"        |",
        L" -0.080 *",
        L"        |",
        L"        +*---------*---------*----------",
        L"         0.998     1.666     2.334",
    });
}

TEST(Golden, negative_large_y)
{
    /* margin 12; the empty top row is B5 in #4 */
    const double xs[] = { 0, 1 }, ys[] = { -1000, 100 };
    Fig f(40, 14);
    f.plot(BLOT_LINE, BLOT_DATA_DOUBLE, 2, xs, ys);
    expect_golden(f, F, {
        L"           |",
        L"     0.000 *                         ***",
        L"           |                       **",
        L"  -220.440 *                    ***",
        L"           |                  **",
        L"  -440.880 *               ***",
        L"           |             **",
        L"           |          ***",
        L"  -661.320 *        **",
        L"           |     ***",
        L"  -881.760 *   **",
        L"           |***",
        L"           +*--------*--------*---------",
        L"            0.000    0.334    0.668",
    });
}

TEST(Golden, small_fraction_y)
{
    const double xs[] = { 0, 1 }, ys[] = { 0.05, -0.05 };
    Fig f(40, 14);
    f.plot(BLOT_LINE, BLOT_DATA_DOUBLE, 2, xs, ys);
    expect_golden(f, F, {
        L"        |*",
        L"  0.040 * **",
        L"        |   ***",
        L"  0.020 *      ***",
        L"        |         **",
        L"  0.000 *           ***",
        L"        |              ***",
        L"        |                 ***",
        L" -0.020 *                    **",
        L"        |                      ***",
        L" -0.040 *                         ***",
        L"        |                            ***",
        L"        +*---------*---------*----------",
        L"         0.000     0.334     0.668",
    });
}

TEST(Golden, narrow_y_range_high_offset)
{
    const double xs[] = { 0, 1 }, ys[] = { 999.9, 1000 };
    Fig f(40, 14);
    f.plot(BLOT_LINE, BLOT_DATA_DOUBLE, 2, xs, ys);
    expect_golden(f, F, {
        L"          |                            *",
        L"          |                          **",
        L"  999.980 *                       ***",
        L"          |                     **",
        L"  999.960 *                  ***",
        L"          |                **",
        L"          |             ***",
        L"  999.940 *           **",
        L"          |        ***",
        L"  999.920 *      **",
        L"          |   ***",
        L"  999.900 ****",
        L"          +*--------*---------*---------",
        L"           0.000    0.334     0.668",
    });
}

TEST(Golden, zero_width_x_range)
{
    /* X becomes 3.6 .. 4.4 (10% either side): a vertical line in the
     * centre column */
    const double xs[] = { 4, 4, 4 }, ys[] = { 1, 2, 3 };
    Fig f(40, 14);
    f.plot(BLOT_LINE, BLOT_DATA_DOUBLE, 3, xs, ys);
    expect_golden(f, F, {
        L"       |               *",
        L"       |               *",
        L" 2.601 *               *",
        L"       |               *",
        L" 2.200 *               *",
        L"       |               *",
        L"       |               *",
        L" 1.800 *               *",
        L"       |               *",
        L" 1.399 *               *",
        L"       |               *",
        L" 0.998 *               *",
        L"       +*---------*----------*----------",
        L"        3.600     3.867      4.133",
    });
}

TEST(Golden, bar_with_custom_x_labels)
{
    /* mapping-neutral: limits 0..cells-1 with integer data, one cell per
     * integer; each label sits under the left edge of its bar */
    const gint32 xs[] = { 0, 8, 16, 24 }, ys[] = { 6, 2, 9, 2 };
    char *labels[] = { (char *)"a", (char *)"b", (char *)"c", (char *)"d" };
    Fig f(41, 14);
    f.lim(0, 31, 0, 11);
    ASSERT_TRUE(blot_figure_set_x_axis_labels(f, 4, labels, NULL));
    f.plot(BLOT_BAR, BLOT_DATA_INT32, 4, xs, ys);
    expect_golden(f, F, {
        L"        |",
        L"        |",
        L"  8.800 *                **",
        L"        |                **",
        L"  6.600 *                **",
        L"        |**              **",
        L"        |**              **",
        L"  4.400 ***              **",
        L"        |**              **",
        L"  2.200 ***      **      **      **",
        L"        |**      **      **      **",
        L"  0.000 ***      **      **      **",
        L"        +*-------*-------*-------*-------",
        L"         a       b       c       d",
    });
}

// BUG B1: https://git.jukie.net/bart/blot/-/work_items/3
TEST(Golden, DISABLED_bar_with_custom_x_labels_auto_limits)
{
    /* auto limits (Y 0.997..4.003): bar i sits under label i at full slot
     * width, as on master; its top follows the Y axis mapping, so it
     * agrees with the Y ticks: 3 tops out on the 2.801 row (that cell
     * covers 2.751..3.002) and 4 on the top row.  Master draws them two
     * rows lower (the range+1 mapping).  X placement on a numeric axis is
     * not pinned here (#12). */
    const gint32 ys[] = { 3, 1, 4, 1 };
    char *labels[] = { (char *)"a", (char *)"b", (char *)"c", (char *)"d" };
    Fig f(40, 14);
    ASSERT_TRUE(blot_figure_set_x_axis_labels(f, 4, labels, NULL));
    f.plot(BLOT_BAR, BLOT_DATA_INT32, 4, NULL, ys);
    expect_golden(f, F, {
        L"       |                *****",
        L"       |                *****",
        L" 3.402 *                *****",
        L"       |                *****",
        L" 2.801 ******           *****",
        L"       |*****           *****",
        L"       |*****           *****",
        L" 2.199 ******           *****",
        L"       |*****           *****",
        L" 1.598 ******           *****",
        L"       |*****           *****",
        L" 0.997 ******   *****   *****   *****",
        L"       +*-------*-------*-------*-------",
        L"        a       b       c       d",
    });
}

TEST(Golden, union_of_two_layers)
{
    /* the limits are the union of both layers */
    const gint32 ax[] = { 0, 5 }, ay[] = { 0, 1 };
    const gint32 bx[] = { 10, 20 }, by[] = { -5, -1 };
    Fig f(40, 14);
    f.plot(BLOT_LINE, BLOT_DATA_INT32, 2, ax, ay, 1);
    f.plot(BLOT_LINE, BLOT_DATA_INT32, 2, bx, by, 2);
    expect_golden(f, F, {
        L"        |        *",
        L"        |    ****",
        L"  0.000 *****",
        L"        |",
        L" -1.202 *                              *",
        L"        |                            **",
        L" -2.405 *                          **",
        L"        |                        **",
        L"        |                      **",
        L" -3.607 *                    **",
        L"        |                  **",
        L" -4.810 *               ***",
        L"        +*---------*---------*----------",
        L"         0.000     6.680     13.360",
    });
}
