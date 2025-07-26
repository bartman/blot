#include <gtest/gtest.h>

#include "blot_terminal.h"
#include "blot_layer.h"
#include "blot_types.h"
#include "blot_canvas.h"
#include "blot_axis.h"

// used for combining multiple enum entries
template <typename T>
T combine(T a, T b) {
    return static_cast<T>(static_cast<int>(a) | static_cast<int>(b));
}

TEST(BaseTest, AssertionTrue)
{
    ASSERT_TRUE(true);
    ASSERT_FALSE(false);
}

TEST(Layer, alloc_scatter_int32)
{
    blot_plot_type plot_type = BLOT_SCATTER;
    blot_data_type data_type = BLOT_DATA_INT32;
    const constexpr size_t data_count = 3;
    const int data_xs[data_count] = { 1, 2, 3 };
    const int data_ys[data_count] = { 1, 2, 3 };
    blot_color data_color = 1;
    const char *data_label = "label";
    GError *error = NULL;

    blot_layer *layer = blot_layer_new(plot_type, data_type, data_count, data_xs, data_ys, data_color, data_label, &error);
    ASSERT_TRUE(layer != NULL);
    blot_layer_delete(layer);
    g_clear_error(&error);
}

TEST(Layer, alloc_zero_count)
{
    blot_plot_type plot_type = BLOT_SCATTER;
    blot_data_type data_type = BLOT_DATA_INT32;
    const constexpr size_t data_count = 0;
    const int data_xs[data_count] = {};
    const int data_ys[data_count] = {};
    blot_color data_color = 1;
    const char *data_label = "label";
    GError *error = NULL;

    blot_layer *layer = blot_layer_new(plot_type, data_type, data_count, data_xs, data_ys, data_color, data_label, &error);
    ASSERT_TRUE(layer == NULL);
    blot_layer_delete(layer);
    g_clear_error(&error);
}

TEST(Layer, alloc_NULL_xs)
{
    blot_plot_type plot_type = BLOT_SCATTER;
    blot_data_type data_type = BLOT_DATA_INT32;
    const constexpr size_t data_count = 3;
    const int *data_xs = NULL;
    const int data_ys[data_count] = {};
    blot_color data_color = 1;
    const char *data_label = "label";
    GError *error = NULL;

    /* It's OK for data_xs to be NULL */
    blot_layer *layer = blot_layer_new(plot_type, data_type, data_count, data_xs, data_ys, data_color, data_label, &error);
    ASSERT_FALSE(layer == NULL);
    blot_layer_delete(layer);
    g_clear_error(&error);
}

TEST(Layer, alloc_NULL_ys)
{
    blot_plot_type plot_type = BLOT_SCATTER;
    blot_data_type data_type = BLOT_DATA_INT32;
    const constexpr size_t data_count = 3;
    const int data_xs[data_count] = {};
    const int *data_ys = NULL;
    blot_color data_color = 1;
    const char *data_label = "label";
    GError *error = NULL;

    blot_layer *layer = blot_layer_new(plot_type, data_type, data_count, data_xs, data_ys, data_color, data_label, &error);
    ASSERT_TRUE(layer == NULL);
    blot_layer_delete(layer);
    g_clear_error(&error);
}

TEST(Layer, alloc_NULL_label)
{
    blot_plot_type plot_type = BLOT_SCATTER;
    blot_data_type data_type = BLOT_DATA_INT32;
    const constexpr size_t data_count = 3;
    const int data_xs[data_count] = {};
    const int data_ys[data_count] = {};
    blot_color data_color = 1;
    const char *data_label = NULL;
    GError *error = NULL;

    blot_layer *layer = blot_layer_new(plot_type, data_type, data_count, data_xs, data_ys, data_color, data_label, &error);
    ASSERT_TRUE(layer != NULL);
    blot_layer_delete(layer);
    g_clear_error(&error);
}

