#include <gtest/gtest.h>

extern "C" {
#include "blot_time.h"
};

TEST(BaseTest, AssertionTrue)
{
    ASSERT_TRUE(true);
    ASSERT_FALSE(false);
}

TEST(Time, time_passing)
{
    double t0 = blot_double_time();
    usleep(1000);
    double t1 = blot_double_time();
    double dt = t1 - t0;
    EXPECT_TRUE(dt > 0.001 && dt < 1) << "t0=" << t0 << " t1=" << t1 << " dt=" << dt;
}
