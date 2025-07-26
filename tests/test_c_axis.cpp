#include <gtest/gtest.h>

#include "blot_axis.h"
#include "blot_error.h"

TEST(Axis, alloc_new_delete)
{
    GError *error = NULL;
    blot_axis *axis = blot_axis_new(true, true, 1, 100, 0, 100, NULL, &error);
    ASSERT_TRUE(axis != NULL);
    ASSERT_TRUE(error == NULL);
    blot_axis_delete(axis);
}

TEST(Axis, get_tick_at_valid_index)
{
    GError *error = NULL;
    blot_axis *axis = blot_axis_new(true, true, 1, 100, 0, 100, NULL, &error);
    ASSERT_TRUE(axis != NULL);

    const blot_axis_tick *tick = blot_axis_get_tick_at(axis, 0, &error);
    ASSERT_TRUE(tick != NULL);
    ASSERT_TRUE(error == NULL);

    blot_axis_delete(axis);
}

TEST(Axis, get_tick_at_invalid_index)
{
    GError *error = NULL;
    blot_axis *axis = blot_axis_new(true, true, 1, 100, 0, 100, NULL, &error);
    ASSERT_TRUE(axis != NULL);

    const blot_axis_tick *tick = blot_axis_get_tick_at(axis, 101, &error);
    ASSERT_TRUE(tick == NULL);
    ASSERT_TRUE(error == NULL); // Should not set error for out of bounds, just return NULL

    blot_axis_delete(axis);
}

TEST(Axis, get_tick_at_invisible_axis)
{
    GError *error = NULL;
    blot_axis *axis = blot_axis_new(true, false, 1, 100, 0, 100, NULL, &error);
    ASSERT_TRUE(axis != NULL);

    const blot_axis_tick *tick = blot_axis_get_tick_at(axis, 50, &error);
    ASSERT_TRUE(tick == NULL);
    ASSERT_TRUE(error == NULL);

    blot_axis_delete(axis);
}

TEST(Axis, get_tick_with_labels)
{
    GError *error = NULL;
    char *labels[] = { (char*)"Label0", (char*)"Label1", (char*)"Label2" };
    blot_strv strv_labels = { .count = 3, .strings = labels };

    blot_axis *axis = blot_axis_new(true, true, 1, 3, 0, 2, &strv_labels, &error);
    ASSERT_TRUE(axis != NULL);

    const blot_axis_tick *tick = blot_axis_get_tick_at(axis, 1, &error);
    ASSERT_TRUE(tick != NULL);
    ASSERT_STREQ(tick->label, "Label1");

    blot_axis_delete(axis);
}

TEST(Axis, new_invalid_screen_length)
{
    GError *error = NULL;
    blot_axis *axis = blot_axis_new(true, true, 1, 0, 0, 100, NULL, &error);
    ASSERT_TRUE(axis == NULL);
    ASSERT_TRUE(error != NULL);
    g_clear_error(&error);
}

TEST(Axis, new_invalid_data_range)
{
    GError *error = NULL;
    blot_axis *axis = blot_axis_new(true, true, 1, 100, 100, 0, NULL, &error);
    ASSERT_TRUE(axis == NULL);
    ASSERT_TRUE(error != NULL);
    g_clear_error(&error);
}
