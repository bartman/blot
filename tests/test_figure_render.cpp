#include <gtest/gtest.h>

extern "C" {
#include "blot_terminal.h"
#include "blot_figure.h"
#include "blot_error.h"
#include "blot_types.h"
#include "blot_screen.h"
#include "blot_layer.h"
#include "blot_canvas.h"
#include "blot_axis.h"
};

TEST(Figure, render_no_layers)
{
    GError *error = NULL;
    blot_figure *fig = blot_figure_new(&error);
    ASSERT_TRUE(fig != NULL);
    ASSERT_TRUE(error == NULL);

    blot_screen *screen = blot_figure_render(fig, BLOT_RENDER_NONE, &error);
    ASSERT_TRUE(screen == NULL);
    ASSERT_TRUE(error != NULL);
    g_clear_error(&error);

    blot_figure_delete(fig);
}

TEST(Figure, render_with_flags)
{
    GError *error = NULL;

    // fake size for testing
    ASSERT_TRUE(blot_terminal_set_size({100,100}, &error));

    blot_figure *fig = blot_figure_new(&error);
    ASSERT_TRUE(fig != NULL);

    const int xs[] = {1, 2, 3};
    const int ys[] = {4, 5, 6};
    ASSERT_TRUE(blot_figure_scatter(fig, BLOT_DATA_INT32, 3, xs, ys, 1, "scatter", &error));
    ASSERT_TRUE(error == NULL);

    // Test with BLOT_RENDER_BRAILLE
    blot_screen *screen = blot_figure_render(fig, BLOT_RENDER_BRAILLE, &error);
    ASSERT_TRUE(screen != NULL);
    ASSERT_TRUE(error == NULL);
    blot_screen_delete(screen);

    // Test with BLOT_RENDER_LEGEND_ABOVE
    screen = blot_figure_render(fig, BLOT_RENDER_LEGEND_ABOVE, &error);
    ASSERT_TRUE(screen != NULL);
    ASSERT_TRUE(error == NULL);
    blot_screen_delete(screen);

    // Test with BLOT_RENDER_LEGEND_BELOW
    screen = blot_figure_render(fig, BLOT_RENDER_LEGEND_BELOW, &error);
    ASSERT_TRUE(screen != NULL);
    ASSERT_TRUE(error == NULL);
    blot_screen_delete(screen);

    // Test with BLOT_RENDER_CLEAR
    screen = blot_figure_render(fig, BLOT_RENDER_CLEAR, &error);
    ASSERT_TRUE(screen != NULL);
    ASSERT_TRUE(error == NULL);
    blot_screen_delete(screen);

    // Test with BLOT_RENDER_NO_X_AXIS
    screen = blot_figure_render(fig, BLOT_RENDER_NO_X_AXIS, &error);
    ASSERT_TRUE(screen != NULL);
    ASSERT_TRUE(error == NULL);
    blot_screen_delete(screen);

    // Test with BLOT_RENDER_NO_Y_AXIS
    screen = blot_figure_render(fig, BLOT_RENDER_NO_Y_AXIS, &error);
    ASSERT_TRUE(screen != NULL);
    ASSERT_TRUE(error == NULL);
    blot_screen_delete(screen);

    // Test with BLOT_RENDER_DONT_INVERT_Y_AXIS
    screen = blot_figure_render(fig, BLOT_RENDER_DONT_INVERT_Y_AXIS, &error);
    ASSERT_TRUE(screen != NULL);
    ASSERT_TRUE(error == NULL);
    blot_screen_delete(screen);

    // Test with BLOT_RENDER_NO_UNICODE
    screen = blot_figure_render(fig, BLOT_RENDER_NO_UNICODE, &error);
    ASSERT_TRUE(screen != NULL);
    ASSERT_TRUE(error == NULL);
    blot_screen_delete(screen);

    // Test with BLOT_RENDER_NO_COLOR
    screen = blot_figure_render(fig, BLOT_RENDER_NO_COLOR, &error);
    ASSERT_TRUE(screen != NULL);
    ASSERT_TRUE(error == NULL);
    blot_screen_delete(screen);

    blot_figure_delete(fig);
}