TEST(Layer, get_lim_int32)
{
    blot_plot_type plot_type = BLOT_SCATTER;
    blot_data_type data_type = BLOT_DATA_INT32;
    const constexpr size_t data_count = 9;
    const int data_xs[data_count] = { 1,-2,3,4,5,6,7,18,9 };
    const int data_ys[data_count] = { 1,12,3,4,5,6,7,-8,9 };
    blot_color data_color = 1;
    const char *data_label = "label";
    GError *error = NULL;

    blot_layer *layer = blot_layer_new(plot_type, data_type, data_count, data_xs, data_ys, data_color, data_label, &error);
    ASSERT_TRUE(layer != NULL);

    blot_xy_limits limits = {};
    ASSERT_TRUE(blot_layer_get_lim(layer, &limits, &error));

    ASSERT_FLOAT_EQ(limits.x_min, -2);
    ASSERT_FLOAT_EQ(limits.x_max, 18);
    ASSERT_FLOAT_EQ(limits.y_min, -8);
    ASSERT_FLOAT_EQ(limits.y_max, 12);

    blot_layer_delete(layer);
    g_clear_error(&error);
}

TEST(Layer, get_lim_float)
{
    blot_plot_type plot_type = BLOT_SCATTER;
    blot_data_type data_type = BLOT_DATA_FLOAT;
    const constexpr size_t data_count = 10;
    const float data_xs[data_count] = { 1,0,-2,3,4,5,6,7,18,9 };
    const float data_ys[data_count] = { 1,12,3,4,5,6,7,-8,0,9 };
    blot_color data_color = 1;
    const char *data_label = "label";
    GError *error = NULL;

    blot_layer *layer = blot_layer_new(plot_type, data_type, data_count, data_xs, data_ys, data_color, data_label, &error);
    ASSERT_TRUE(layer != NULL);

    blot_xy_limits limits = {};
    ASSERT_TRUE(blot_layer_get_lim(layer, &limits, &error));

    ASSERT_FLOAT_EQ(limits.x_min, -2);
    ASSERT_FLOAT_EQ(limits.x_max, 18);
    ASSERT_FLOAT_EQ(limits.y_min, -8);
    ASSERT_FLOAT_EQ(limits.y_max, 12);

    blot_layer_delete(layer);
    g_clear_error(&error);
}

TEST(Layer, get_x_y_double)
{
    blot_plot_type plot_type = BLOT_SCATTER;
    blot_data_type data_type = BLOT_DATA_DOUBLE;
    const constexpr size_t data_count = 2;
    const double data_xs[data_count] = { 1.1, 2.2 };
    const double data_ys[data_count] = { 3.3, 4.4 };
    blot_color data_color = 1;
    const char *data_label = "label";
    GError *error = NULL;

    blot_layer *layer = blot_layer_new(plot_type, data_type, data_count, data_xs, data_ys, data_color, data_label, &error);
    ASSERT_TRUE(layer != NULL);
    ASSERT_TRUE(error == NULL);

    double x, y;
    ASSERT_TRUE(blot_layer_get_x_y(layer, 0, &x, &y, &error));
    ASSERT_TRUE(error == NULL);
    ASSERT_DOUBLE_EQ(x, 1.1);
    ASSERT_DOUBLE_EQ(y, 3.3);

    ASSERT_TRUE(blot_layer_get_x_y(layer, 1, &x, &y, &error));
    ASSERT_TRUE(error == NULL);
    ASSERT_DOUBLE_EQ(x, 2.2);
    ASSERT_DOUBLE_EQ(y, 4.4);

    blot_layer_delete(layer);
    g_clear_error(&error);
}

TEST(Layer, get_x_null_xs)
{
    const constexpr size_t data_count = 3;
    const int data_ys[data_count] = { 10, 20, 30 };

    blot_layer layer = {
        .plot_type = BLOT_SCATTER,
        .data_type = BLOT_DATA_INT32,
        .count = data_count,
        .xs = NULL,
        .ys = data_ys,
        .color = 1,
        .label = "label"
    };

    GError *error = NULL;
    double x, y;

    ASSERT_TRUE(blot_layer_get_x_y(&layer, 0, &x, &y, &error));
    ASSERT_TRUE(error == NULL);
    ASSERT_FLOAT_EQ(x, 0);
    ASSERT_FLOAT_EQ(y, 10);

    ASSERT_TRUE(blot_layer_get_x_y(&layer, 1, &x, &y, &error));
    ASSERT_TRUE(error == NULL);
    ASSERT_FLOAT_EQ(x, 1);
    ASSERT_FLOAT_EQ(y, 20);

    ASSERT_TRUE(blot_layer_get_x_y(&layer, 2, &x, &y, &error));
    ASSERT_TRUE(error == NULL);
    ASSERT_FLOAT_EQ(x, 2);
    ASSERT_FLOAT_EQ(y, 30);

    g_clear_error(&error);
}

