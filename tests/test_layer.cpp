#include <gtest/gtest.h>

extern "C" {
#include "blot_layer.h"
};

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

    blot_layer *layer = blot_layer_new(plot_type, data_type, data_count, data_xs, data_ys, data_color, data_label, &error);
    ASSERT_TRUE(layer == NULL);
    blot_layer_delete(layer);
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
}

