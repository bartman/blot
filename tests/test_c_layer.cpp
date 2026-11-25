#include "blot_test.hpp"

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

/* --- layer to canvas: exact cells (#2, group L) ------------------------
 *
 * blot_layer_render() and the exact set of cells it sets.  Unless stated:
 * limits 0..9 on both axes, a 10x10 canvas, flags NO_UNICODE. */

#include "blot_test.hpp"
#include <cfloat>
#include <random>

using namespace blot_test;

namespace {

const blot_xy_limits LIM09 = { 0, 9, 0, 9 };
const blot_dimensions DIM10 = { 10, 10 };

/* render a layer; the canvas is NULL (and the test failed) on error */
blot_canvas *layer_canvas(blot_plot_type type, blot_data_type dt, size_t n,
                          const void *xs, const void *ys,
                          blot_xy_limits lim = LIM09, blot_dimensions dim = DIM10,
                          blot_render_flags flags = BLOT_RENDER_NO_UNICODE)
{
    GError *error = NULL;
    blot_layer *lay = blot_layer_new(type, dt, n, xs, ys, 1, "l", &error);
    EXPECT_TRUE(lay != NULL);
    if (!lay) {
        g_clear_error(&error);
        return NULL;
    }
    blot_canvas *can = blot_layer_render(lay, &lim, &dim, flags, &error);
    EXPECT_TRUE(can != NULL) << (error ? error->message : "");
    g_clear_error(&error);
    blot_layer_delete(lay);
    return can;
}

Cells layer_cells(blot_plot_type type, blot_data_type dt, size_t n,
                  const void *xs, const void *ys,
                  blot_xy_limits lim = LIM09, blot_dimensions dim = DIM10,
                  blot_render_flags flags = BLOT_RENDER_NO_UNICODE)
{
    Canvas can(layer_canvas(type, dt, n, xs, ys, lim, dim, flags));
    return can.can ? cells(can) : Cells();
}

/* values stored as one of the 5 data types: 0..4 are INT16, INT32,
 * INT64, FLOAT, DOUBLE, the BLOT_DATA_X_* values */
std::vector<unsigned char> as_type(unsigned t, const std::vector<double> &vals)
{
    static const size_t sizes[] = { 2, 4, 8, 4, 8 };
    std::vector<unsigned char> out(sizes[t] * vals.size());
    for (size_t i = 0; i < vals.size(); i++) {
        unsigned char *p = &out[i * sizes[t]];
        switch (t) {
        case 0: { gint16 v = vals[i]; memcpy(p, &v, sizeof(v)); break; }
        case 1: { gint32 v = vals[i]; memcpy(p, &v, sizeof(v)); break; }
        case 2: { gint64 v = vals[i]; memcpy(p, &v, sizeof(v)); break; }
        case 3: { float  v = vals[i]; memcpy(p, &v, sizeof(v)); break; }
        case 4: { double v = vals[i]; memcpy(p, &v, sizeof(v)); break; }
        }
    }
    return out;
}

const char *type_name(unsigned t)
{
    static const char *names[] = { "INT16", "INT32", "INT64", "FLOAT", "DOUBLE" };
    return names[t];
}

/* the cells of every (x type, y type) pair for {0,5,9} x {0,5,9} */
void expect_all_type_combinations(blot_plot_type type, const Cells &expected)
{
    const std::vector<double> vals = { 0, 5, 9 };
    for (unsigned xt = 0; xt < 5; xt++) {
        for (unsigned yt = 0; yt < 5; yt++) {
            SCOPED_TRACE(std::string("x ") + type_name(xt) + ", y " + type_name(yt));
            auto xs = as_type(xt, vals), ys = as_type(yt, vals);
            blot_data_type dt = BLOT_DATA_TYPE(xt, yt << 4);
            EXPECT_CELLS_EQ(layer_cells(type, dt, 3, xs.data(), ys.data()), expected);
        }
    }
}

Cells diagonal(unsigned n)
{
    Cells out;
    for (unsigned i = 0; i < n; i++)
        out.insert({i, i});
    return out;
}

Cells rect(unsigned c0, unsigned c1, unsigned r0, unsigned r1)
{
    Cells out;
    for (unsigned c = c0; c <= c1; c++)
        for (unsigned r = r0; r <= r1; r++)
            out.insert({c, r});
    return out;
}

Cells operator+(Cells a, const Cells &b)
{
    a.insert(b.begin(), b.end());
    return a;
}

} // namespace

TEST(Layer, scatter_exact_cells)
{
    const gint32 xs[] = { 0, 9, 5, 3, 9 }, ys[] = { 0, 9, 5, 7, 0 };
    EXPECT_CELLS_EQ(layer_cells(BLOT_SCATTER, BLOT_DATA_INT32, 5, xs, ys),
                    Cells({ {0,0}, {9,9}, {5,5}, {3,7}, {9,0} }));
}

TEST(Layer, scatter_all_type_combinations)
{
    /* BLOT_DATA_INT64 goes through blot_layer_scatter_int64() */
    expect_all_type_combinations(BLOT_SCATTER, { {0,0}, {5,5}, {9,9} });
}

TEST(Layer, line_all_type_combinations)
{
    expect_all_type_combinations(BLOT_LINE, diagonal(10));
}

TEST(Layer, bar_all_type_combinations)
{
    /* the bar picture of Golden.no_axes_three_plot_types */
    expect_all_type_combinations(BLOT_BAR, rect(0, 1, 0, 0) + rect(5, 6, 0, 5) + rect(9, 9, 0, 9));
}