TEST(Layer, get_x_y_out_of_bounds)
{
    blot_plot_type plot_type = BLOT_SCATTER;
    blot_data_type data_type = BLOT_DATA_INT32;
    const constexpr size_t data_count = 3;
    const int data_xs[data_count] = { 1, 2, 3 };
    const int data_ys[data_count] = { 4, 5, 6 };
    blot_color data_color = 1;
    const char *data_label = "label";
    GError *error = NULL;

    blot_layer *layer = blot_layer_new(plot_type, data_type, data_count, data_xs, data_ys, data_color, data_label, &error);
    ASSERT_TRUE(layer != NULL);
    ASSERT_TRUE(error == NULL);

    double x, y;
    // Test with index > count, which is out of bounds.
    ASSERT_FALSE(blot_layer_get_x_y(layer, data_count + 1, &x, &y, &error));
    ASSERT_TRUE(error != NULL);
    ASSERT_EQ(error->code, ERANGE);
    g_clear_error(&error);

    blot_layer_delete(layer);
}

TEST(Layer, get_lim_null_xs)
{
    const constexpr size_t data_count = 3;
    const int data_ys[data_count] = { 10, -5, 20 };

    blot_layer layer = {
        .plot_type = BLOT_SCATTER,
        .data_type = BLOT_DATA_INT32,
        .count = data_count,
        .xs = NULL,
        .ys = data_ys,
        .color = 1,
        .label = "label"
    };

    GError *error = NULL;
    blot_xy_limits limits = {};
    ASSERT_TRUE(blot_layer_get_lim(&layer, &limits, &error));
    ASSERT_TRUE(error == NULL);

    ASSERT_FLOAT_EQ(limits.x_min, 0);
    ASSERT_FLOAT_EQ(limits.x_max, 2);
    ASSERT_FLOAT_EQ(limits.y_min, -5);
    ASSERT_FLOAT_EQ(limits.y_max, 20);

    g_clear_error(&error);
}

TEST(Layer, get_x_y_int16)
{
    blot_plot_type plot_type = BLOT_SCATTER;
    blot_data_type data_type = BLOT_DATA_INT16;
    const constexpr size_t data_count = 2;
    const gint16 data_xs[data_count] = { 10, 20 };
    const gint16 data_ys[data_count] = { 30, 40 };
    blot_color data_color = 1;
    const char *data_label = "label";
    GError *error = NULL;

    blot_layer *layer = blot_layer_new(plot_type, data_type, data_count, data_xs, data_ys, data_color, data_label, &error);
    ASSERT_TRUE(layer != NULL);
    ASSERT_TRUE(error == NULL);

    double x, y;
    ASSERT_TRUE(blot_layer_get_x_y(layer, 0, &x, &y, &error));
    ASSERT_TRUE(error == NULL);
    ASSERT_DOUBLE_EQ(x, 10);
    ASSERT_DOUBLE_EQ(y, 30);

    ASSERT_TRUE(blot_layer_get_x_y(layer, 1, &x, &y, &error));
    ASSERT_TRUE(error == NULL);
    ASSERT_DOUBLE_EQ(x, 20);
    ASSERT_DOUBLE_EQ(y, 40);

    blot_layer_delete(layer);
    g_clear_error(&error);
}

TEST(Layer, get_x_y_int32)
{
    blot_plot_type plot_type = BLOT_SCATTER;
    blot_data_type data_type = BLOT_DATA_INT32;
    const constexpr size_t data_count = 2;
    const gint32 data_xs[data_count] = { 100, 200 };
    const gint32 data_ys[data_count] = { 300, 400 };
    blot_color data_color = 1;
    const char *data_label = "label";
    GError *error = NULL;

    blot_layer *layer = blot_layer_new(plot_type, data_type, data_count, data_xs, data_ys, data_color, data_label, &error);
    ASSERT_TRUE(layer != NULL);
    ASSERT_TRUE(error == NULL);

    double x, y;
    ASSERT_TRUE(blot_layer_get_x_y(layer, 0, &x, &y, &error));
    ASSERT_TRUE(error == NULL);
    ASSERT_DOUBLE_EQ(x, 100);
    ASSERT_DOUBLE_EQ(y, 300);

    ASSERT_TRUE(blot_layer_get_x_y(layer, 1, &x, &y, &error));
    ASSERT_TRUE(error == NULL);
    ASSERT_DOUBLE_EQ(x, 200);
    ASSERT_DOUBLE_EQ(y, 400);

    blot_layer_delete(layer);
    g_clear_error(&error);
}

