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

#include <climits>
#include <cstddef>
#include <cstdint>
#include <memory>

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

/* struct blot_canvas declares its bitmap __aligned64, so the canvas must be
 * allocated 64-byte aligned (#15): g_malloc only guarantees 16 bytes, and
 * UBSan reports every access to a misaligned canvas.  A single allocation
 * can be 64-byte aligned by luck, so keep many canvases of varied sizes
 * and flags alive at once and require every one to be aligned. */
TEST(Canvas, bitmap_is_64_byte_aligned)
{
    const blot_render_flags flags[] = {
        BLOT_RENDER_NONE,
        BLOT_RENDER_NO_UNICODE,
        BLOT_RENDER_BRAILLE,
        (blot_render_flags)(BLOT_RENDER_BRAILLE | BLOT_RENDER_DONT_INVERT_Y_AXIS),
    };
    /* unique_ptr: an ASSERT returns early, and must not leak the canvases */
    std::vector<std::unique_ptr<blot_canvas, decltype(&blot_canvas_delete)>> cans;
    unsigned misaligned = 0;
    for (unsigned i = 0; i < 256; i++) {
        unsigned cols = 1 + (i * 7) % 97;
        unsigned rows = 1 + (i * 13) % 61;
        GError *error = NULL;
        blot_canvas *can = blot_canvas_new(cols, rows, flags[i % 4], 1, &error);
        if (can)
            cans.emplace_back(can, blot_canvas_delete);
        ASSERT_TRUE(can != NULL) << cols << "x" << rows << ": "
                                 << (error ? error->message : "no error");
        ASSERT_TRUE(error == NULL) << error->message;
        /* uintptr_t: no access through a misaligned pointer here */
        uintptr_t base = (uintptr_t)can;
        uintptr_t bitmap = base + offsetof(blot_canvas, bitmap);
        if (base % 64 || bitmap % 64)
            misaligned++;
    }
    EXPECT_EQ(misaligned, 0u) << "of " << cans.size() << " canvases";
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
