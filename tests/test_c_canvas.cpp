#include <gtest/gtest.h>

#include "blot_canvas.h"
#include "blot_error.h"

TEST(Canvas, alloc_new_delete)
{
    GError *error = NULL;
    blot_canvas *canvas = blot_canvas_new(10, 10, BLOT_RENDER_NONE, 1, &error);
    ASSERT_TRUE(canvas != NULL);
    ASSERT_TRUE(error == NULL);
    blot_canvas_delete(canvas);
}

TEST(Canvas, new_invalid_dimensions)
{
    GError *error = NULL;
    blot_canvas *canvas = blot_canvas_new(0, 10, BLOT_RENDER_NONE, 1, &error);
    ASSERT_TRUE(canvas == NULL);
    ASSERT_TRUE(error != NULL);
    g_clear_error(&error);

    error = NULL;
    canvas = blot_canvas_new(10, 0, BLOT_RENDER_NONE, 1, &error);
    ASSERT_TRUE(canvas == NULL);
    ASSERT_TRUE(error != NULL);
    g_clear_error(&error);
}

TEST(Canvas, draw_point_and_get_cell)
{
    GError *error = NULL;
    blot_canvas *canvas = blot_canvas_new(10, 10, BLOT_RENDER_NONE, 1, &error);
    ASSERT_TRUE(canvas != NULL);

    // Draw a point at (5, 5)
    blot_canvas_draw_point(canvas, 5, 5);

    // Check if the cell at (5, 5) is set
    ASSERT_EQ(blot_canvas_get_cell(canvas, 5, 5), L'•'); // Bullet character

    // Check a cell that should be empty
    ASSERT_EQ(blot_canvas_get_cell(canvas, 0, 0), L'\0');

    blot_canvas_delete(canvas);
}

TEST(Canvas, draw_point_out_of_bounds)
{
    GError *error = NULL;
    blot_canvas *canvas = blot_canvas_new(10, 10, BLOT_RENDER_NONE, 1, &error);
    ASSERT_TRUE(canvas != NULL);

    // Drawing out of bounds should not cause a crash or change other cells
    blot_canvas_draw_point(canvas, -1, -1);
    blot_canvas_draw_point(canvas, 10, 10);

    // Check a cell that should be empty
    ASSERT_EQ(blot_canvas_get_cell(canvas, 0, 0), L'\0');

    blot_canvas_delete(canvas);
}

TEST(Canvas, draw_line)
{
    GError *error = NULL;
    blot_canvas *canvas = blot_canvas_new(10, 10, BLOT_RENDER_NONE, 1, &error);
    ASSERT_TRUE(canvas != NULL);

    // Draw a horizontal line
    blot_canvas_draw_line(canvas, 0, 0, 9, 0);
    for (int i = 0; i < 10; ++i) {
        ASSERT_EQ(blot_canvas_get_cell(canvas, i, 0), L'•');
    }

    // Draw a vertical line
    blot_canvas_draw_line(canvas, 0, 0, 0, 9);
    for (int i = 0; i < 10; ++i) {
        ASSERT_EQ(blot_canvas_get_cell(canvas, 0, i), L'•');
    }

    // Draw a diagonal line (simple check)
    blot_canvas_draw_line(canvas, 0, 0, 9, 9);
    ASSERT_EQ(blot_canvas_get_cell(canvas, 0, 0), L'•');
    ASSERT_EQ(blot_canvas_get_cell(canvas, 5, 5), L'•'); // Bullet character
    ASSERT_EQ(blot_canvas_get_cell(canvas, 9, 9), L'•');

    blot_canvas_delete(canvas);
}

TEST(Canvas, fill_rect)
{
    GError *error = NULL;
    blot_canvas *canvas = blot_canvas_new(10, 10, BLOT_RENDER_NONE, 1, &error);
    ASSERT_TRUE(canvas != NULL);

    // Fill a rectangle
    blot_canvas_fill_rect(canvas, 2, 2, 7, 7);

    for (int y = 0; y < 10; ++y) {
        for (int x = 0; x < 10; ++x) {
            if (x >= 2 && x <= 7 && y >= 2 && y <= 7) {
                ASSERT_EQ(blot_canvas_get_cell(canvas, x, y), L'•');
            } else {
                ASSERT_EQ(blot_canvas_get_cell(canvas, x, y), L'\0');
            }
        }
    }

    blot_canvas_delete(canvas);
}