TEST(Layer, get_x_y_int64)
{
    blot_plot_type plot_type = BLOT_SCATTER;
    blot_data_type data_type = BLOT_DATA_INT64;
    const constexpr size_t data_count = 2;
    const gint64 data_xs[data_count] = { 1000, 2000 };
    const gint64 data_ys[data_count] = { 3000, 4000 };
    blot_color data_color = 1;
    const char *data_label = "label";
    GError *error = NULL;

    blot_layer *layer = blot_layer_new(plot_type, data_type, data_count, data_xs, data_ys, data_color, data_label, &error);
    ASSERT_TRUE(layer != NULL);
    ASSERT_TRUE(error == NULL);

    double x, y;
    ASSERT_TRUE(blot_layer_get_x_y(layer, 0, &x, &y, &error));
    ASSERT_TRUE(error == NULL);
    ASSERT_DOUBLE_EQ(x, 1000);
    ASSERT_DOUBLE_EQ(y, 3000);

    ASSERT_TRUE(blot_layer_get_x_y(layer, 1, &x, &y, &error));
    ASSERT_TRUE(error == NULL);
    ASSERT_DOUBLE_EQ(x, 2000);
    ASSERT_DOUBLE_EQ(y, 4000);

    blot_layer_delete(layer);
    g_clear_error(&error);
}

TEST(Layer, get_x_y_float)
{
    blot_plot_type plot_type = BLOT_SCATTER;
    blot_data_type data_type = BLOT_DATA_FLOAT;
    const constexpr size_t data_count = 2;
    const float data_xs[data_count] = { 1.5f, 2.5f };
    const float data_ys[data_count] = { 3.5f, 4.5f };
    blot_color data_color = 1;
    const char *data_label = "label";
    GError *error = NULL;

    blot_layer *layer = blot_layer_new(plot_type, data_type, data_count, data_xs, data_ys, data_color, data_label, &error);
    ASSERT_TRUE(layer != NULL);
    ASSERT_TRUE(error == NULL);

    double x, y;
    ASSERT_TRUE(blot_layer_get_x_y(layer, 0, &x, &y, &error));
    ASSERT_TRUE(error == NULL);
    ASSERT_FLOAT_EQ(x, 1.5f);
    ASSERT_FLOAT_EQ(y, 3.5f);

    ASSERT_TRUE(blot_layer_get_x_y(layer, 1, &x, &y, &error));
    ASSERT_TRUE(error == NULL);
    ASSERT_FLOAT_EQ(x, 2.5f);
    ASSERT_FLOAT_EQ(y, 4.5f);

    blot_layer_delete(layer);
    g_clear_error(&error);
}

TEST(Layer, get_x_y_null_y)
{
    blot_plot_type plot_type = BLOT_SCATTER;
    blot_data_type data_type = BLOT_DATA_INT32;
    const constexpr size_t data_count = 3;
    const int data_xs[data_count] = { 1, 2, 3 };
    const int *data_ys = NULL;
    blot_color data_color = 1;
    const char *data_label = "label";
    GError *error = NULL;

    blot_layer *layer = blot_layer_new(plot_type, data_type, data_count, data_xs, data_ys, data_color, data_label, &error);
    ASSERT_TRUE(layer == NULL);
    ASSERT_TRUE(error != NULL);
    ASSERT_EQ(error->code, EFAULT);
    g_clear_error(&error);
}

