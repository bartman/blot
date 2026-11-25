#include <gtest/gtest.h>

#include "blot_test.hpp"

#include "blot_terminal.h"
#include "blot_figure.h"
#include "blot_error.h"
#include "blot_types.h"
#include "blot_screen.h"
#include "blot_layer.h"
#include "blot_canvas.h"
#include "blot_axis.h"

TEST(Figure, alloc_new_delete)
{
    GError *error = NULL;
    blot_figure *fig = blot_figure_new(&error);
    ASSERT_TRUE(fig != NULL);
    ASSERT_TRUE(error == NULL);
    blot_figure_delete(fig);
}

TEST(Figure, set_axis_color)
{
    GError *error = NULL;
    blot_figure *fig = blot_figure_new(&error);
    ASSERT_TRUE(fig != NULL);
    ASSERT_TRUE(error == NULL);

    ASSERT_TRUE(blot_figure_set_axis_color(fig, 10, &error));
    ASSERT_TRUE(error == NULL);
    ASSERT_EQ(fig->axis_color, 10);

    blot_figure_delete(fig);
}

TEST(Figure, set_screen_size)
{
    GError *error = NULL;
    blot_figure *fig = blot_figure_new(&error);
    ASSERT_TRUE(fig != NULL);
    ASSERT_TRUE(error == NULL);

    ASSERT_TRUE(blot_figure_set_screen_size(fig, 100, 50, &error));
    ASSERT_TRUE(error == NULL);
    ASSERT_EQ(fig->dim.cols, 100);
    ASSERT_EQ(fig->dim.rows, 50);

    // Test invalid size
    ASSERT_FALSE(blot_figure_set_screen_size(fig, 0, 0, &error));
    ASSERT_TRUE(error != NULL);
    g_clear_error(&error);

    blot_figure_delete(fig);
}

TEST(Figure, set_x_limits)
{
    GError *error = NULL;
    blot_figure *fig = blot_figure_new(&error);
    ASSERT_TRUE(fig != NULL);
    ASSERT_TRUE(error == NULL);

    ASSERT_TRUE(blot_figure_set_x_limits(fig, -10, 10, &error));
    ASSERT_TRUE(error == NULL);
    ASSERT_EQ(fig->lim.x_min, -10);
    ASSERT_EQ(fig->lim.x_max, 10);

    // Test invalid limits
    ASSERT_FALSE(blot_figure_set_x_limits(fig, 10, -10, &error));
    ASSERT_TRUE(error != NULL);
    g_clear_error(&error);

    blot_figure_delete(fig);
}

TEST(Figure, set_y_limits)
{
    GError *error = NULL;
    blot_figure *fig = blot_figure_new(&error);
    ASSERT_TRUE(fig != NULL);
    ASSERT_TRUE(error == NULL);

    ASSERT_TRUE(blot_figure_set_y_limits(fig, -20, 20, &error));
    ASSERT_TRUE(error == NULL);
    ASSERT_EQ(fig->lim.y_min, -20);
    ASSERT_EQ(fig->lim.y_max, 20);

    // Test invalid limits
    ASSERT_FALSE(blot_figure_set_y_limits(fig, 20, -20, &error));
    ASSERT_TRUE(error != NULL);
    g_clear_error(&error);

    blot_figure_delete(fig);
}

TEST(Figure, set_x_axis_labels)
{
    GError *error = NULL;
    blot_figure *fig = blot_figure_new(&error);
    ASSERT_TRUE(fig != NULL);
    ASSERT_TRUE(error == NULL);

    char *labels[] = { (char*)"a", (char*)"b", (char*)"c" };
    ASSERT_TRUE(blot_figure_set_x_axis_labels(fig, 3, labels, &error));
    ASSERT_TRUE(error == NULL);
    ASSERT_EQ(fig->xlabels.count, 3);
    ASSERT_EQ(fig->xlabels.strings, labels);

    // Test invalid labels
    ASSERT_FALSE(blot_figure_set_x_axis_labels(fig, 0, labels, &error));
    ASSERT_TRUE(error != NULL);
    g_clear_error(&error);

    ASSERT_FALSE(blot_figure_set_x_axis_labels(fig, 3, NULL, &error));
    ASSERT_TRUE(error != NULL);
    g_clear_error(&error);

    blot_figure_delete(fig);
}