/* --- canvas primitives: exact cells (#2, group C) ----------------------
 *
 * Every test compares the exact set of cells, so stray cells are caught
 * as well as missing ones. */

#include <cfloat>
#include <climits>
#include <iomanip>
#include <random>

#include "blot_test.hpp"

using namespace blot_test;

namespace {

Cells line_cells(unsigned w, unsigned h, double x0, double y0, double x1, double y1)
{
    Canvas can(w, h);
    blot_canvas_draw_line(can, x0, y0, x1, y1);
    return cells(can);
}

Cells row_cells(unsigned row, unsigned c0, unsigned c1)
{
    Cells out;
    for (unsigned c = c0; c <= c1; c++)
        out.insert({ c, row });
    return out;
}

Cells col_cells(unsigned col, unsigned r0, unsigned r1)
{
    Cells out;
    for (unsigned r = r0; r <= r1; r++)
        out.insert({ col, r });
    return out;
}

/* C10, C11: from (12,12) on a 25x25 canvas */
const int DELTAS[][2] = {
    { 10, 1 }, { 10, 3 }, { 3, 10 }, { 1, 10 }, { -10, 3 }, { 10, -3 }, { -3, -10 }, { -10, -1 },
};

} // namespace

TEST(Canvas, every_cell_roundtrip)
{
    Canvas can(13, 7);
    ASSERT_TRUE(can.can);
    for (unsigned r = 0; r < 7; r++) {
        for (unsigned c = 0; c < 13; c++) {
            ASSERT_TRUE(blot_canvas_set(can, c, r, true));
            ASSERT_EQ(cells(can), Cells({ { c, r } }));
            ASSERT_TRUE(blot_canvas_set(can, c, r, false));
            ASSERT_TRUE(cells(can).empty()) << c << "," << r;
        }
    }
}

TEST(Canvas, out_of_bounds_access)
{
    Canvas can(10, 10);
    const unsigned pts[][2] = { { 10, 0 }, { 0, 10 }, { UINT_MAX, UINT_MAX } };
    for (const auto &p : pts) {
        EXPECT_FALSE(blot_canvas_set(can, p[0], p[1], true)) << p[0] << "," << p[1];
        EXPECT_FALSE(blot_canvas_get(can, p[0], p[1]));
        EXPECT_EQ(blot_canvas_get_cell(can, p[0], p[1]), 0);
    }
    EXPECT_TRUE(cells(can).empty());
}

TEST(Canvas, plot_char_by_flag)
{
    Canvas uni(4, 4, BLOT_RENDER_NONE), ascii(4, 4, BLOT_RENDER_NO_UNICODE);
    blot_canvas_draw_point(uni, 1, 2);
    blot_canvas_draw_point(ascii, 1, 2);
    EXPECT_EQ(blot_canvas_get_cell(uni, 1, 2), (wchar_t)0x2022);
    EXPECT_EQ(blot_canvas_get_cell(ascii, 1, 2), L'*');
}

namespace {
/* one dot at a time on a 1x1 braille canvas */
void expect_braille_dots(blot_render_flags flags, const wchar_t expected[4][2])
{
    for (unsigned y = 0; y < 4; y++) {
        for (unsigned x = 0; x < 2; x++) {
            Canvas can(1, 1, flags);
            ASSERT_TRUE(can.can);
            blot_canvas_draw_point(can, x, y);
            EXPECT_EQ((unsigned)blot_canvas_get_cell(can, 0, 0), (unsigned)expected[y][x])
                << "dot (" << x << "," << y << ")";
        }
    }
    Canvas all(1, 1, flags);
    for (unsigned y = 0; y < 4; y++)
        for (unsigned x = 0; x < 2; x++)
            blot_canvas_draw_point(all, x, y);
    EXPECT_EQ((unsigned)blot_canvas_get_cell(all, 0, 0), 0x28FFu);
}
} // namespace