TEST(Layer, null_xs_uses_index)
{
    const gint16 ys16[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
    EXPECT_CELLS_EQ(layer_cells(BLOT_SCATTER, BLOT_DATA_INT16, 10, NULL, ys16), diagonal(10));

    const gint64 ys64[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
    EXPECT_CELLS_EQ(layer_cells(BLOT_SCATTER, BLOT_DATA_INT64, 10, NULL, ys64), diagonal(10));

    const float ysf[] = { 0, 3, 6, 9 };
    EXPECT_CELLS_EQ(layer_cells(BLOT_LINE, BLOT_DATA_FLOAT, 4, NULL, ysf, { 0, 3, 0, 9 }), diagonal(10));
}

TEST(Layer, line_integer_extremes)
{
    const blot_dimensions dim = { 9, 9 };
    {
        const gint16 v[] = { G_MININT16, 0, G_MAXINT16 };
        EXPECT_CELLS_EQ(layer_cells(BLOT_LINE, BLOT_DATA_INT16, 3, v, v,
                                    { G_MININT16, G_MAXINT16, G_MININT16, G_MAXINT16 }, dim),
                        diagonal(9));
    }
    {
        const gint32 v[] = { G_MININT32, 0, G_MAXINT32 };
        EXPECT_CELLS_EQ(layer_cells(BLOT_LINE, BLOT_DATA_INT32, 3, v, v,
                                    { G_MININT32, G_MAXINT32, G_MININT32, G_MAXINT32 }, dim),
                        diagonal(9));
    }
    {
        const gint64 v[] = { G_MININT64, 0, G_MAXINT64 };
        EXPECT_CELLS_EQ(layer_cells(BLOT_LINE, BLOT_DATA_INT64, 3, v, v,
                                    { (double)G_MININT64, (double)G_MAXINT64,
                                      (double)G_MININT64, (double)G_MAXINT64 }, dim),
                        diagonal(9));
    }
}

TEST(Layer, scatter_out_of_range_clipped)
{
    const gint32 xs[] = { -1, 5, 10, 5, 100, -100, 5 };
    const gint32 ys[] = { 5, -1, 5, 10, 100, -100, 5 };
    EXPECT_CELLS_EQ(layer_cells(BLOT_SCATTER, BLOT_DATA_INT32, 7, xs, ys), Cells({ {5,5} }));
}

TEST(Layer, scatter_far_out_of_range_not_wrapped)
{
    /* (2^32 + 5) / 6: far above the limit, the cell index wraps to row 5 */
    const double xs[] = { 0.5 }, ys[] = { 715827883.5 };
    EXPECT_CELLS_EQ(layer_cells(BLOT_SCATTER, BLOT_DATA_DOUBLE, 1, xs, ys, { 0, 1, 0, 1 }, { 12, 12 }),
                    Cells());
}

TEST(Layer, scatter_fractional_range)
{
    const double xs[] = { 0, 1 }, ys[] = { 0, 1 };
    EXPECT_CELLS_EQ(layer_cells(BLOT_SCATTER, BLOT_DATA_DOUBLE, 2, xs, ys, { 0, 1, 0, 1 }),
                    Cells({ {0,0}, {9,9} }));
}

TEST(Layer, scatter_wide_range_keeps_max)
{
    const gint32 xs[] = { 0, 1000 }, ys[] = { 0, 1000 };
    EXPECT_CELLS_EQ(layer_cells(BLOT_SCATTER, BLOT_DATA_INT32, 2, xs, ys, { 0, 1000, 0, 1000 }, { 20, 10 }),
                    Cells({ {0,0}, {19,9} }));
}

TEST(Layer, bar_wide_range_keeps_max)
{
    const gint32 xs[] = { 1000 }, ys[] = { 1000 };
    Cells got = layer_cells(BLOT_BAR, BLOT_DATA_INT32, 1, xs, ys, { 0, 1000, 0, 1000 }, { 20, 10 });
    for (unsigned r = 0; r < 10; r++)
        EXPECT_TRUE(got.count({19, r})) << "row " << r << "; cells:" << cells_str(got);
}

/* #3: the same mapping checks for scatter_int64, and bar heights */
TEST(Layer, scatter_int64_fractional_range)
{
    const gint64 xs[] = { 0, 1 }, ys[] = { 0, 1 };
    EXPECT_CELLS_EQ(layer_cells(BLOT_SCATTER, BLOT_DATA_INT64, 2, xs, ys, { 0, 1, 0, 1 }),
                    Cells({ {0,0}, {9,9} }));
}

TEST(Layer, scatter_int64_wide_range_keeps_max)
{
    const gint64 xs[] = { 0, 1000 }, ys[] = { 0, 1000 };
    EXPECT_CELLS_EQ(layer_cells(BLOT_SCATTER, BLOT_DATA_INT64, 2, xs, ys, { 0, 1000, 0, 1000 }, { 20, 10 }),
                    Cells({ {0,0}, {19,9} }));
}

TEST(Layer, scatter_int64_far_out_of_range_not_wrapped)
{
    /* 6 * 715827883 is 2^32 + 2: far above the limit, the old cell index wrapped to row 2 */
    const gint64 xs[] = { 0 }, ys[] = { 715827883 };
    EXPECT_CELLS_EQ(layer_cells(BLOT_SCATTER, BLOT_DATA_INT64, 1, xs, ys, { 0, 1, 0, 1 }, { 12, 12 }),
                    Cells());
}

TEST(Layer, bar_height_fractional_range)
{
    /* y 0..1 on 10 rows: 1 is the top row, 0.5 is row 5; the bar for x 1 starts in column 5 */
    const double xs[] = { 0, 1 }, ys[] = { 0.5, 1 };
    Cells got = layer_cells(BLOT_BAR, BLOT_DATA_DOUBLE, 2, xs, ys, { 0, 1, 0, 1 });
    EXPECT_TRUE(got.count({0, 5})) << cells_str(got);
    EXPECT_FALSE(got.count({0, 6})) << cells_str(got);
    EXPECT_TRUE(got.count({5, 9})) << cells_str(got);
}

TEST(Layer, bar_taller_than_limits_cut_at_top)
{
    /* 6 * 715827883.5 is 2^32 + 5: the old bar height wrapped to row 5.  X has
     * 2 slots of 6 columns, the bar fills columns 0..3 */
    const double xs[] = { 0 }, ys[] = { 715827883.5 };
    EXPECT_CELLS_EQ(layer_cells(BLOT_BAR, BLOT_DATA_DOUBLE, 1, xs, ys, { 0, 1, 0, 1 }, { 12, 12 }),
                    rect(0, 3, 0, 11));
}

/* #3: a value exactly at max is in the last cell.  The position
 * (max - min) * len / (max - min) can round to just above len, e.g. 0.1 .. 0.7
 * on 7 cells gives 7.000000000000001, and the point used to be dropped. */
static bool reaches_row(const Cells &cs, unsigned row)
{
    for (const auto &c : cs)
        if (c.second == row)
            return true;
    return false;
}

TEST(Layer, value_at_max_in_last_cell_reproducer)
{
    const double xs[] = { 0.7 }, ys[] = { 0.7 };
    EXPECT_CELLS_EQ(layer_cells(BLOT_SCATTER, BLOT_DATA_DOUBLE, 1, xs, ys, { 0.1, 0.7, 0.1, 0.7 }, { 7, 7 }),
                    Cells({ {6,6} }));
    /* bar X is the slot model (#12); only its height is checked */
    Cells bar = layer_cells(BLOT_BAR, BLOT_DATA_DOUBLE, 1, xs, ys, { 0.1, 0.7, 0.1, 0.7 }, { 7, 7 });
    EXPECT_TRUE(reaches_row(bar, 6)) << cells_str(bar);
}

TEST(Layer, value_at_max_in_last_cell_sweep)
{
    const double ranges[][2] = {
        { 0.1, 0.7 }, { 0.1, 0.3 }, { 0.2, 0.9 }, { -0.3, 0.7 }, { 0.15, 1.35 },
        { -2.2, -0.1 }, { 1e-9, 3e-9 }, { 0.001, 0.007 }, { 3.3, 9.9 }, { -1e300, 1e300 },
    };
    unsigned bad = 0;
    for (const auto &rg : ranges) {
        for (unsigned len = 1; len <= 64; len++) {
            SCOPED_TRACE(testing::Message() << "range " << rg[0] << ".." << rg[1] << " len " << len);
            const blot_xy_limits lim = { rg[0], rg[1], rg[0], rg[1] };
            const blot_dimensions dim = { len, len };
            const double xs[] = { rg[1] }, ys[] = { rg[1] };
            Cells sc = layer_cells(BLOT_SCATTER, BLOT_DATA_DOUBLE, 1, xs, ys, lim, dim);
            EXPECT_EQ(sc, Cells({ { len - 1, len - 1 } })) << cells_str(sc);
            Cells bar = layer_cells(BLOT_BAR, BLOT_DATA_DOUBLE, 1, xs, ys, lim, dim);
            EXPECT_TRUE(reaches_row(bar, len - 1)) << cells_str(bar);
            bad += sc.size() != 1 || !reaches_row(bar, len - 1);
            if (bad > 5)
                return;
        }
    }
}

/* an infinite range (-DBL_MAX .. DBL_MAX) or NaN limits give every value a
 * NaN position: nothing is drawn, and nothing is converted to unsigned */
TEST(Layer, infinite_or_nan_range_draws_nothing)
{
    const double xs[] = { 0, G_MAXDOUBLE }, ys[] = { 0, G_MAXDOUBLE };
    const double nan = std::nan("");
    const blot_xy_limits lims[] = {
        { -G_MAXDOUBLE, G_MAXDOUBLE, -G_MAXDOUBLE, G_MAXDOUBLE },
        { nan, nan, nan, nan },
    };
    for (const auto &lim : lims) {
        SCOPED_TRACE(testing::Message() << "x " << lim.x_min << ".." << lim.x_max);
        EXPECT_CELLS_EQ(layer_cells(BLOT_SCATTER, BLOT_DATA_DOUBLE, 2, xs, ys, lim), Cells());
        EXPECT_CELLS_EQ(layer_cells(BLOT_BAR, BLOT_DATA_DOUBLE, 2, xs, ys, lim), Cells());
        const gint64 xi[] = { 0, 1 }, yi[] = { 0, 1 };
        EXPECT_CELLS_EQ(layer_cells(BLOT_SCATTER, BLOT_DATA_INT64, 2, xi, yi, lim), Cells());
    }
}

TEST(Layer, line_fractional_tiny_huge)
{
    const double ranges[][2] = { { 0, 1 }, { 1e-9, 3e-9 }, { -1e300, 1e300 } };
    for (const auto &rg : ranges) {
        SCOPED_TRACE(rg[0]);
        const double v[] = { rg[0], rg[1] };
        EXPECT_CELLS_EQ(layer_cells(BLOT_LINE, BLOT_DATA_DOUBLE, 2, v, v, { rg[0], rg[1], rg[0], rg[1] }),
                        diagonal(10));
    }
}

TEST(Layer, line_draws_last_point)
{
    const gint32 xs[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 }, ys[10] = {};
    EXPECT_CELLS_EQ(layer_cells(BLOT_LINE, BLOT_DATA_INT32, 10, xs, ys), rect(0, 9, 0, 0));
}

TEST(Layer, line_single_point)
{
    const gint32 xs[] = { 5 }, ys[] = { 5 };
    EXPECT_CELLS_EQ(layer_cells(BLOT_LINE, BLOT_DATA_INT32, 1, xs, ys), Cells({ {5,5} }));
}

TEST(Layer, line_shallow_segment_endpoint)
{
    const gint32 xs[] = { 0, 10 }, ys[] = { 0, 1 };
    Cells got = layer_cells(BLOT_LINE, BLOT_DATA_INT32, 2, xs, ys, { 0, 10, 0, 9 }, { 11, 10 });
    EXPECT_TRUE(got.count({10, 1})) << cells_str(got);
    EXPECT_EQ(got.size(), 11u) << cells_str(got);
}

/* a NaN point must not hang the walk (lround(NaN) is LONG_MIN on x86-64);
 * how NaN should look is #6, so only the finite segment is pinned */
TEST(Layer, line_with_nan_terminates)
{
    RUN_ISOLATED(5, {
        const double xs[] = { 0, 1, 2, 3 }, ys[] = { 0, NAN, 2, 3 };
        Cells got = layer_cells(BLOT_LINE, BLOT_DATA_DOUBLE, 4, xs, ys, { 0, 3, 0, 3 });
        for (unsigned i = 6; i <= 9; i++)
            EXPECT_TRUE(got.count({ i, i })) << cells_str(got);
    });
    RUN_ISOLATED(5, {
        const double xs[] = { 0, NAN, 2, 3 }, ys[] = { 0, 1, 2, 3 };
        Cells got = layer_cells(BLOT_LINE, BLOT_DATA_DOUBLE, 4, xs, ys, { 0, 3, 0, 3 });
        for (unsigned i = 6; i <= 9; i++)
            EXPECT_TRUE(got.count({ i, i })) << cells_str(got);
    });
}

TEST(Layer, line_right_to_left_and_downward)
{
    const gint32 xs[] = { 9, 0 }, ys[] = { 0, 9 };
    Cells anti;
    for (unsigned i = 0; i < 10; i++)
        anti.insert({9 - i, i});
    EXPECT_CELLS_EQ(layer_cells(BLOT_LINE, BLOT_DATA_INT32, 2, xs, ys), anti);

    const gint32 vx[] = { 5, 5 }, vy[] = { 9, 0 };
    EXPECT_CELLS_EQ(layer_cells(BLOT_LINE, BLOT_DATA_INT32, 2, vx, vy), rect(5, 5, 0, 9));
}

/* #14: a line end is drawn in the cell of the axis mapping, the cell a
 * scatter point of the same value is in (expected_cell(): floor((v - min) *
 * len / range), max in the last cell).  For many limits and sizes, and for
 * values on cell edges (and one ulp either side), cell centres, tick values
 * of 3, 5 and 7 ticks, min and max:
 * - a one-point line draws exactly the scatter cell, and that is the
 *   expected cell;
 * - a two-point line draws both end cells. */
TEST(Layer, line_ends_use_axis_mapping_sweep)
{
    const double ranges[][2] = {
        { 0, 1 }, { 0, 9 }, { -250, -200 }, { -0.301, 0.701 }, { 999.9, 1000 },
        { 1e-9, 3e-9 }, { -1e6, 1e6 }, { 0.1, 0.7 }, { -5.006, 1.006 }, { 9.99, 20.01 },
    };
    const unsigned lens[] = { 1, 2, 3, 5, 7, 10, 12, 14, 16, 29, 31, 51, 64, 100 };

    unsigned checked = 0, failed = 0;
    std::ostringstream first;
    auto fail = [&](const std::string &what) {
        if (failed++ < 10)
            first << "\n  " << what;
    };

    for (const auto &rg : ranges) {
        const double lo = rg[0], hi = rg[1], range = hi - lo;
        for (unsigned len : lens) {
            std::vector<double> vals = { lo, hi };
            for (unsigned k = 0; k <= len; k++) {
                double e = lo + k * range / len;
                vals.insert(vals.end(), { e, nextafter(e, -INFINITY), nextafter(e, INFINITY),
                                          lo + (k + 0.5) * range / len });
            }
            for (unsigned n : { 3u, 5u, 7u })
                for (unsigned ti = 0; ti <= n; ti++)
                    vals.push_back(lo + ti * (range / n));
            vals.erase(std::remove_if(vals.begin(), vals.end(),
                                      [&](double v) { return !(v >= lo && v <= hi); }),
                       vals.end());

            const blot_xy_limits lim = { lo, hi, lo, hi };
            const blot_dimensions dim = { len, len };
            const size_t nv = vals.size();
            for (size_t i = 0; i < nv; i++) {
                /* x and y from different values, so both axes see all of them */
                double x = vals[i], y = vals[(i * 7 + 3) % nv];
                Cell want = { expected_cell(x, lo, hi, len), expected_cell(y, lo, hi, len) };
                std::ostringstream at;
                at.precision(17);
                at << "limits " << lo << ".." << hi << " len " << len << " point (" << x << ", " << y << ")";

                Cells sc = layer_cells(BLOT_SCATTER, BLOT_DATA_DOUBLE, 1, &x, &y, lim, dim);
                Cells ln = layer_cells(BLOT_LINE, BLOT_DATA_DOUBLE, 1, &x, &y, lim, dim);
                checked++;
                if (sc != Cells({ want }))
                    fail(at.str() + ": scatter" + cells_str(sc));
                if (ln != Cells({ want }))
                    fail(at.str() + ": line" + cells_str(ln) + ", want (" + std::to_string(want.first) +
                         "," + std::to_string(want.second) + ")");

                /* a segment to another point: both end cells drawn */
                double xs[] = { x, vals[(i * 3 + 1) % nv] }, ys[] = { y, vals[(i * 5 + 2) % nv] };
                Cell want1 = { expected_cell(xs[1], lo, hi, len), expected_cell(ys[1], lo, hi, len) };
                Cells seg = layer_cells(BLOT_LINE, BLOT_DATA_DOUBLE, 2, xs, ys, lim, dim);
                if (!seg.count(want) || !seg.count(want1))
                    fail(at.str() + " to (" + std::to_string(xs[1]) + ", " + std::to_string(ys[1]) +
                         "): segment ends not drawn" + cells_str(seg));
            }
        }
    }
    EXPECT_EQ(failed, 0u) << "of " << checked << " points; first failures:" << first.str();
    EXPECT_GT(checked, 10000u);
}

/* #14: the issue's tick values, on a 10x14 layer (the plot area of a 40x16
 * figure is 31x14; 5 ticks on 14 rows are on rows 0, 2, 5, 8 and 11) */
TEST(Layer, line_on_tick_values)
{
    const struct { double lo, hi, v; unsigned row; } cases[] = {
        { 0, 1, 0.2, 2 }, { 0, 1, 0.8, 11 },
        { 0, 9, 1.8, 2 }, { 0, 9, 7.2, 11 },
        { -250, -200, -240, 2 }, { -250, -200, -210, 11 },
    };
    for (const auto &c : cases) {
        SCOPED_TRACE(testing::Message() << "y " << c.lo << ".." << c.hi << " value " << c.v);
        const double xs[] = { 0, 1 }, ys[] = { c.v, c.v };
        EXPECT_CELLS_EQ(layer_cells(BLOT_LINE, BLOT_DATA_DOUBLE, 2, xs, ys, { 0, 1, c.lo, c.hi }, { 10, 14 }),
                        rect(0, 9, c.row, c.row));
        /* the same on X: a vertical line in column 2 or 11 of 14 */
        EXPECT_CELLS_EQ(layer_cells(BLOT_LINE, BLOT_DATA_DOUBLE, 2, ys, xs, { c.lo, c.hi, 0, 1 }, { 14, 10 }),
                        rect(c.row, c.row, 0, 9));
    }
}

TEST(Layer, bar_width_and_height)
{
    const gint32 xs[] = { 3 }, ys[] = { 4 };
    EXPECT_CELLS_EQ(layer_cells(BLOT_BAR, BLOT_DATA_INT32, 1, xs, ys, LIM09, { 20, 10 }), rect(6, 7, 0, 4));
}

// BUG B21: https://git.jukie.net/bart/blot/-/work_items/12
TEST(Layer, DISABLED_bar_negative_starts_at_zero)
{
    /* y -5..4 on 10 rows: zero is row 5, -3 is row 2, 3 is row 8 */
    const gint32 xs[] = { 2, 6 }, ys[] = { -3, 3 };
    EXPECT_CELLS_EQ(layer_cells(BLOT_BAR, BLOT_DATA_INT32, 2, xs, ys, { 0, 9, -5, 4 }),
                    rect(2, 3, 2, 5) + rect(6, 7, 5, 8));
}

TEST(Layer, bar_below_limit_terminates)
{
    RUN_ISOLATED(5, {
        const gint32 xs[] = { 1, 2 }, ys[] = { -5, 5 };
        const blot_xy_limits lim = { 0, 3, 0, 10 };
        /* the bar for -5 sets no cell: the cells are those of (2,5) alone */
        Cells alone = layer_cells(BLOT_BAR, BLOT_DATA_INT32, 1, xs + 1, ys + 1, lim);
        ASSERT_FALSE(alone.empty());
        EXPECT_CELLS_EQ(layer_cells(BLOT_BAR, BLOT_DATA_INT32, 2, xs, ys, lim), alone);
    });
}

// BUG B8: https://git.jukie.net/bart/blot/-/work_items/5
TEST(Layer, DISABLED_line_far_outside_limits_terminates)
{
    RUN_ISOLATED(5, {
        const double xs[] = { 0, 1 }, ys[] = { 0, 1e12 };
        layer_cells(BLOT_LINE, BLOT_DATA_DOUBLE, 2, xs, ys, { 0, 1, 0, 1 });
    });
}

/* B7 (#6): terminates since #4 (a non-finite segment is not drawn); #6
 * still owns how inf should look */
TEST(Layer, line_infinite_value_terminates)
{
    RUN_ISOLATED(5, {
        const double xs[] = { 0, 1 }, ys[] = { 0, INFINITY };
        layer_cells(BLOT_LINE, BLOT_DATA_DOUBLE, 2, xs, ys, { 0, 1, 0, 1 });
    });
}

TEST(Layer, nan_points_not_drawn)
{
    const double xs[] = { 0.5, NAN, 5 }, ys[] = { NAN, 0.5, 5 };
    EXPECT_CELLS_EQ(layer_cells(BLOT_SCATTER, BLOT_DATA_DOUBLE, 3, xs, ys), Cells({ {5,5} }));
}

TEST(Layer, summary_statistics)
{
    /* limits cover the data: a bar below the Y limit hangs (#5) */
    const blot_xy_limits lim = { 0, 3, -2, 4 };
    const double xs[] = { 0, 1, 2, 3 }, ys[] = { -2, 4, 1, 0.5 };
    const gint64 xs64[] = { 0, 1, 2, 3 }, ys64[] = { -2, 4, 1, 0 };
    const struct {
        blot_plot_type type;
        blot_data_type dt;
        const void *xs, *ys;
        double yttl;
    } cases[] = {
        { BLOT_SCATTER, BLOT_DATA_DOUBLE, xs, ys, 3.5 },
        { BLOT_LINE, BLOT_DATA_DOUBLE, xs, ys, 3.5 },
        { BLOT_BAR, BLOT_DATA_DOUBLE, xs, ys, 3.5 },
        { BLOT_SCATTER, BLOT_DATA_INT64, xs64, ys64, 3 },
    };
    for (const auto &c : cases) {
        SCOPED_TRACE(testing::Message() << "plot " << c.type << " data " << c.dt);
        blot_layer *lay = blot_layer_new(c.type, c.dt, 4, c.xs, c.ys, 1, "l", NULL);
        ASSERT_TRUE(lay != NULL);
        Canvas can(blot_layer_render(lay, &lim, &DIM10,
                                     BLOT_RENDER_NO_UNICODE | BLOT_RENDER_LEGEND_DETAILS, NULL));
        ASSERT_TRUE(can.can != NULL);
        const blot_layer_summary &s = lay->summary;
        EXPECT_TRUE(s.enabled);
        EXPECT_EQ(s.count, 4u);
        EXPECT_DOUBLE_EQ(s.xmin, 0);
        EXPECT_DOUBLE_EQ(s.xmax, 3);
        EXPECT_DOUBLE_EQ(s.xttl, 6);
        EXPECT_DOUBLE_EQ(s.ymin, -2);
        EXPECT_DOUBLE_EQ(s.ymax, 4);
        EXPECT_DOUBLE_EQ(s.yttl, c.yttl);
        blot_layer_delete(lay);
    }
}

TEST(Layer, summary_disabled_and_reset)
{
    const double xs[] = { 0, 1, 2, 3 }, ys[] = { 0, 4, 1, 0.5 };
    blot_layer *lay = blot_layer_new(BLOT_SCATTER, BLOT_DATA_DOUBLE, 4, xs, ys, 1, "l", NULL);
    ASSERT_TRUE(lay != NULL);

    blot_canvas_delete(blot_layer_render(lay, &LIM09, &DIM10, BLOT_RENDER_NO_UNICODE, NULL));
    EXPECT_FALSE(lay->summary.enabled);

    for (int i = 0; i < 2; i++)
        blot_canvas_delete(blot_layer_render(lay, &LIM09, &DIM10,
                                             BLOT_RENDER_NO_UNICODE | BLOT_RENDER_LEGEND_DETAILS, NULL));
    EXPECT_TRUE(lay->summary.enabled);
    EXPECT_EQ(lay->summary.count, 4u);
    EXPECT_DOUBLE_EQ(lay->summary.yttl, 5.5);
    blot_layer_delete(lay);
}

/* B6 (#6): a NaN no longer makes get_lim forget the values before it */
TEST(Layer, get_lim_ignores_nan)
{
    const double xs[] = { 0, 1, 2, 3 }, ys[] = { 100, NAN, 1, 2 };
    blot_layer *lay = blot_layer_new(BLOT_SCATTER, BLOT_DATA_DOUBLE, 4, xs, ys, 1, "l", NULL);
    ASSERT_TRUE(lay != NULL);
    blot_xy_limits lim = {};
    EXPECT_TRUE(blot_layer_get_lim(lay, &lim, NULL));
    EXPECT_DOUBLE_EQ(lim.y_min, 1);
    EXPECT_DOUBLE_EQ(lim.y_max, 100);
    blot_layer_delete(lay);
}

/* #6: each coordinate's limits are those of its finite values; NaN and
 * +-inf are skipped wherever they are.  A coordinate with no finite value
 * at all has NaN limits ("no data"). */
TEST(Layer, get_lim_ignores_non_finite)
{
    struct Case {
        std::vector<double> xs, ys;
        blot_xy_limits want;
    };
    const double N = NAN, I = INFINITY;
    const Case cases[] = {
        { { 0, 1, 2, 3 }, { N, 5, 1, 2 }, { 0, 3, 1, 5 } },
        { { 0, 1, 2, 3 }, { 1, 2, 3, N }, { 0, 3, 1, 3 } },
        { { 0, 1, 2, 3 }, { 1, 2, I, 3 }, { 0, 3, 1, 3 } },
        { { 0, 1, 2, 3 }, { 1, -I, 2, 3 }, { 0, 3, 1, 3 } },
        { { N, 1, I, 3 }, { 7, 8, 9, 6 }, { 1, 3, 6, 9 } },
        { { -I, 4, 2, N }, { I, -I, 9, N }, { 2, 4, 9, 9 } },
    };
    for (const Case &c : cases) {
        SCOPED_TRACE(testing::Message() << "y[0] " << c.ys[0] << " x[0] " << c.xs[0]);
        blot_layer *lay = blot_layer_new(BLOT_LINE, BLOT_DATA_DOUBLE, c.xs.size(), c.xs.data(), c.ys.data(),
                                         1, "l", NULL);
        ASSERT_TRUE(lay != NULL);
        blot_xy_limits lim = {};
        EXPECT_TRUE(blot_layer_get_lim(lay, &lim, NULL));
        EXPECT_EQ(lim.x_min, c.want.x_min);
        EXPECT_EQ(lim.x_max, c.want.x_max);
        EXPECT_EQ(lim.y_min, c.want.y_min);
        EXPECT_EQ(lim.y_max, c.want.y_max);
        blot_layer_delete(lay);
    }

    /* no finite y: the y limits are NaN, the x limits still those of x */
    const double xs[] = { 0, 1, 2 }, ys[] = { N, I, -I };
    blot_layer *lay = blot_layer_new(BLOT_LINE, BLOT_DATA_DOUBLE, 3, xs, ys, 1, "l", NULL);
    ASSERT_TRUE(lay != NULL);
    blot_xy_limits lim = {};
    EXPECT_TRUE(blot_layer_get_lim(lay, &lim, NULL));
    EXPECT_EQ(lim.x_min, 0);
    EXPECT_EQ(lim.x_max, 2);
    EXPECT_TRUE(std::isnan(lim.y_min)) << lim.y_min;
    EXPECT_TRUE(std::isnan(lim.y_max)) << lim.y_max;
    blot_layer_delete(lay);

    /* xs NULL: x is the index 0..count-1, even where y is not finite */
    const double yn[] = { N, 4, 2, N };
    lay = blot_layer_new(BLOT_LINE, BLOT_DATA_DOUBLE, 4, NULL, yn, 1, "l", NULL);
    ASSERT_TRUE(lay != NULL);
    lim = {};
    EXPECT_TRUE(blot_layer_get_lim(lay, &lim, NULL));
    EXPECT_EQ(lim.x_min, 0);
    EXPECT_EQ(lim.x_max, 3);
    EXPECT_EQ(lim.y_min, 2);
    EXPECT_EQ(lim.y_max, 4);
    blot_layer_delete(lay);

    /* FLOAT data takes the same path */
    const float xf[] = { 0, 1, 2 }, yf[] = { 3, NAN, -INFINITY };
    lay = blot_layer_new(BLOT_SCATTER, BLOT_DATA_FLOAT, 3, xf, yf, 1, "l", NULL);
    ASSERT_TRUE(lay != NULL);
    lim = {};
    EXPECT_TRUE(blot_layer_get_lim(lay, &lim, NULL));
    EXPECT_EQ(lim.y_min, 3);
    EXPECT_EQ(lim.y_max, 3);
    blot_layer_delete(lay);
}

/* #6: a NaN or infinite point is a gap in a line: no segment goes to it or
 * from it, and a finite point with a gap on both sides (or at an end of the
 * series next to a gap) is drawn as its own single cell */
TEST(Layer, line_non_finite_is_a_gap)
{
    const double N = NAN, I = INFINITY;
    RUN_ISOLATED(5, {
        /* { NaN, 5, NaN }: the 5 alone */
        const double xs[] = { 0, 1, 2 }, ys[] = { N, 5, N };
        EXPECT_CELLS_EQ(layer_cells(BLOT_LINE, BLOT_DATA_DOUBLE, 3, xs, ys), Cells({ { 1, 5 } }));
        /* the same with xs NULL */
        EXPECT_CELLS_EQ(layer_cells(BLOT_LINE, BLOT_DATA_DOUBLE, 3, NULL, ys), Cells({ { 1, 5 } }));
    });
    RUN_ISOLATED(5, {
        /* a gap is not bridged: (0,1) and (2,3) alone, nothing at x 1 */
        const double xs[] = { 0, 1, 2 }, ys[] = { 1, N, 3 };
        EXPECT_CELLS_EQ(layer_cells(BLOT_LINE, BLOT_DATA_DOUBLE, 3, xs, ys), Cells({ { 0, 1 }, { 2, 3 } }));
    });
    RUN_ISOLATED(5, {
        /* inf and -inf are gaps too, in y or in x */
        const double xs[] = { 0, 1, 2, 3, 4, 5, 6 }, ys[] = { 0, 1, I, 3, -I, 5, 6 };
        Cells want = { { 0, 0 }, { 1, 1 }, { 3, 3 }, { 5, 5 }, { 6, 6 } };
        EXPECT_CELLS_EQ(layer_cells(BLOT_LINE, BLOT_DATA_DOUBLE, 7, xs, ys), want);
        const double xg[] = { 0, 1, N, 3, I, 5, 6 }, yg[] = { 0, 1, 2, 3, 4, 5, 6 };
        EXPECT_CELLS_EQ(layer_cells(BLOT_LINE, BLOT_DATA_DOUBLE, 7, xg, yg), want);
    });
    RUN_ISOLATED(5, {
        /* runs on either side of a gap are drawn as lines */
        const double xs[] = { 0, 2, 4, 5, 7, 9 }, ys[] = { 0, 2, N, 5, 7, 9 };
        EXPECT_CELLS_EQ(layer_cells(BLOT_LINE, BLOT_DATA_DOUBLE, 6, xs, ys),
                        diagonal(3) + Cells({ { 5, 5 }, { 6, 6 }, { 7, 7 }, { 8, 8 }, { 9, 9 } }));
    });
    RUN_ISOLATED(5, {
        /* a leading or trailing gap, and nothing finite at all */
        const double xs[] = { 0, 1, 2, 3 }, yl[] = { N, 1, 2, N }, yn[] = { N, I, -I, N };
        EXPECT_CELLS_EQ(layer_cells(BLOT_LINE, BLOT_DATA_DOUBLE, 4, xs, yl), Cells({ { 1, 1 }, { 2, 2 } }));
        EXPECT_CELLS_EQ(layer_cells(BLOT_LINE, BLOT_DATA_DOUBLE, 4, xs, yn), Cells());
    });
    RUN_ISOLATED(5, {
        /* B7: the line through (1, inf) with limits 0..1 draws (0,0) alone */
        const double xs[] = { 0, 1 }, ys[] = { 0, I };
        EXPECT_CELLS_EQ(layer_cells(BLOT_LINE, BLOT_DATA_DOUBLE, 2, xs, ys, { 0, 1, 0, 1 }), Cells({ { 0, 0 } }));
    });
}

/* #6: a bar with a NaN or infinite height (or x) is not drawn; +inf is
 * not a bar cut at the top */
TEST(Layer, bar_non_finite_not_drawn)
{
    const double N = NAN, I = INFINITY;
    const double xs[] = { 1, 2, 3, 4, N, I }, ys[] = { I, -I, N, 5, 3, 3 };
    Cells alone = layer_cells(BLOT_BAR, BLOT_DATA_DOUBLE, 1, xs + 3, ys + 3);
    ASSERT_FALSE(alone.empty());
    EXPECT_CELLS_EQ(layer_cells(BLOT_BAR, BLOT_DATA_DOUBLE, 6, xs, ys), alone);
}

/* #6: with a range wider than about DBL_MAX / len, (v - min) * len was inf
 * and the point was dropped; such ranges now map like any other */
TEST(Layer, huge_range_mapping)
{
    const double D = DBL_MAX;
    for (blot_plot_type type : { BLOT_SCATTER, BLOT_BAR }) {
        SCOPED_TRACE(testing::Message() << "plot " << type);
        const double xs[] = { 0, 5, 9 };
        const double up[] = { 0, D / 2, D };
        const double down[] = { -D, -D / 2, 0 };
        const double mid[] = { -D / 2, 0, D / 2 };
        const double e308[] = { 0, 5e307, 1e308 };
        struct { const double *ys; blot_xy_limits lim; } cases[] = {
            { up, { 0, 9, 0, D } },
            { down, { 0, 9, -D, 0 } },
            { mid, { 0, 9, -D / 2, D / 2 } },
            { e308, { 0, 9, 0, 1e308 } },
        };
        for (const auto &c : cases) {
            SCOPED_TRACE(testing::Message() << "y " << c.lim.y_min << ".." << c.lim.y_max);
            Cells got = layer_cells(type, BLOT_DATA_DOUBLE, 3, xs, c.ys, c.lim);
            if (type == BLOT_SCATTER) {
                EXPECT_CELLS_EQ(got, Cells({ { 0, 0 }, { 5, 5 }, { 9, 9 } }));
            } else {
                /* the bars reach rows 0, 5 and 9 */
                EXPECT_TRUE(got.count({ 0, 0 }) && !got.count({ 0, 1 })) << cells_str(got);
                EXPECT_TRUE(got.count({ 5, 5 }) && !got.count({ 5, 6 })) << cells_str(got);
                EXPECT_TRUE(got.count({ 9, 9 })) << cells_str(got);
            }
        }
    }
    /* the X axis too, at 1000 columns */
    const double xh[] = { 0, D / 2, D }, yh[] = { 0, 5, 9 };
    Cells got = layer_cells(BLOT_SCATTER, BLOT_DATA_DOUBLE, 3, xh, yh, { 0, D, 0, 9 }, { 1000, 10 });
    EXPECT_CELLS_EQ(got, Cells({ { 0, 0 }, { 500, 5 }, { 999, 9 } }));
}

/* #6: the overflow-safe mapping changes no cell for ordinary ranges: a
 * sweep of values, including every cell boundary and its neighbours,
 * lands where the plain formula floor((v - min) * len / (max - min)) puts
 * it (expected_cell; the axis uses the same formula) */
TEST(Layer, cell_mapping_unchanged_for_normal_ranges)
{
    const double ranges[][2] = { { 0, 1 }, { 0, 9 }, { -250, -200 }, { -1e15, 1e15 }, { 1e-9, 3e-9 },
                                 { 0.1, 0.7 }, { -3.3, -2.9 }, { 0, 1e300 }, { -1e305, 1e305 },
                                 { -0.001, 1.001 }, { 6.3, 7.42 }, { 1, 1 + 1e-12 } };
    const unsigned lens[] = { 10, 12, 14, 37, 100, 1000 };
    std::mt19937_64 rng(6);
    for (const auto &r : ranges) {
        for (unsigned len : lens) {
            /* ordinary: the plain formula doesn't overflow anywhere in
             * the range (huge_range_mapping has the others) */
            if (!std::isfinite((r[1] - r[0]) * len))
                continue;
            SCOPED_TRACE(testing::Message() << r[0] << ".." << r[1] << " len " << len);
            std::vector<double> vs;
            for (unsigned k = 0; k <= len; k++) {
                double b = r[0] + (r[1] - r[0]) * k / len;
                vs.push_back(b);
                vs.push_back(std::nextafter(b, -INFINITY));
                vs.push_back(std::nextafter(b, INFINITY));
            }
            std::uniform_real_distribution<double> u(r[0], r[1]);
            for (int i = 0; i < 2000; i++)
                vs.push_back(u(rng));
            vs.push_back(r[0]);
            vs.push_back(r[1]);

            /* one point per value, on its own column, so no two collide */
            for (size_t i = 0; i < vs.size(); i += 900) {
                size_t n = std::min<size_t>(900, vs.size() - i);
                std::vector<double> xs(n), ys(vs.begin() + i, vs.begin() + i + n);
                for (size_t j = 0; j < n; j++)
                    xs[j] = j;
                Cells want;
                for (size_t j = 0; j < n; j++) {
                    /* drawn when the value is within the limits (#14),
                     * in the cell of the plain formula */
                    if (ys[j] >= r[0] && ys[j] <= r[1])
                        want.insert({ (unsigned)j, expected_cell(ys[j], r[0], r[1], len) });
                }
                Cells got = layer_cells(BLOT_SCATTER, BLOT_DATA_DOUBLE, n, xs.data(), ys.data(),
                                        { -0.5, 900 - 0.5, r[0], r[1] }, { 900, len });
                EXPECT_CELLS_EQ(got, want);
                if (HasFailure())
                    return;
            }
        }
    }
}

// B18, fixed in #11: https://git.jukie.net/bart/blot/-/work_items/11
TEST(Layer, index_equal_to_count_rejected)
{
    const gint32 xs[] = { 0, 1, 2, 3 }, ys[] = { 0, 1, 2, 3 };
    blot_layer *lay = blot_layer_new(BLOT_SCATTER, BLOT_DATA_INT32, 3, xs, ys, 1, "l", NULL);
    ASSERT_TRUE(lay != NULL);
    GError *error = NULL;
    double v = -1;
    EXPECT_FALSE(blot_layer_get_x(lay, 3, &v, &error));
    ASSERT_TRUE(error != NULL);
    EXPECT_EQ(error->code, ERANGE);
    g_clear_error(&error);
    EXPECT_FALSE(blot_layer_get_y(lay, 3, &v, &error));
    ASSERT_TRUE(error != NULL);
    EXPECT_EQ(error->code, ERANGE);
    g_clear_error(&error);
    blot_layer_delete(lay);
}

TEST(Layer, braille_scatter_dots)
{
    /* 5x3 cells = 10x12 dots, one dot per integer */
    const gint32 xs[] = { 0, 9, 4 }, ys[] = { 0, 11, 6 };
    Canvas can(layer_canvas(BLOT_SCATTER, BLOT_DATA_INT32, 3, xs, ys, { 0, 9, 0, 11 }, { 5, 3 },
                            BLOT_RENDER_BRAILLE));
    ASSERT_TRUE(can.can != NULL);
    std::map<Cell, wchar_t> expected = {
        { {0,0}, 0x2840 }, { {4,2}, 0x2808 }, { {2,1}, 0x2802 },
    };
    EXPECT_EQ(glyphs(can), expected);
}

/* B18 (#11): blot_layer_render() checked the plot and data types with '>'
 * before indexing its tables; the value MAX itself is out of range */
TEST(Layer, render_rejects_types_at_max)
{
    RUN_ISOLATED(10, {
        const gint32 xs[] = { 0, 1, 2 }, ys[] = { 0, 1, 2 };
        const blot_xy_limits lim = { 0, 2, 0, 2 };
        const blot_dimensions dim = { 10, 10 };
        blot_layer *lay = blot_layer_new(BLOT_SCATTER, BLOT_DATA_INT32, 3, xs, ys, 1, "l", NULL);
        ASSERT_TRUE(lay != NULL);

        GError *error = NULL;
        lay->data_type = BLOT_DATA_TYPE_MAX;
        EXPECT_TRUE(blot_layer_render(lay, &lim, &dim, BLOT_RENDER_NONE, &error) == NULL);
        ASSERT_TRUE(error != NULL);
        EXPECT_EQ(error->code, EINVAL);
        g_clear_error(&error);

        lay->data_type = BLOT_DATA_INT32;
        lay->plot_type = BLOT_PLOT_TYPE_MAX;
        EXPECT_TRUE(blot_layer_render(lay, &lim, &dim, BLOT_RENDER_NONE, &error) == NULL);
        ASSERT_TRUE(error != NULL);
        EXPECT_EQ(error->code, EINVAL);
        g_clear_error(&error);
        blot_layer_delete(lay);
    });
}