TEST(Figure, add_scatter_plot)
{
    GError *error = NULL;
    blot_figure *fig = blot_figure_new(&error);
    ASSERT_TRUE(fig != NULL);
    ASSERT_TRUE(error == NULL);

    const int xs[] = {1, 2, 3};
    const int ys[] = {4, 5, 6};

    ASSERT_TRUE(blot_figure_scatter(fig, BLOT_DATA_INT32, 3, xs, ys, 1, "scatter", &error));
    ASSERT_TRUE(error == NULL);
    ASSERT_EQ(fig->layer_count, 1);

    blot_figure_delete(fig);
}

TEST(Figure, add_line_plot)
{
    GError *error = NULL;
    blot_figure *fig = blot_figure_new(&error);
    ASSERT_TRUE(fig != NULL);
    ASSERT_TRUE(error == NULL);

    const int xs[] = {1, 2, 3};
    const int ys[] = {4, 5, 6};

    ASSERT_TRUE(blot_figure_line(fig, BLOT_DATA_INT32, 3, xs, ys, 1, "line", &error));
    ASSERT_TRUE(error == NULL);
    ASSERT_EQ(fig->layer_count, 1);

    blot_figure_delete(fig);
}

TEST(Figure, add_bar_plot)
{
    GError *error = NULL;
    blot_figure *fig = blot_figure_new(&error);
    ASSERT_TRUE(fig != NULL);
    ASSERT_TRUE(error == NULL);

    const int xs[] = {1, 2, 3};
    const int ys[] = {4, 5, 6};

    ASSERT_TRUE(blot_figure_bar(fig, BLOT_DATA_INT32, 3, xs, ys, 1, "bar", &error));
    ASSERT_TRUE(error == NULL);
    ASSERT_EQ(fig->layer_count, 1);

    blot_figure_delete(fig);
}

TEST(Figure, add_scatter_plot_different_data_types)
{
    GError *error = NULL;
    blot_figure *fig = blot_figure_new(&error);
    ASSERT_TRUE(fig != NULL);

    const float xs_f[] = {1.0f, 2.0f, 3.0f};
    const float ys_f[] = {4.0f, 5.0f, 6.0f};
    ASSERT_TRUE(blot_figure_scatter(fig, BLOT_DATA_FLOAT, 3, xs_f, ys_f, 1, "scatter_float", &error));
    ASSERT_TRUE(error == NULL);
    ASSERT_EQ(fig->layer_count, 1);

    const double xs_d[] = {7.0, 8.0, 9.0};
    const double ys_d[] = {10.0, 11.0, 12.0};
    ASSERT_TRUE(blot_figure_scatter(fig, BLOT_DATA_DOUBLE, 3, xs_d, ys_d, 2, "scatter_double", &error));
    ASSERT_TRUE(error == NULL);
    ASSERT_EQ(fig->layer_count, 2);

    blot_figure_delete(fig);
}

TEST(Figure, add_line_plot_different_data_types)
{
    GError *error = NULL;
    blot_figure *fig = blot_figure_new(&error);
    ASSERT_TRUE(fig != NULL);

    const float xs_f[] = {1.0f, 2.0f, 3.0f};
    const float ys_f[] = {4.0f, 5.0f, 6.0f};
    ASSERT_TRUE(blot_figure_line(fig, BLOT_DATA_FLOAT, 3, xs_f, ys_f, 1, "line_float", &error));
    ASSERT_TRUE(error == NULL);
    ASSERT_EQ(fig->layer_count, 1);

    const double xs_d[] = {7.0, 8.0, 9.0};
    const double ys_d[] = {10.0, 11.0, 12.0};
    ASSERT_TRUE(blot_figure_line(fig, BLOT_DATA_DOUBLE, 3, xs_d, ys_d, 2, "line_double", &error));
    ASSERT_TRUE(error == NULL);
    ASSERT_EQ(fig->layer_count, 2);

    blot_figure_delete(fig);
}

TEST(Figure, add_bar_plot_different_data_types)
{
    GError *error = NULL;
    blot_figure *fig = blot_figure_new(&error);
    ASSERT_TRUE(fig != NULL);

    const float xs_f[] = {1.0f, 2.0f, 3.0f};
    const float ys_f[] = {4.0f, 5.0f, 6.0f};
    ASSERT_TRUE(blot_figure_bar(fig, BLOT_DATA_FLOAT, 3, xs_f, ys_f, 1, "bar_float", &error));
    ASSERT_TRUE(error == NULL);
    ASSERT_EQ(fig->layer_count, 1);

    const double xs_d[] = {7.0, 8.0, 9.0};
    const double ys_d[] = {10.0, 11.0, 12.0};
    ASSERT_TRUE(blot_figure_bar(fig, BLOT_DATA_DOUBLE, 3, xs_d, ys_d, 2, "bar_double", &error));
    ASSERT_TRUE(error == NULL);
    ASSERT_EQ(fig->layer_count, 2);

    blot_figure_delete(fig);
}