TEST(Canvas, braille_dots_default)
{
    const wchar_t expected[4][2] = {
        { 0x2840, 0x2880 }, { 0x2804, 0x2820 }, { 0x2802, 0x2810 }, { 0x2801, 0x2808 },
    };
    expect_braille_dots(BLOT_RENDER_BRAILLE, expected);
}

TEST(Canvas, braille_dots_dont_invert)
{
    const wchar_t expected[4][2] = {
        { 0x2801, 0x2808 }, { 0x2802, 0x2810 }, { 0x2804, 0x2820 }, { 0x2840, 0x2880 },
    };
    expect_braille_dots(BLOT_RENDER_BRAILLE | BLOT_RENDER_DONT_INVERT_Y_AXIS, expected);
}

TEST(Canvas, braille_cell_addressing)
{
    Canvas can(3, 2, BLOT_RENDER_BRAILLE);
    ASSERT_TRUE(can.can);
    EXPECT_EQ(can->dim.cols, 6u);
    EXPECT_EQ(can->dim.rows, 8u);
    EXPECT_EQ(can->bitmap_bytes, 6u);

    blot_canvas_draw_point(can, 5, 7);
    std::map<Cell, wchar_t> expected = { { { 2, 1 }, 0x2808 } };
    EXPECT_EQ(glyphs(can), expected);
    EXPECT_EQ(blot_canvas_get_cell(can, 3, 0), 0);
    EXPECT_EQ(blot_canvas_get_cell(can, 0, 2), 0);
    EXPECT_EQ(cells(can), Cells({ { 5, 7 } }));   /* the other 47 dots are clear */

    EXPECT_TRUE(blot_canvas_set(can, 5, 7, false));
    EXPECT_TRUE(glyphs(can).empty());
}

TEST(Canvas, braille_with_no_unicode_rejected)
{
    GError *error = NULL;
    blot_canvas *can = blot_canvas_new(4, 4, BLOT_RENDER_BRAILLE | BLOT_RENDER_NO_UNICODE, 1, &error);
    EXPECT_TRUE(can == NULL);
    ASSERT_TRUE(error != NULL);
    EXPECT_EQ(error->code, EINVAL);
    g_clear_error(&error);
    blot_canvas_delete(can);
}

TEST(Canvas, line_horizontal_vertical)
{
    EXPECT_CELLS_EQ(line_cells(12, 12, 2, 3, 8, 3), row_cells(3, 2, 8));
    EXPECT_CELLS_EQ(line_cells(12, 12, 8, 3, 2, 3), row_cells(3, 2, 8));
    EXPECT_CELLS_EQ(line_cells(12, 12, 4, 1, 4, 9), col_cells(4, 1, 9));
    EXPECT_CELLS_EQ(line_cells(12, 12, 4, 9, 4, 1), col_cells(4, 1, 9));
}

TEST(Canvas, line_diagonals)
{
    Canvas can(6, 6);
    blot_canvas_draw_line(can, 0, 0, 5, 5);
    blot_canvas_draw_line(can, 0, 5, 5, 0);
    Cells expected;
    for (unsigned i = 0; i < 6; i++) {
        expected.insert({ i, i });
        expected.insert({ i, 5 - i });
    }
    EXPECT_CELLS_EQ(cells(can), expected);
}

TEST(Canvas, line_reaches_both_endpoints)
{
    for (const auto &d : DELTAS) {
        SCOPED_TRACE(testing::Message() << "(12,12) to (" << 12 + d[0] << "," << 12 + d[1] << ")");
        Cells got = line_cells(25, 25, 12, 12, 12 + d[0], 12 + d[1]);
        EXPECT_TRUE(got.count({ 12, 12 })) << cells_str(got);
        EXPECT_TRUE(got.count({ 12 + d[0], 12 + d[1] })) << cells_str(got);
        EXPECT_EQ(got.size(), (size_t)std::max(std::abs(d[0]), std::abs(d[1])) + 1) << cells_str(got);
    }
}

