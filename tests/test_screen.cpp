#include <gtest/gtest.h>

extern "C" {
#include "blot_screen.h"
#include "blot_layer.h"
#include "blot_axis.h"
#include "blot_canvas.h"
};

TEST(BaseTest, AssertionTrue)
{
    ASSERT_TRUE(true);
    ASSERT_FALSE(false);
}

TEST(Screen, alloc_100x100)
{
    const blot_dimensions dim{ 100, 100 };
    const blot_margins mrg{ 10, 2, 2, 2 };
    blot_render_flags flags{};
    GError *error = NULL;

    blot_screen *screen = blot_screen_new(&dim, &mrg, flags, &error);
    ASSERT_TRUE(screen != NULL);
    blot_screen_delete(screen);
}

TEST(Screen, alloc_NULL_dim)
{
    const blot_margins mrg{ 1, 1, 1, 1 };
    blot_render_flags flags{};
    GError *error = NULL;

    blot_screen *screen = blot_screen_new(NULL, &mrg, flags, &error);
    ASSERT_TRUE(screen == NULL);
    ASSERT_TRUE(error != NULL);
    g_error_free(error);
}

TEST(Screen, alloc_zero_dim)
{
    const blot_dimensions dim{ 0, 0 };
    const blot_margins mrg{ 1, 1, 1, 1 };
    blot_render_flags flags{};
    GError *error = NULL;

    blot_screen *screen = blot_screen_new(&dim, &mrg, flags, &error);
    ASSERT_TRUE(screen == NULL);
    ASSERT_TRUE(error != NULL);
    g_error_free(error);
}

TEST(Screen, alloc_NULL_mrg)
{
    const blot_dimensions dim{ 100, 100 };
    blot_render_flags flags{};
    GError *error = NULL;

    blot_screen *screen = blot_screen_new(&dim, NULL, flags, &error);
    ASSERT_TRUE(screen == NULL);
    ASSERT_TRUE(error != NULL);
    g_error_free(error);
}

TEST(Screen, render_empty)
{
    const blot_dimensions dim{ 100, 100 };
    const blot_margins mrg{ 10, 2, 2, 2 };
    blot_render_flags flags{};
    GError *error = NULL;

    blot_screen *screen = blot_screen_new(&dim, &mrg, flags, &error);
    ASSERT_TRUE(screen != NULL);

    blot_xy_limits lim{ 0, 1, 0, 1 };
    blot_axis *x_axis = blot_axis_new(0, true, 1, 98, 0, 1, NULL, &error);
    ASSERT_TRUE(x_axis != NULL);
    blot_axis *y_axis = blot_axis_new(1, true, 1, 98, 0, 1, NULL, &error);
    ASSERT_TRUE(y_axis != NULL);

    ASSERT_TRUE(blot_screen_render(screen, &lim, x_axis, y_axis, 0, NULL, NULL, &error));

    blot_axis_delete(x_axis);
    blot_axis_delete(y_axis);
    blot_screen_delete(screen);
}

TEST(Screen, render_and_get_text)
{
    const blot_dimensions dim{ 100, 100 };
    const blot_margins mrg{ 10, 2, 2, 2 };
    blot_render_flags flags{};
    GError *error = NULL;

    blot_screen *screen = blot_screen_new(&dim, &mrg, flags, &error);
    ASSERT_TRUE(screen != NULL);

    blot_xy_limits lim{ 0, 1, 0, 1 };
    blot_axis *x_axis = blot_axis_new(0, true, 1, 98, 0, 1, NULL, &error);
    ASSERT_TRUE(x_axis != NULL);
    blot_axis *y_axis = blot_axis_new(1, true, 1, 98, 0, 1, NULL, &error);
    ASSERT_TRUE(y_axis != NULL);

    const int xs[] = {1, 2, 3};
    const int ys[] = {4, 5, 6};
    blot_layer *layer = blot_layer_new(BLOT_SCATTER, BLOT_DATA_INT32, 3, xs, ys, 1, "scatter", &error);
    ASSERT_TRUE(layer != NULL);

    blot_canvas *canvas = blot_canvas_new(98, 98, flags, 1, &error);
    ASSERT_TRUE(canvas != NULL);

    ASSERT_TRUE(blot_screen_render(screen, &lim, x_axis, y_axis, 1, &layer, &canvas, &error));

    gsize txt_size;
    const wchar_t *txt = blot_screen_get_text(screen, &txt_size, &error);
    ASSERT_TRUE(txt != NULL);
    ASSERT_TRUE(error == NULL);
    ASSERT_GT(txt_size, 0);

    blot_canvas_delete(canvas);
    blot_layer_delete(layer);
    blot_axis_delete(x_axis);
    blot_axis_delete(y_axis);
    blot_screen_delete(screen);
}