TEST(Figure, add_multiple_layers)
{
    GError *error = NULL;
    blot_figure *fig = blot_figure_new(&error);
    ASSERT_TRUE(fig != NULL);

    const int xs1[] = {1, 2, 3};
    const int ys1[] = {4, 5, 6};
    ASSERT_TRUE(blot_figure_scatter(fig, BLOT_DATA_INT32, 3, xs1, ys1, 1, "scatter1", &error));
    ASSERT_TRUE(error == NULL);
    ASSERT_EQ(fig->layer_count, 1);

    const float xs2[] = {7.0f, 8.0f, 9.0f};
    const float ys2[] = {10.0f, 11.0f, 12.0f};
    ASSERT_TRUE(blot_figure_line(fig, BLOT_DATA_FLOAT, 3, xs2, ys2, 2, "line1", &error));
    ASSERT_TRUE(error == NULL);
    ASSERT_EQ(fig->layer_count, 2);

    const double xs3[] = {13.0, 14.0, 15.0};
    const double ys3[] = {16.0, 17.0, 18.0};
    ASSERT_TRUE(blot_figure_bar(fig, BLOT_DATA_DOUBLE, 3, xs3, ys3, 3, "bar1", &error));
    ASSERT_TRUE(error == NULL);
    ASSERT_EQ(fig->layer_count, 3);

    blot_figure_delete(fig);
}

TEST(Figure, add_plot_null_xs)
{
    GError *error = NULL;
    blot_figure *fig = blot_figure_new(&error);
    ASSERT_TRUE(fig != NULL);

    const int ys[] = {4, 5, 6};
    ASSERT_TRUE(blot_figure_scatter(fig, BLOT_DATA_INT32, 3, NULL, ys, 1, "scatter", &error));
    ASSERT_TRUE(error == NULL);

    ASSERT_EQ(fig->layer_count, 1);

    blot_figure_delete(fig);
}

TEST(Figure, add_plot_null_ys)
{
    GError *error = NULL;
    blot_figure *fig = blot_figure_new(&error);
    ASSERT_TRUE(fig != NULL);

    const int xs[] = {1, 2, 3};
    ASSERT_FALSE(blot_figure_scatter(fig, BLOT_DATA_INT32, 3, xs, NULL, 1, "scatter", &error));
    ASSERT_TRUE(error != NULL);
    ASSERT_EQ(error->code, EFAULT);
    g_clear_error(&error);

    blot_figure_delete(fig);
}

TEST(Figure, add_plot_zero_count)
{
    GError *error = NULL;
    blot_figure *fig = blot_figure_new(&error);
    ASSERT_TRUE(fig != NULL);

    const int xs[] = {1, 2, 3};
    const int ys[] = {4, 5, 6};
    ASSERT_FALSE(blot_figure_scatter(fig, BLOT_DATA_INT32, 0, xs, ys, 1, "scatter", &error));
    ASSERT_TRUE(error != NULL);
    /* a zero count is an invalid value, not a NULL pointer (#34) */
    ASSERT_EQ(error->code, EINVAL);
    g_clear_error(&error);

    blot_figure_delete(fig);
}

/* #34: a failed blot_figure_plot() returns false with the error of the
 * argument that is wrong, and leaves the figure as it was: same layers
 * array, same count, the old layer still there */

/* "<func>:<line>: <text>: <strerror>" -> "<text>: <strerror>", if the
 * message starts with "<func>:<line>: " */
static std::string figure_error_text(const GError *error, const char *func)
{
    const std::string prefix = std::string(func) + ":";
    const std::string msg = error->message;

    if (msg.compare(0, prefix.size(), prefix) != 0)
        return "unexpected location in: " + msg;
    size_t i = prefix.size();
    size_t digits = i;
    while (i < msg.size() && isdigit((unsigned char)msg[i]))
        i++;
    if (i == digits || msg.compare(i, 2, ": ") != 0)
        return "unexpected location in: " + msg;
    return msg.substr(i + 2);
}