TEST(Layer, get_lim_int16)
{
    blot_plot_type plot_type = BLOT_SCATTER;
    blot_data_type data_type = BLOT_DATA_INT16;
    const constexpr size_t data_count = 5;
    const gint16 data_xs[data_count] = { 10, -5, 20, 0, 15 };
    const gint16 data_ys[data_count] = { 100, 50, 150, 75, 125 };
    blot_color data_color = 1;
    const char *data_label = "label";
    GError *error = NULL;

    blot_layer *layer = blot_layer_new(plot_type, data_type, data_count, data_xs, data_ys, data_color, data_label, &error);
    ASSERT_TRUE(layer != NULL);

    blot_xy_limits limits = {};
    ASSERT_TRUE(blot_layer_get_lim(layer, &limits, &error));

    ASSERT_FLOAT_EQ(limits.x_min, -5);
    ASSERT_FLOAT_EQ(limits.x_max, 20);
    ASSERT_FLOAT_EQ(limits.y_min, 50);
    ASSERT_FLOAT_EQ(limits.y_max, 150);

    blot_layer_delete(layer);
    g_clear_error(&error);
}

TEST(Layer, get_lim_int64)
{
    blot_plot_type plot_type = BLOT_SCATTER;
    blot_data_type data_type = BLOT_DATA_INT64;
    const constexpr size_t data_count = 5;
    const gint64 data_xs[data_count] = { 1000, -500, 2000, 0, 1500 };
    const gint64 data_ys[data_count] = { 10000, 5000, 15000, 7500, 12500 };
    blot_color data_color = 1;
    const char *data_label = "label";
    GError *error = NULL;

    blot_layer *layer = blot_layer_new(plot_type, data_type, data_count, data_xs, data_ys, data_color, data_label, &error);
    ASSERT_TRUE(layer != NULL);

    blot_xy_limits limits = {};
    ASSERT_TRUE(blot_layer_get_lim(layer, &limits, &error));

    ASSERT_FLOAT_EQ(limits.x_min, -500);
    ASSERT_FLOAT_EQ(limits.x_max, 2000);
    ASSERT_FLOAT_EQ(limits.y_min, 5000);
    ASSERT_FLOAT_EQ(limits.y_max, 15000);

    blot_layer_delete(layer);
    g_clear_error(&error);
}

TEST(Layer, get_lim_double)
{
    blot_plot_type plot_type = BLOT_SCATTER;
    blot_data_type data_type = BLOT_DATA_DOUBLE;
    const constexpr size_t data_count = 5;
    const double data_xs[data_count] = { 10.5, -5.5, 20.5, 0.5, 15.5 };
    const double data_ys[data_count] = { 100.5, 50.5, 150.5, 75.5, 125.5 };
    blot_color data_color = 1;
    const char *data_label = "label";
    GError *error = NULL;

    blot_layer *layer = blot_layer_new(plot_type, data_type, data_count, data_xs, data_ys, data_color, data_label, &error);
    ASSERT_TRUE(layer != NULL);

    blot_xy_limits limits = {};
    ASSERT_TRUE(blot_layer_get_lim(layer, &limits, &error));

    ASSERT_FLOAT_EQ(limits.x_min, -5.5);
    ASSERT_FLOAT_EQ(limits.x_max, 20.5);
    ASSERT_FLOAT_EQ(limits.y_min, 50.5);
    ASSERT_FLOAT_EQ(limits.y_max, 150.5);

    blot_layer_delete(layer);
    g_clear_error(&error);
}

TEST(Layer, get_lim_single_point)
{
    blot_plot_type plot_type = BLOT_SCATTER;
    blot_data_type data_type = BLOT_DATA_INT32;
    const constexpr size_t data_count = 1;
    const int data_xs[data_count] = { 10 };
    const int data_ys[data_count] = { 20 };
    blot_color data_color = 1;
    const char *data_label = "label";
    GError *error = NULL;

    blot_layer *layer = blot_layer_new(plot_type, data_type, data_count, data_xs, data_ys, data_color, data_label, &error);
    ASSERT_TRUE(layer != NULL);

    blot_xy_limits limits = {};
    ASSERT_TRUE(blot_layer_get_lim(layer, &limits, &error));

    ASSERT_FLOAT_EQ(limits.x_min, 10);
    ASSERT_FLOAT_EQ(limits.x_max, 10);
    ASSERT_FLOAT_EQ(limits.y_min, 20);
    ASSERT_FLOAT_EQ(limits.y_max, 20);

    blot_layer_delete(layer);
    g_clear_error(&error);
}

