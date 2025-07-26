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