TEST(Canvas, line_direction_independent)
{
    for (const auto &d : DELTAS) {
        SCOPED_TRACE(testing::Message() << "(12,12) - (" << 12 + d[0] << "," << 12 + d[1] << ")");
        EXPECT_CELLS_EQ(line_cells(25, 25, 12 + d[0], 12 + d[1], 12, 12),
                        line_cells(25, 25, 12, 12, 12 + d[0], 12 + d[1]));
    }
}

TEST(Canvas, line_between_neighbours)
{
    EXPECT_CELLS_EQ(line_cells(5, 5, 0, 0, 1, 1), Cells({ { 0, 0 }, { 1, 1 } }));
    EXPECT_CELLS_EQ(line_cells(5, 5, 0, 0, 1, 0), Cells({ { 0, 0 }, { 1, 0 } }));
    EXPECT_CELLS_EQ(line_cells(5, 5, 0, 0, 0, 1), Cells({ { 0, 0 }, { 0, 1 } }));
    EXPECT_CELLS_EQ(line_cells(5, 5, 2, 2, 2, 2), Cells({ { 2, 2 } }));
}

TEST(Canvas, line_clipped_at_edge)
{
    EXPECT_CELLS_EQ(line_cells(10, 10, 5, 5, 30, 5), row_cells(5, 5, 9));
    EXPECT_CELLS_EQ(line_cells(10, 10, 5, 5, 5, 40), col_cells(5, 5, 9));
}

TEST(Canvas, fill_rect_exact)
{
    Cells nine;
    for (unsigned c = 2; c <= 4; c++)
        for (unsigned r = 1; r <= 3; r++)
            nine.insert({ c, r });
    {
        Canvas can(8, 8);
        blot_canvas_fill_rect(can, 2, 1, 4, 3);
        EXPECT_CELLS_EQ(cells(can), nine);
    }
    {
        Canvas can(8, 8);
        blot_canvas_fill_rect(can, 4, 3, 2, 1);
        EXPECT_CELLS_EQ(cells(can), nine);
    }
    {
        Canvas can(8, 8);
        blot_canvas_fill_rect(can, 6, 6, 20, 20);
        EXPECT_CELLS_EQ(cells(can), Cells({ { 6, 6 }, { 6, 7 }, { 7, 6 }, { 7, 7 } }));
    }
}

TEST(Canvas, draw_rect_outline)
{
    Canvas can(6, 6);
    blot_canvas_draw_rect(can, 1, 1, 4, 4);
    Cells border = row_cells(1, 1, 4);
    for (const auto &c : row_cells(4, 1, 4)) border.insert(c);
    for (const auto &c : col_cells(1, 1, 4)) border.insert(c);
    for (const auto &c : col_cells(4, 1, 4)) border.insert(c);
    ASSERT_EQ(border.size(), 12u);
    EXPECT_CELLS_EQ(cells(can), border);
}

TEST(Canvas, draw_rect_smallest)
{
    Canvas can(6, 6);
    blot_canvas_draw_rect(can, 1, 1, 2, 2);
    EXPECT_CELLS_EQ(cells(can), Cells({ { 1, 1 }, { 1, 2 }, { 2, 1 }, { 2, 2 } }));
}

TEST(Canvas, line_rounds_fractional_endpoints)
{
    /* each end goes to its nearest cell: (0.6,0.4) -> (1,0), (4.4,2.6) -> (4,3) */
    Cells diag = { { 1, 0 }, { 2, 1 }, { 3, 2 }, { 4, 3 } };
    EXPECT_CELLS_EQ(line_cells(8, 8, 0.6, 0.4, 4.4, 2.6), diag);
    EXPECT_CELLS_EQ(line_cells(8, 8, 4.4, 2.6, 0.6, 0.4), diag);
}