TEST(Layer, get_lim_all_same_values)
{
    blot_plot_type plot_type = BLOT_SCATTER;
    blot_data_type data_type = BLOT_DATA_INT32;
    const constexpr size_t data_count = 5;
    const int data_xs[data_count] = { 10, 10, 10, 10, 10 };
    const int data_ys[data_count] = { 20, 20, 20, 20, 20 };
    blot_color data_color = 1;
    const char *data_label = "label";
    GError *error = NULL;

    blot_layer *layer = blot_layer_new(plot_type, data_type, data_count, data_xs, data_ys, data_color, data_label, &error);
    ASSERT_TRUE(layer != NULL);

    blot_xy_limits limits = {};
    ASSERT_TRUE(blot_layer_get_lim(layer, &limits, &error));

    ASSERT_FLOAT_EQ(limits.x_min, 10);
    ASSERT_FLOAT_EQ(limits.x_max, 10);
    ASSERT_FLOAT_EQ(limits.y_min, 20);
    ASSERT_FLOAT_EQ(limits.y_max, 20);

    blot_layer_delete(layer);
    g_clear_error(&error);
}

TEST(Layer, get_lim_negative_values)
{
    blot_plot_type plot_type = BLOT_SCATTER;
    blot_data_type data_type = BLOT_DATA_INT32;
    const constexpr size_t data_count = 5;
    const int data_xs[data_count] = { -10, -5, -20, -1, -15 };
    const int data_ys[data_count] = { -100, -50, -150, -75, -125 };
    blot_color data_color = 1;
    const char *data_label = "label";
    GError *error = NULL;

    blot_layer *layer = blot_layer_new(plot_type, data_type, data_count, data_xs, data_ys, data_color, data_label, &error);
    ASSERT_TRUE(layer != NULL);

    blot_xy_limits limits = {};
    ASSERT_TRUE(blot_layer_get_lim(layer, &limits, &error));

    ASSERT_FLOAT_EQ(limits.x_min, -20);
    ASSERT_FLOAT_EQ(limits.x_max, -1);
    ASSERT_FLOAT_EQ(limits.y_min, -150);
    ASSERT_FLOAT_EQ(limits.y_max, -50);

    blot_layer_delete(layer);
    g_clear_error(&error);
}

TEST(Layer, render_scatter_int32)
{
    GError *error = NULL;

    // fake size for testing
    ASSERT_TRUE(blot_terminal_set_size({100,100}, &error));

    blot_plot_type plot_type = BLOT_SCATTER;
    blot_data_type data_type = BLOT_DATA_INT32;
    const constexpr size_t data_count = 3;
    const int data_xs[data_count] = { 0, 5, 9 };
    const int data_ys[data_count] = { 0, 5, 9 };
    blot_color data_color = 1;
    const char *data_label = "label";

    blot_layer *layer = blot_layer_new(plot_type, data_type, data_count, data_xs, data_ys, data_color, data_label, &error);
    ASSERT_TRUE(layer != NULL);

    blot_xy_limits lim = { 0, 9, 0, 9 };
    blot_dimensions dim = { 10, 10 };
    blot_render_flags flags = BLOT_RENDER_NONE;

    blot_canvas *canvas = blot_layer_render(layer, &lim, &dim, flags, &error);
    ASSERT_TRUE(canvas != NULL);
    ASSERT_TRUE(error == NULL);

    ASSERT_EQ(blot_canvas_get_cell(canvas, 0, 0), L'•');
    ASSERT_EQ(blot_canvas_get_cell(canvas, 5, 5), L'•');
    ASSERT_EQ(blot_canvas_get_cell(canvas, 9, 9), L'•');

    blot_canvas_delete(canvas);
    blot_layer_delete(layer);
}