TEST(Figure, render_with_multiple_layer_types)
{
    GError *error = NULL;

    // fake size for testing
    ASSERT_TRUE(blot_terminal_set_size({100,100}, &error));

    blot_figure *fig = blot_figure_new(&error);
    ASSERT_TRUE(fig != NULL);

    const int xs1[] = {1, 2, 3};
    const int ys1[] = {4, 5, 6};
    ASSERT_TRUE(blot_figure_scatter(fig, BLOT_DATA_INT32, 3, xs1, ys1, 1, "scatter", &error));
    ASSERT_TRUE(error == NULL);

    const float xs2[] = {7.0f, 8.0f, 9.0f};
    const float ys2[] = {10.0f, 11.0f, 12.0f};
    ASSERT_TRUE(blot_figure_line(fig, BLOT_DATA_FLOAT, 3, xs2, ys2, 2, "line", &error));
    ASSERT_TRUE(error == NULL);

    const double xs3[] = {13.0, 14.0, 15.0};
    const double ys3[] = {16.0, 17.0, 18.0};
    ASSERT_TRUE(blot_figure_bar(fig, BLOT_DATA_DOUBLE, 3, xs3, ys3, 3, "bar", &error));
    ASSERT_TRUE(error == NULL);

    blot_screen *screen = blot_figure_render(fig, BLOT_RENDER_NONE, &error);
    ASSERT_TRUE(screen != NULL);
    ASSERT_TRUE(error == NULL);
    blot_screen_delete(screen);

    blot_figure_delete(fig);
}

TEST(Figure, render_automatic_limits)
{
    GError *error = NULL;

    // fake size for testing
    ASSERT_TRUE(blot_terminal_set_size({100,100}, &error));

    blot_figure *fig = blot_figure_new(&error);
    ASSERT_TRUE(fig != NULL);

    const int xs[] = {1, 2, 10};
    const int ys[] = {1, 5, 20};
    ASSERT_TRUE(blot_figure_scatter(fig, BLOT_DATA_INT32, 3, xs, ys, 1, "scatter", &error));
    ASSERT_TRUE(error == NULL);

    blot_screen *screen = blot_figure_render(fig, BLOT_RENDER_NONE, &error);
    ASSERT_TRUE(screen != NULL);
    ASSERT_TRUE(error == NULL);

    blot_screen_delete(screen);
    blot_figure_delete(fig);
}

TEST(Figure, render_automatic_dimensions)
{
    GError *error = NULL;

    // fake size for testing
    ASSERT_TRUE(blot_terminal_set_size({100,100}, &error));

    blot_figure *fig = blot_figure_new(&error);
    ASSERT_TRUE(fig != NULL);

    const int xs[] = {1, 2, 3};
    const int ys[] = {4, 5, 6};
    ASSERT_TRUE(blot_figure_scatter(fig, BLOT_DATA_INT32, 3, xs, ys, 1, "scatter", &error));
    ASSERT_TRUE(error == NULL);

    // Do not set screen dimensions, expect them to be derived from terminal size
    blot_screen *screen = blot_figure_render(fig, BLOT_RENDER_NONE, &error);
    ASSERT_TRUE(screen != NULL);
    ASSERT_TRUE(error == NULL);

    // Check if dimensions are automatically calculated (should be > 0)
    ASSERT_GT(screen->dim.cols, 0);
    ASSERT_GT(screen->dim.rows, 0);

    blot_screen_delete(screen);
    blot_figure_delete(fig);
}

TEST(Figure, render_layer_render_failure)
{
    GError *error = NULL;
    blot_figure *fig = blot_figure_new(&error);
    ASSERT_TRUE(fig != NULL);

    // Add a layer that will cause blot_layer_new to fail (e.g., NULL ys)
    const int xs[] = {1, 2, 3};
    ASSERT_FALSE(blot_figure_scatter(fig, BLOT_DATA_INT32, 3, xs, NULL, 1, "scatter", &error));
    ASSERT_TRUE(error != NULL);
    g_clear_error(&error);

    // Now try to render, it should fail because there are no valid layers
    blot_screen *screen = blot_figure_render(fig, BLOT_RENDER_NONE, &error);
    ASSERT_TRUE(screen == NULL);
    ASSERT_TRUE(error != NULL);
    g_clear_error(&error);

    blot_figure_delete(fig);
}