TEST(Canvas, line_exact_cells_shallow_and_steep)
{
    /* rows round(0.3 * x), the 1.5 tie at x=5 towards the far end */
    const unsigned rows[] = { 0, 0, 1, 1, 1, 2, 2, 2, 2, 3, 3 };
    Cells shallow, steep;
    for (unsigned i = 0; i < 11; i++) {
        shallow.insert({ i, rows[i] });
        steep.insert({ rows[i], i });
    }
    EXPECT_CELLS_EQ(line_cells(12, 12, 0, 0, 10, 3), shallow);
    EXPECT_CELLS_EQ(line_cells(12, 12, 10, 3, 0, 0), shallow);
    EXPECT_CELLS_EQ(line_cells(12, 12, 0, 0, 3, 10), steep);
    EXPECT_CELLS_EQ(line_cells(12, 12, 3, 10, 0, 0), steep);
}

TEST(Canvas, line_tie_same_both_directions)
{
    Cells tie = { { 0, 0 }, { 1, 1 }, { 2, 1 } };
    EXPECT_CELLS_EQ(line_cells(5, 5, 0, 0, 2, 1), tie);
    EXPECT_CELLS_EQ(line_cells(5, 5, 2, 1, 0, 0), tie);
}

TEST(Canvas, line_interior_follows_true_segment)
{
    /* ends (0.49,0) -> (0,0) and (10.49,3) -> (10,3); inside, the rows are
     * nearest to y = 0.3 * (x - 0.49), not to the line between the end
     * cells (that would put x=2 and x=5 a row higher) */
    const unsigned rows[] = { 0, 0, 0, 1, 1, 1, 2, 2, 2, 3, 3 };
    Cells want;
    for (unsigned i = 0; i < 11; i++)
        want.insert({ i, rows[i] });
    EXPECT_CELLS_EQ(line_cells(12, 12, 0.49, 0, 10.49, 3), want);
    EXPECT_CELLS_EQ(line_cells(12, 12, 10.49, 3, 0.49, 0), want);
}

TEST(Canvas, line_without_gaps_when_ends_round_steeper)
{
    /* ends (0.6,0.4) -> (1,0) and (2.6,2.4) -> (3,2): the true segment
     * at x=2 is y=1.8, but the cell moves one row per column at most */
    Cells want = { { 1, 0 }, { 2, 1 }, { 3, 2 } };
    EXPECT_CELLS_EQ(line_cells(5, 5, 0.6, 0.4, 2.6, 2.4), want);
    EXPECT_CELLS_EQ(line_cells(5, 5, 2.6, 2.4, 0.6, 0.4), want);
}

TEST(Canvas, line_with_non_finite_end_terminates)
{
    /* lround() of NaN/inf is unspecified; such a segment is not drawn
     * (what it should look like is #6) */
    RUN_ISOLATED(5, {
        EXPECT_CELLS_EQ(line_cells(8, 8, 1, 1, NAN, 5), Cells());
        EXPECT_CELLS_EQ(line_cells(8, 8, 1, 1, 5, NAN), Cells());
        EXPECT_CELLS_EQ(line_cells(8, 8, NAN, NAN, 5, 5), Cells());
        EXPECT_CELLS_EQ(line_cells(8, 8, 1, 1, INFINITY, 5), Cells());
        EXPECT_CELLS_EQ(line_cells(8, 8, 1, -INFINITY, 5, 5), Cells());
    });
}

/* --- #5: lines that reach far outside the canvas ----------------------
 *
 * A line used to be walked cell by cell from end to end, also off the
 * canvas: an end at 1e12 took hours, and an end beyond the range of a
 * long made lround() return LONG_MIN, which flipped the line and walked
 * ~2^63 cells.  Only the visible cells may be walked, and those must be
 * the cells the full walk sets on the canvas. */