TEST(Layer, render_line_float)
{
    GError *error = NULL;

    // fake size for testing
    ASSERT_TRUE(blot_terminal_set_size({100,100}, &error));

    blot_plot_type plot_type = BLOT_LINE;
    blot_data_type data_type = BLOT_DATA_FLOAT;
    const constexpr size_t data_count = 2;
    const float data_xs[data_count] = { 0.0f, 9.0f };
    const float data_ys[data_count] = { 0.0f, 9.0f };
    blot_color data_color = 1;
    const char *data_label = "label";

    blot_layer *layer = blot_layer_new(plot_type, data_type, data_count, data_xs, data_ys, data_color, data_label, &error);
    ASSERT_TRUE(layer != NULL);

    blot_xy_limits lim = { 0, 9, 0, 9 };
    blot_dimensions dim = { 10, 10 };
    blot_render_flags flags = BLOT_RENDER_NONE;

    blot_canvas *canvas = blot_layer_render(layer, &lim, &dim, flags, &error);
    ASSERT_TRUE(canvas != NULL);
    ASSERT_TRUE(error == NULL);

    ASSERT_EQ(blot_canvas_get_cell(canvas, 0, 0), L'•');
    ASSERT_EQ(blot_canvas_get_cell(canvas, 5, 5), L'•');
    ASSERT_EQ(blot_canvas_get_cell(canvas, 9, 9), L'•');

    blot_canvas_delete(canvas);
    blot_layer_delete(layer);
}

TEST(Layer, render_bar_double)
{
    GError *error = NULL;

    // fake size for testing
    ASSERT_TRUE(blot_terminal_set_size({100,100}, &error));

    blot_plot_type plot_type = BLOT_BAR;
    blot_data_type data_type = BLOT_DATA_DOUBLE;
    const constexpr size_t data_count = 1;
    const double data_xs[data_count] = { 5.0 };
    const double data_ys[data_count] = { 5.0 };
    blot_color data_color = 1;
    const char *data_label = "label";

    blot_layer *layer = blot_layer_new(plot_type, data_type, data_count, data_xs, data_ys, data_color, data_label, &error);
    ASSERT_TRUE(layer != NULL);

    blot_xy_limits lim = { 0, 9, 0, 9 };
    blot_dimensions dim = { 10, 10 };
    blot_render_flags flags = BLOT_RENDER_NONE;

    blot_canvas *canvas = blot_layer_render(layer, &lim, &dim, flags, &error);
    ASSERT_TRUE(canvas != NULL);
    ASSERT_TRUE(error == NULL);

    // For a bar chart, it fills a rectangle from x to x+width and 0 to y
    // Given x=5, y=5, dim.cols=10, dim.rows=10, it should fill from (5,0) to (6,5)
    // The plot character is L'•'
    ASSERT_EQ(blot_canvas_get_cell(canvas, 5, 0), L'•');
    ASSERT_EQ(blot_canvas_get_cell(canvas, 5, 1), L'•');
    ASSERT_EQ(blot_canvas_get_cell(canvas, 5, 2), L'•');
    ASSERT_EQ(blot_canvas_get_cell(canvas, 5, 3), L'•');
    ASSERT_EQ(blot_canvas_get_cell(canvas, 5, 4), L'•');
    ASSERT_EQ(blot_canvas_get_cell(canvas, 5, 5), L'•');
    ASSERT_EQ(blot_canvas_get_cell(canvas, 6, 0), L'•');
    ASSERT_EQ(blot_canvas_get_cell(canvas, 6, 1), L'•');
    ASSERT_EQ(blot_canvas_get_cell(canvas, 6, 2), L'•');
    ASSERT_EQ(blot_canvas_get_cell(canvas, 6, 3), L'•');
    ASSERT_EQ(blot_canvas_get_cell(canvas, 6, 4), L'•');
    ASSERT_EQ(blot_canvas_get_cell(canvas, 6, 5), L'•');

    blot_canvas_delete(canvas);
    blot_layer_delete(layer);
}

