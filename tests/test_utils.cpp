#include <gtest/gtest.h>

extern "C" {
#include "blot_utils.h"
};

TEST(BaseTest, AssertionTrue)
{
    ASSERT_TRUE(true);
    ASSERT_FALSE(false);
}

TEST(Utils, pointer_suspect)
{
    EXPECT_TRUE(__pointer_suspect(NULL));
    EXPECT_TRUE(__pointer_suspect((void *)1));
    EXPECT_TRUE(__pointer_suspect((void *)4095));
    EXPECT_FALSE(__pointer_suspect((void *)4096));
    EXPECT_FALSE(__pointer_suspect(this));
}

TEST(Utils, offsetof)
{
    struct __attribute__((packed)) Foo {
        uint8_t  at0;
        uint16_t at1;
        uint32_t at3;
        uint8_t  at7;
    };
    EXPECT_EQ(offsetof(Foo, at0), 0);
    EXPECT_EQ(offsetof(Foo, at1), 1);
    EXPECT_EQ(offsetof(Foo, at3), 3);
    EXPECT_EQ(offsetof(Foo, at7), 7);
}

TEST(Utils, container_of)
{
    struct Inner {
        int i;
    };
    struct Outer {
        Inner inner;
    };
    Outer  outer;
    Inner *inner_ptr = &outer.inner;

    EXPECT_EQ(container_of(inner_ptr, Outer, inner), &outer);
}

TEST(Utils, ARRAY_SIZE)
{
    char one_char[1];
    EXPECT_EQ(ARRAY_SIZE(one_char), 1);
    char ten_char[10];
    EXPECT_EQ(ARRAY_SIZE(ten_char), 10);
    int one_int[1];
    EXPECT_EQ(ARRAY_SIZE(one_int), 1);
    int ten_int[10];
    EXPECT_EQ(ARRAY_SIZE(ten_int), 10);
}

TEST(Utils, ONCE)
{
    int cnt = 0;
    for (int i = 0; i < 10; i++) {
        ONCE(cnt++);
    }
    EXPECT_EQ(cnt, 1);
}

TEST(Utils, min_t)
{
    EXPECT_EQ(min_t(char, 0, 1), 0);
    EXPECT_EQ(min_t(int, 0, 1), 0);
    EXPECT_EQ(min_t(long, 0, 1), 0);

    EXPECT_EQ(min_t(char, CHAR_MIN, CHAR_MAX), CHAR_MIN);
    EXPECT_EQ(min_t(int, INT_MIN, INT_MAX), INT_MIN);
    EXPECT_EQ(min_t(long, LONG_MIN, LONG_MAX), LONG_MIN);
}

TEST(Utils, max_t)
{
    EXPECT_EQ(max_t(char, 0, 1), 1);
    EXPECT_EQ(max_t(int, 0, 1), 1);
    EXPECT_EQ(max_t(long, 0, 1), 1);

    EXPECT_EQ(max_t(char, CHAR_MIN, CHAR_MAX), CHAR_MAX);
    EXPECT_EQ(max_t(int, INT_MIN, INT_MAX), INT_MAX);
    EXPECT_EQ(max_t(long, LONG_MIN, LONG_MAX), LONG_MAX);
}

TEST(Utils, swap_t)
{
    uint32_t a = 0, b = 1;
    swap_t(int, a, b);
    EXPECT_EQ(a, 1);
    EXPECT_EQ(b, 0);

    uint64_t c = UINT64_MAX;
    swap_t(int, a, c);
    EXPECT_EQ(a, UINT32_MAX);
    EXPECT_EQ(c, 1);
}

TEST(Utils, abs_t)
{
    EXPECT_EQ(abs_t(int,0), 0);
    EXPECT_EQ(abs_t(int,1), 1);
    EXPECT_EQ(abs_t(int,-1), 1);
}