namespace {

/* The line walk of #4 (!6), before #5: it visits every cell from one end
 * to the other and drops those off the canvas.  Only for ends where that
 * is fast.  This is the reference the clipped walk must match.  The one
 * change since #4 is the tie rule (#5): an interior tie goes to the upper
 * cell, not towards the far end. */
Cells unclipped_line_cells(unsigned w, unsigned h, double x0, double y0, double x1, double y1)
{
    Cells out;
    if (!std::isfinite(x0) || !std::isfinite(y0) || !std::isfinite(x1) || !std::isfinite(y1))
        return out;

    long cx0 = lround(x0), cy0 = lround(y0);
    long cx1 = lround(x1), cy1 = lround(y1);

    if (cx0 > cx1 || (cx0 == cx1 && cy0 > cy1)) {
        std::swap(cx0, cx1);
        std::swap(cy0, cy1);
        std::swap(x0, x1);
        std::swap(y0, y1);
    }

    bool y_down = cy1 < cy0;
    unsigned long dx = (unsigned long)cx1 - (unsigned long)cx0;
    unsigned long dy = y_down ? (unsigned long)cy0 - (unsigned long)cy1
                              : (unsigned long)cy1 - (unsigned long)cy0;

    bool x_major = dx >= dy;
    unsigned long n = x_major ? dx : dy;
    unsigned long m = x_major ? dy : dx;

    double a0 = x_major ? x0 : y0;
    double a1 = x_major ? x1 : y1;
    double b0 = x_major ? y0 : x0;
    double b1 = x_major ? y1 : x1;
    double ca0 = (double)(x_major ? cx0 : cy0);
    double cb0 = (double)(x_major ? cy0 : cx0);
    double a_dir = (!x_major && y_down) ? -1 : 1;
    double b_dir = (x_major && y_down) ? -1 : 1;

    unsigned long off = 0;

    for (unsigned long i = 0; ; i++) {
        if (i == n) {
            off = m;
        } else if (i > 0) {
            double a = ca0 + a_dir * (double)i;
            double b = b0 + (a - a0) * (b1 - b0) / (a1 - a0);
            double d = (b - cb0) * b_dir;
            unsigned long want;
            if (!(d > 0))
                want = 0;
            else if (d >= (double)m)
                want = m;
            else
                want = (unsigned long)lround(d);
            /* #5: a tie goes to the upper cell, also walking down */
            if (b_dir < 0 && d > 0 && d < (double)m && (double)want - d == 0.5)
                want--;

            unsigned long lo = off ? off - 1 : 0;
            unsigned long hi = off < m ? off + 1 : m;
            if (m - lo > n - i)
                lo = m - (n - i);
            if (want < lo)
                want = lo;
            if (want > hi)
                want = hi;
            off = want;
        }

        unsigned long ox = x_major ? i : off;
        unsigned long oy = x_major ? off : i;

        long x = (long)((unsigned long)cx0 + ox);
        long y = y_down ? (long)((unsigned long)cy0 - oy)
                        : (long)((unsigned long)cy0 + oy);

        if (x >= 0 && y >= 0 && x < (long)w && y < (long)h)
            out.insert({ (unsigned)x, (unsigned)y });

        if (i == n)
            break;
    }
    return out;
}

Cells diag_cells(unsigned c0, unsigned r0, unsigned len)
{
    Cells out;
    for (unsigned i = 0; i < len; i++)
        out.insert({ c0 + i, r0 + i });
    return out;
}

} // namespace

TEST(Canvas, line_end_at_1e12_terminates)
{
    RUN_ISOLATED(5, {
        EXPECT_CELLS_EQ(line_cells(10, 10, 2, 2, 2, 1e12), col_cells(2, 2, 9));
        EXPECT_CELLS_EQ(line_cells(10, 10, 2, 2, 1e12, 2), row_cells(2, 2, 9));
        EXPECT_CELLS_EQ(line_cells(10, 10, 2, 2, 1e12, 1e12), diag_cells(2, 2, 8));
        EXPECT_CELLS_EQ(line_cells(10, 10, 2, 7, 2, -1e12), col_cells(2, 0, 7));
        EXPECT_CELLS_EQ(line_cells(10, 10, -1e12, 3, 1e12, 3), row_cells(3, 0, 9));
    });
}

