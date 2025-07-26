#include <gtest/gtest.h>
#include <cstdint>
#include <vector>

#include "blot.hpp"

TEST(Figure, alloc_new_delete)
{
    ASSERT_NO_THROW({
        Blot::Figure fig;
    });
}

TEST(Figure, set_axis_color)
{
    Blot::Figure fig;
    ASSERT_NO_THROW(fig.set_axis_color(10));
    ASSERT_EQ(fig.axis_color, 10);
}

TEST(Figure, set_screen_size)
{
    Blot::Figure fig;
    ASSERT_NO_THROW(fig.set_screen_size(100, 50));
    ASSERT_EQ(fig.dim.cols, 100);
    ASSERT_EQ(fig.dim.rows, 50);

    // Test invalid size
    ASSERT_THROW(fig.set_screen_size(0, 0), Blot::Exception);
}

TEST(Figure, set_x_limits)
{
    Blot::Figure fig;
    ASSERT_NO_THROW(fig.set_x_limits(-10, 10));
    ASSERT_EQ(fig.lim.x_min, -10);
    ASSERT_EQ(fig.lim.x_max, 10);

    // Test invalid limits
    ASSERT_THROW(fig.set_x_limits(10, -10), Blot::Exception);
}

TEST(Figure, set_y_limits)
{
    Blot::Figure fig;
    ASSERT_NO_THROW(fig.set_y_limits(-20, 20));
    ASSERT_EQ(fig.lim.y_min, -20);
    ASSERT_EQ(fig.lim.y_max, 20);

    // Test invalid limits
    ASSERT_THROW(fig.set_y_limits(20, -20), Blot::Exception);
}

TEST(Figure, set_x_axis_labels)
{
    Blot::Figure fig;

    std::vector<const char *> labels = { "a", "b", "c" };
    ASSERT_NO_THROW(fig.set_x_axis_labels(labels));
    ASSERT_EQ(fig.xlabels.count, 3);
    ASSERT_STREQ(fig.xlabels.strings[0], "a");
    ASSERT_STREQ(fig.xlabels.strings[1], "b");
    ASSERT_STREQ(fig.xlabels.strings[2], "c");

    // Test invalid labels
    std::vector<const char *> empty_labels;
    ASSERT_THROW(fig.set_x_axis_labels(empty_labels), Blot::Exception);
}

TEST(Figure, add_scatter_plot)
{
    Blot::Figure fig;

    const std::vector<int32_t> xs = {1, 2, 3};
    const std::vector<int32_t> ys = {4, 5, 6};

    ASSERT_NO_THROW(fig.scatter(xs, ys, 1, "scatter"));
    ASSERT_EQ(fig.layer_count, 1);
}

TEST(Figure, add_line_plot)
{
    Blot::Figure fig;

    const std::vector<int32_t> xs = {1, 2, 3};
    const std::vector<int32_t> ys = {4, 5, 6};

    ASSERT_NO_THROW(fig.line(xs, ys, 1, "line"));
    ASSERT_EQ(fig.layer_count, 1);
}

TEST(Figure, add_bar_plot)
{
    Blot::Figure fig;

    const std::vector<int32_t> xs = {1, 2, 3};
    const std::vector<int32_t> ys = {4, 5, 6};

    ASSERT_NO_THROW(fig.bar(xs, ys, 1, "bar"));
    ASSERT_EQ(fig.layer_count, 1);
}

TEST(Figure, add_scatter_plot_different_data_types)
{
    Blot::Figure fig;

    const std::vector<float> xs_f = {1.0f, 2.0f, 3.0f};
    const std::vector<float> ys_f = {4.0f, 5.0f, 6.0f};
    ASSERT_NO_THROW(fig.scatter(xs_f, ys_f, 1, "scatter_float"));
    ASSERT_EQ(fig.layer_count, 1);

    const std::vector<double> xs_d = {7.0, 8.0, 9.0};
    const std::vector<double> ys_d = {10.0, 11.0, 12.0};
    ASSERT_NO_THROW(fig.scatter(xs_d, ys_d, 2, "scatter_double"));
    ASSERT_EQ(fig.layer_count, 2);
}

TEST(Figure, add_line_plot_different_data_types)
{
    Blot::Figure fig;

    const std::vector<float> xs_f = {1.0f, 2.0f, 3.0f};
    const std::vector<float> ys_f = {4.0f, 5.0f, 6.0f};
    ASSERT_NO_THROW(fig.line(xs_f, ys_f, 1, "line_float"));
    ASSERT_EQ(fig.layer_count, 1);

    const std::vector<double> xs_d = {7.0, 8.0, 9.0};
    const std::vector<double> ys_d = {10.0, 11.0, 12.0};
    ASSERT_NO_THROW(fig.line(xs_d, ys_d, 2, "line_double"));
    ASSERT_EQ(fig.layer_count, 2);
}

TEST(Figure, add_bar_plot_different_data_types)
{
    Blot::Figure fig;

    const std::vector<float> xs_f = {1.0f, 2.0f, 3.0f};
    const std::vector<float> ys_f = {4.0f, 5.0f, 6.0f};
    ASSERT_NO_THROW(fig.bar(xs_f, ys_f, 1, "bar_float"));
    ASSERT_EQ(fig.layer_count, 1);

    const std::vector<double> xs_d = {7.0, 8.0, 9.0};
    const std::vector<double> ys_d = {10.0, 11.0, 12.0};
    ASSERT_NO_THROW(fig.bar(xs_d, ys_d, 2, "bar_double"));
    ASSERT_EQ(fig.layer_count, 2);
}

TEST(Figure, add_multiple_layers)
{
    Blot::Figure fig;

    const std::vector<int32_t> xs1 = {1, 2, 3};
    const std::vector<int32_t> ys1 = {4, 5, 6};
    ASSERT_NO_THROW(fig.scatter(xs1, ys1, 1, "scatter1"));
    ASSERT_EQ(fig.layer_count, 1);

    const std::vector<float> xs2 = {7.0f, 8.0f, 9.0f};
    const std::vector<float> ys2 = {10.0f, 11.0f, 12.0f};
    ASSERT_NO_THROW(fig.line(xs2, ys2, 2, "line1"));
    ASSERT_EQ(fig.layer_count, 2);

    const std::vector<double> xs3 = {13.0, 14.0, 15.0};
    const std::vector<double> ys3 = {16.0, 17.0, 18.0};
    ASSERT_NO_THROW(fig.bar(xs3, ys3, 3, "bar1"));
    ASSERT_EQ(fig.layer_count, 3);
}

TEST(Figure, add_plot_null_xs)
{
    Blot::Figure fig;

    const std::vector<int32_t> ys = {4, 5, 6};
    ASSERT_NO_THROW(fig.scatter(ys, 1, "scatter"));
    ASSERT_EQ(fig.layer_count, 1);
}

TEST(Figure, add_plot_null_ys)
{
    Blot::Figure fig;

    const std::vector<int32_t> xs = {1, 2, 3};
    const std::vector<int32_t> ys = {};
    ASSERT_THROW(fig.scatter(xs, ys, 1, "scatter"), Blot::Exception);
}

TEST(Figure, add_plot_zero_count)
{
    Blot::Figure fig;

    const std::vector<int32_t> xs = {};
    const std::vector<int32_t> ys = {};
    ASSERT_THROW(fig.scatter(xs, ys, 1, "scatter"), Blot::Exception);
}