TEST(Layer, render_with_braille_flag)
{
    GError *error = NULL;

    // fake size for testing
    ASSERT_TRUE(blot_terminal_set_size({100,100}, &error));

    blot_plot_type plot_type = BLOT_SCATTER;
    blot_data_type data_type = BLOT_DATA_INT32;
    const constexpr size_t data_count = 1;
    const int data_xs[data_count] = { 0 };
    const int data_ys[data_count] = { 0 };
    blot_color data_color = 1;
    const char *data_label = "label";

    blot_layer *layer = blot_layer_new(plot_type, data_type, data_count, data_xs, data_ys, data_color, data_label, &error);
    ASSERT_TRUE(layer != NULL);

    blot_xy_limits lim = { 0, 9, 0, 9 };
    blot_dimensions dim = { 10, 10 };
    blot_render_flags flags = combine(BLOT_RENDER_BRAILLE,
                                      BLOT_RENDER_DONT_INVERT_Y_AXIS);

    blot_canvas *canvas = blot_layer_render(layer, &lim, &dim, flags, &error);
    ASSERT_TRUE(canvas != NULL);
    ASSERT_TRUE(error == NULL);

    // When BLOT_RENDER_BRAILLE is set, the canvas uses braille characters
    // A single point at (0,0) should result in a braille character at (0,0) cell
    // The braille character for a single dot at (0,0) is L'⠁'
    ASSERT_EQ(blot_canvas_get_cell(canvas, 0, 0), L'⠁');

    blot_canvas_delete(canvas);
    blot_layer_delete(layer);
}

TEST(Layer, render_with_no_unicode_flag)
{
    GError *error = NULL;

    // fake size for testing
    ASSERT_TRUE(blot_terminal_set_size({100,100}, &error));

    blot_plot_type plot_type = BLOT_SCATTER;
    blot_data_type data_type = BLOT_DATA_INT32;
    const constexpr size_t data_count = 1;
    const int data_xs[data_count] = { 0 };
    const int data_ys[data_count] = { 0 };
    blot_color data_color = 1;
    const char *data_label = "label";

    blot_layer *layer = blot_layer_new(plot_type, data_type, data_count, data_xs, data_ys, data_color, data_label, &error);
    ASSERT_TRUE(layer != NULL);

    blot_xy_limits lim = { 0, 9, 0, 9 };
    blot_dimensions dim = { 10, 10 };
    blot_render_flags flags = BLOT_RENDER_NO_UNICODE;

    blot_canvas *canvas = blot_layer_render(layer, &lim, &dim, flags, &error);
    ASSERT_TRUE(canvas != NULL);
    ASSERT_TRUE(error == NULL);

    // When BLOT_RENDER_NO_UNICODE is set, the plot character should be L'*'
    ASSERT_EQ(blot_canvas_get_cell(canvas, 0, 0), L'*');

    blot_canvas_delete(canvas);
    blot_layer_delete(layer);
}

TEST(Layer, render_error_conditions)
{
    GError *error = NULL;

    // fake size for testing
    ASSERT_TRUE(blot_terminal_set_size({100,100}, &error));

    blot_plot_type plot_type = BLOT_SCATTER;
    blot_data_type data_type = BLOT_DATA_INT32;
    const constexpr size_t data_count = 3;
    const int data_xs[data_count] = { 0, 5, 9 };
    const int data_ys[data_count] = { 0, 5, 9 };
    blot_color data_color = 1;
    const char *data_label = "label";

    blot_layer *layer = blot_layer_new(plot_type, data_type, data_count, data_xs, data_ys, data_color, data_label, &error);
    ASSERT_TRUE(layer != NULL);

    blot_xy_limits lim = { 0, 9, 0, 9 };
    blot_dimensions dim = { 10, 10 };
    blot_render_flags flags = BLOT_RENDER_NONE;

    // Test with NULL layer
    blot_canvas *canvas = blot_layer_render(NULL, &lim, &dim, flags, &error);
    ASSERT_TRUE(canvas == NULL);
    ASSERT_TRUE(error != NULL);
    ASSERT_EQ(error->code, EFAULT);
    g_clear_error(&error);

    // Test with invalid limits (e.g., x_min >= x_max)
    lim = { 9, 0, 0, 9 }; // Invalid x_limits
    canvas = blot_layer_render(layer, &lim, &dim, flags, &error);
    ASSERT_TRUE(canvas == NULL);
    ASSERT_TRUE(error != NULL);
    ASSERT_EQ(error->code, ERANGE);
    g_clear_error(&error);

    // Test with invalid dimensions (e.g., cols = 0)
    lim = { 0, 9, 0, 9 }; // Reset limits
    dim = { 0, 10 }; // Invalid dimensions
    canvas = blot_layer_render(layer, &lim, &dim, flags, &error);
    ASSERT_TRUE(canvas == NULL);
    ASSERT_TRUE(error != NULL);
    ASSERT_EQ(error->code, EINVAL);
    g_clear_error(&error);

    blot_layer_delete(layer);
}