TEST(Canvas, line_end_at_1e300_terminates)
{
    /* far beyond the range of a long: lround() can't take it */
    RUN_ISOLATED(5, {
        EXPECT_CELLS_EQ(line_cells(10, 10, 2, 2, 2, 1e300), col_cells(2, 2, 9));
        EXPECT_CELLS_EQ(line_cells(10, 10, 2, 2, 1e300, 2), row_cells(2, 2, 9));
        EXPECT_CELLS_EQ(line_cells(10, 10, 2, 2, 1e300, 1e300), diag_cells(2, 2, 8));
        EXPECT_CELLS_EQ(line_cells(10, 10, 2, 2, 1e19, 2), row_cells(2, 2, 9));
        EXPECT_CELLS_EQ(line_cells(10, 10, 2, 2, DBL_MAX, 2), row_cells(2, 2, 9));
        EXPECT_CELLS_EQ(line_cells(10, 10, 20, 2, 1e300, 2), Cells());
    });
}

TEST(Canvas, line_end_at_minus_1e300_terminates)
{
    RUN_ISOLATED(5, {
        EXPECT_CELLS_EQ(line_cells(10, 10, 2, 7, 2, -1e300), col_cells(2, 0, 7));
        EXPECT_CELLS_EQ(line_cells(10, 10, 7, 2, -1e300, 2), row_cells(2, 0, 7));
        EXPECT_CELLS_EQ(line_cells(10, 10, 7, 7, -1e300, -1e300), diag_cells(0, 0, 8));
        EXPECT_CELLS_EQ(line_cells(10, 10, 7, 2, -1e19, 2), row_cells(2, 0, 7));
        EXPECT_CELLS_EQ(line_cells(10, 10, 7, 2, -DBL_MAX, 2), row_cells(2, 0, 7));
        /* both ends far out, on either side */
        EXPECT_CELLS_EQ(line_cells(10, 10, -1e300, 3, 1e300, 3), row_cells(3, 0, 9));
        EXPECT_CELLS_EQ(line_cells(10, 10, 4, 1e300, 4, -1e300), col_cells(4, 0, 9));
        EXPECT_CELLS_EQ(line_cells(10, 10, -1e300, -1e300, -1e300, 1e300), Cells());
    });
}

TEST(Canvas, line_crossing_from_far_outside)
{
    RUN_ISOLATED(5, {
        /* y = x + 5, ends a billion cells out */
        EXPECT_CELLS_EQ(line_cells(10, 10, -1e9, -1e9 + 5, 1e9, 1e9 + 5), diag_cells(0, 5, 5));
        EXPECT_CELLS_EQ(line_cells(10, 10, 1e9, 1e9 + 5, -1e9, -1e9 + 5), diag_cells(0, 5, 5));

        /* y = 2 + x/4: the rows nearest the true segment, a half
         * rounded towards the far end */
        const unsigned rows[] = { 2, 2, 3, 3, 3, 3, 4, 4, 4, 4 };
        Cells shallow;
        for (unsigned x = 0; x < 10; x++)
            shallow.insert({ x, rows[x] });
        EXPECT_CELLS_EQ(line_cells(10, 10, -4e6, 2 - 1e6, 4e6, 2 + 1e6), shallow);

        /* x = 2 + y/4, from a billion rows below to a billion above */
        Cells steep;
        for (unsigned y = 0; y < 10; y++)
            steep.insert({ rows[y], y });
        EXPECT_CELLS_EQ(line_cells(10, 10, 2 - 1e9, -4e9, 2 + 1e9, 4e9), steep);
    });
}