TEST(Figure, plot_bad_argument_leaves_figure_unchanged)
{
    const int32_t xs[] = {1, 2, 3};
    const int32_t ys[] = {4, 5, 6};
    const struct {
        const char *name;
        blot_plot_type plot_type;
        unsigned data_type;
        size_t count;
        const void *ys;
        int code;
        const char *text;
    } cases[] = {
        { "plot type", BLOT_PLOT_TYPE_MAX, BLOT_DATA_INT32, 3, ys,
          EINVAL, "plot_type 3 is not valid: Invalid argument" },
        { "data type", BLOT_SCATTER, 0x05, 3, ys,
          EINVAL, "data_type 0x5 is not valid: Invalid argument" },
        { "zero count", BLOT_SCATTER, BLOT_DATA_INT32, 0, ys,
          EINVAL, "data_count is 0: Invalid argument" },
        { "NULL ys", BLOT_SCATTER, BLOT_DATA_INT32, 3, NULL,
          EFAULT, "data_ys is NULL: Bad address" },
    };

    for (const auto &c : cases) {
        SCOPED_TRACE(c.name);
        GError *error = NULL;
        blot_figure *fig = blot_figure_new(&error);
        ASSERT_TRUE(fig != NULL);

        ASSERT_TRUE(blot_figure_plot(fig, BLOT_LINE, BLOT_DATA_INT32, 3, xs, ys, 2, "first", &error));
        ASSERT_EQ(fig->layer_count, 1u);
        blot_layer **layers = fig->layers;
        blot_layer *first = fig->layers[0];

        EXPECT_FALSE(blot_figure_plot(fig, c.plot_type, (blot_data_type)c.data_type,
                                      c.count, xs, c.ys, 1, "bad", &error));
        ASSERT_TRUE(error != NULL);
        EXPECT_EQ(error->code, c.code);
        EXPECT_EQ(figure_error_text(error, "blot_layer_new"), c.text);
        g_clear_error(&error);

        EXPECT_EQ(fig->layer_count, 1u);
        EXPECT_EQ(fig->layers, layers);
        EXPECT_EQ(fig->layers[0], first);
        EXPECT_STREQ(fig->layers[0]->label, "first");

        blot_figure_delete(fig);
    }
}

TEST(Figure, plot_NULL_figure)
{
    const int32_t ys[] = {4, 5, 6};
    GError *error = NULL;

    EXPECT_FALSE(blot_figure_plot(NULL, BLOT_SCATTER, BLOT_DATA_INT32, 3, NULL, ys, 1, "x", &error));
    ASSERT_TRUE(error != NULL);
    EXPECT_EQ(error->code, EFAULT);
    EXPECT_EQ(figure_error_text(error, "blot_figure_plot"), "fig is NULL: Bad address");
    g_clear_error(&error);
}

/* The layers array can't grow: blot_figure_plot() frees the new layer,
 * and returns false with ENOMEM, the figure unchanged.  A layer count so
 * large that (count + 1) * sizeof(blot_layer *) overflows gsize makes the
 * resize fail without allocating anything; g_realloc_n() would abort, so
 * this runs isolated.  The child's LeakSanitizer check (sanitize job)
 * finds the layer if it leaks. */
TEST(Figure, plot_layers_resize_failure)
{
    RUN_ISOLATED(10, {
        const int32_t ys[] = {4, 5, 6};
        GError *error = NULL;
        blot_figure *fig = blot_figure_new(&error);
        ASSERT_TRUE(fig != NULL);

        ASSERT_TRUE(blot_figure_plot(fig, BLOT_LINE, BLOT_DATA_INT32, 3, NULL, ys, 2, "first", &error));
        blot_layer **layers = fig->layers;
        blot_layer *first = fig->layers[0];

        const gsize huge = G_MAXSIZE / sizeof(blot_layer *);
        fig->layer_count = huge;

        EXPECT_FALSE(blot_figure_plot(fig, BLOT_SCATTER, BLOT_DATA_INT32, 3, NULL, ys, 1, "new", &error));
        ASSERT_TRUE(error != NULL);
        EXPECT_EQ(error->code, ENOMEM);
        gchar *text = g_strdup_printf("cannot grow the layers array to %" G_GSIZE_FORMAT
                                      " layers: Cannot allocate memory", huge + 1);
        EXPECT_EQ(figure_error_text(error, "blot_figure_plot"), text);
        g_free(text);
        g_clear_error(&error);

        EXPECT_EQ(fig->layer_count, huge);
        EXPECT_EQ(fig->layers, layers);

        fig->layer_count = 1;
        EXPECT_EQ(fig->layers[0], first);
        blot_figure_delete(fig);
    });
}
