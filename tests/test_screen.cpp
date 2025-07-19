#include <gtest/gtest.h>

extern "C" {
#include "blot_screen.h"
};

TEST(BaseTest, AssertionTrue)
{
    ASSERT_TRUE(true);
    ASSERT_FALSE(false);
}

TEST(Screen, alloc_100x100)
{
    const blot_dimensions dim{ 100, 100 };
    const blot_margins mrg{ 1, 1, 1, 1 };
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