TEST(Canvas, line_clipped_matches_unclipped)
{
    /* the cells on the canvas are those of the full walk, for random
     * segments partly or wholly off the canvas, on ranges where the full
     * walk is still fast; ends are often whole or half cells, to hit the
     * rounding ties, or tenths, which add rounding noise, and often near a
     * diagonal, where the no-gap rule matters most */
    std::mt19937_64 rng(5);
    auto uni = [&](double lo, double hi) {
        return std::uniform_real_distribution<double>(lo, hi)(rng);
    };
    auto pick = [&](unsigned k) { return (unsigned)(rng() % k); };
    auto snap = [&](double v) {
        switch (pick(5)) {
        case 0: return std::round(v);
        case 1: return std::floor(v) + 0.5;
        case 2: return std::round(v * 10) / 10;   /* inexact: rounding noise */
        default: return v;
        }
    };

    unsigned failures = 0;
    for (unsigned t = 0; t < 200000 && failures < 10; t++) {
        unsigned w = 1 + pick(40), h = 1 + pick(20);
        static const double spans[] = { 2, 10, 100, 2000 };
        double span = t % 1000 == 0 ? 1e5 : spans[pick(4)];
        double x0 = snap(uni(-span, w + span)), y0 = snap(uni(-span, h + span));
        double x1, y1;
        if (pick(3) == 0) {
            /* near a diagonal: |dx| and |dy| differ by up to two cells */
            double len = uni(0, 2 * span);
            x1 = snap(x0 + (pick(2) ? len : -len));
            y1 = snap(y0 + (pick(2) ? 1 : -1) * (len + uni(-2, 2)));
        } else {
            x1 = snap(uni(-span, w + span));
            y1 = snap(uni(-span, h + span));
        }
        Cells want = unclipped_line_cells(w, h, x0, y0, x1, y1);
        Cells got = line_cells(w, h, x0, y0, x1, y1);
        if (got != want) {
            failures++;
            ADD_FAILURE() << std::setprecision(17) << "canvas " << w << "x" << h
                          << ", line (" << x0 << "," << y0 << ") -> (" << x1 << "," << y1 << ")\n"
                          << cells_eq(got, want).message();
        }
    }
}

TEST(Canvas, line_diagonal_rounding_matches_full_walk)
{
    /* a true diagonal (53.4 by 53.4) between end cells 53 by 54 apart:
     * the rounding of the tenths puts some steps just below a half, and
     * from the first of those on the cells lag one row; the canvas must
     * show the same cells as the full walk */
    EXPECT_CELLS_EQ(line_cells(33, 12, -29, -48.5, 24.4, 4.9),
                    unclipped_line_cells(33, 12, -29, -48.5, 24.4, 4.9));
    EXPECT_CELLS_EQ(line_cells(33, 12, 24.4, 4.9, -29, -48.5),
                    unclipped_line_cells(33, 12, -29, -48.5, 24.4, 4.9));
}

TEST(Canvas, line_interior_tie_goes_to_upper_cell)
{
    /* y = 3 - x/2 crosses y = 2.5 and y = 1.5 at the centres of columns 1
     * and 3: those ties go to the upper row, as a value on a cell edge
     * does on the axes (G18 in #14), whichever end the walk starts from */
    Cells down = { { 0, 3 }, { 1, 3 }, { 2, 2 }, { 3, 2 }, { 4, 1 } };
    EXPECT_CELLS_EQ(line_cells(6, 6, 0, 3, 4, 1), down);
    EXPECT_CELLS_EQ(line_cells(6, 6, 4, 1, 0, 3), down);

    /* y = 1 + x/2: upwards the upper row was already the far one */
    Cells up = { { 0, 1 }, { 1, 2 }, { 2, 2 }, { 3, 3 }, { 4, 3 } };
    EXPECT_CELLS_EQ(line_cells(6, 6, 0, 1, 4, 3), up);
    EXPECT_CELLS_EQ(line_cells(6, 6, 4, 3, 0, 1), up);

    /* steep: x = 3 - y/2, ties at the centres of rows 1 and 3 go to the
     * larger column (the walk goes up in y, so x is the minor axis) */
    Cells steep = { { 3, 0 }, { 3, 1 }, { 2, 2 }, { 2, 3 }, { 1, 4 } };
    EXPECT_CELLS_EQ(line_cells(6, 6, 3, 0, 1, 4), steep);
    EXPECT_CELLS_EQ(line_cells(6, 6, 1, 4, 3, 0), steep);
}
