#include <gtest/gtest.h>

#include "blot_utils.h"

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

/* --- number formatting and environment (#2, group U) -------------------
 *
 * blot_format_number(buf, room, v) returns the length, or -1 when nothing
 * fits in room - 1 characters. */

#include <cmath>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

namespace {

/* the text, or "-1" when it doesn't fit */
std::string fmt(double v, unsigned room)
{
    char buf[64];
    int rc = blot_format_number(buf, room, v);
    if (rc < 0)
        return "-1";
    EXPECT_EQ((size_t)rc, strlen(buf)) << buf;
    return buf;
}

const double U1_VALUES[] = { 0, 1, -1, 0.5, 123.4567, 0.1 + 0.2, 99999.9996 };
const double U2_VALUES[] = { 123.4567, 99999.9996, 1234567.891, -1234567.891, 1e15, 1e100, 0, -1 };

/* a variable of our own, so nothing else in the process depends on it */
const char *ENV = "BLOT_TEST_ENV_TO_UINT";

} // namespace

TEST(Utils, format_number_fixed_3)
{
    const char *expected[] = { "0.000", "1.000", "-1.000", "0.500", "123.457", "0.300", "100000.000" };
    for (size_t i = 0; i < G_N_ELEMENTS(U1_VALUES); i++)
        EXPECT_EQ(fmt(U1_VALUES[i], 16), expected[i]) << U1_VALUES[i];
}

TEST(Utils, format_number_fallback_chain)
{
    const unsigned rooms[] = { 16, 10, 9, 8, 7, 6, 5, 4, 3 };
    const std::vector<std::vector<std::string>> table = {
        /* 16             10           9           8         7         6        5       4      3 */
        { "123.457",      "123.457",   "123.457",  "123.457", "123.46", "123.5", "123",  "123", "-1" },
        { "100000.000",   "100000.00", "100000.0", "100000",  "100000", "1e+05", "-1",   "-1",  "-1" },
        { "1234567.891",  "1234567.9", "1234568",  "1234568", "1e+06",  "1e+06", "-1",   "-1",  "-1" },
        { "-1234567.891", "-1234568",  "-1234568", "-1e+06",  "-1e+06", "-1",    "-1",   "-1",  "-1" },
        { "1e+15",        "1e+15",     "1e+15",    "1e+15",   "1e+15",  "1e+15", "-1",   "-1",  "-1" },
        { "1e+100",       "1e+100",    "1e+100",   "1e+100",  "1e+100", "-1",    "-1",   "-1",  "-1" },
        { "0.000",        "0.000",     "0.000",    "0.000",   "0.000",  "0.000", "0.00", "0.0", "0" },
        { "-1.000",       "-1.000",    "-1.000",   "-1.000",  "-1.000", "-1.00", "-1.0", "-1",  "-1" },
    };
    for (size_t vi = 0; vi < G_N_ELEMENTS(U2_VALUES); vi++)
        for (size_t ri = 0; ri < G_N_ELEMENTS(rooms); ri++) {
            EXPECT_EQ(fmt(U2_VALUES[vi], rooms[ri]), table[vi][ri])
                << "value " << U2_VALUES[vi] << " room " << rooms[ri];
        }
}

/* #13: the chain used to go %.3g, then "%3.g" (precision 0: one digit) */
TEST(Utils, format_number_two_digits_fit)
{
    /* The chain went %.3g, then "%3.g" (precision 0: one digit), so it
     * printed 1e+06 (19% off) where 1234568 or 1.2e+06 fits.  The text
     * must be at least as precise as %.2g, the step the chain is missing:
     * within half a unit of the second significant digit (50000 here).
     * 1234568 / -1234568 (the most precise that fit) pass, and so do
     * 1.2e+06 / -1.2e+06; the test doesn't pick the fix. */
    const struct {
        double v;
        unsigned room;
    } cases[] = { { 1234567.891, 8 }, { -1234567.891, 9 } };
    for (const auto &c : cases) {
        char buf[64];
        int rc = blot_format_number(buf, c.room, c.v);
        ASSERT_GT(rc, 0) << c.v << " room " << c.room;
        EXPECT_LT(rc, (int)c.room) << buf;
        double half_unit = 0.5 * pow(10, floor(log10(fabs(c.v))) - 1);
        EXPECT_LE(fabs(strtod(buf, NULL) - c.v), half_unit)
            << c.v << " room " << c.room << " -> " << buf;
    }
}

/* #13: each step of the chain, and its boundaries.  The text is the most
 * precise that fits in room - 1 characters: fixed point with 3, 2, 1
 * decimals first, then the most precise of the integer and %.6g..%.1g
 * (the first of those on a tie), never less precise than %.1g. */
TEST(Utils, format_number_most_precise_fit)
{
    const struct {
        double v;
        unsigned room;
        const char *text; /* "-1": nothing fits */
    } cases[] = {
        /* fixed point; room == length doesn't fit (the NUL needs one) */
        { 123.4567, 8, "123.457" },
        { 123.4567, 7, "123.46" },
        { 123.4567, 6, "123.5" },
        { 123.4567, 5, "123" },  /* integer */
        { 123.4567, 4, "123" },  /* room == length + 1 */
        { 123.4567, 3, "-1" },   /* room == length */
        /* the integer in full, before any %g */
        { 1234567.891, 9, "1234568" },
        { 1234567.891, 8, "1234568" },
        { 1234567.0, 8, "1234567" },
        { -1234567.891, 9, "-1234568" },
        /* the tie-break: both texts are exact, the integer comes first */
        { 1e15, 17, "1000000000000000" },
        { 1e15, 16, "1e+15" },
        { 99999.9996, 7, "100000" }, /* %.0f and %.6g print the same text */
        /* from 2^53 on, no %.0f: its digits past the 16th are noise */
        { 9007199254740991.0, 17, "9007199254740991" }, /* 2^53 - 1 */
        { 9007199254740992.0, 17, "9.0072e+15" },       /* 2^53 */
        { 1e16, 18, "1e+16" },
        { -1e16, 19, "-1e+16" },
        { 1.5e30, 32, "1.5e+30" }, /* the legend's room; not 1500000000000000019884624838656 */
        { 99999.9996, 6, "1e+05" },
        /* %.6g down to %.1g, when the integer doesn't fit */
        { 123456789012.0, 13, "123456789012" },
        { 123456789012.0, 12, "1.23457e+11" },
        { 123456789012.0, 11, "1.2346e+11" },
        { 123456789012.0, 10, "1.235e+11" },
        { 123456789012.0, 9, "1.23e+11" },
        { 123456789012.0, 8, "1.2e+11" },
        { 123456789012.0, 7, "1e+11" },
        { 123456789012.0, 6, "1e+11" },
        { 123456789012.0, 5, "-1" },
        { -123456789012.0, 12, "-1.2346e+11" },
        { -123456789012.0, 9, "-1.2e+11" },
        { -123456789012.0, 8, "-1e+11" },
        { -123456789012.0, 7, "-1e+11" },
        { -123456789012.0, 6, "-1" },
        /* two digits instead of one where they fit */
        { -190000.0, 8, "-190000" },
        { -210000.0, 8, "-210000" },
        { -190000.0, 7, "-2e+05" }, /* -1.9e+05 needs 9 */
        /* values below 1 */
        { 0.123456, 6, "0.123" },
        { 0.123456, 5, "0.12" },
        { 0.123456, 4, "0.1" },
        { 0.123456, 3, "-1" },
        { 0.5, 3, "-1" },  /* not "0": less precise than %.1g */
        { -0.4, 3, "-1" }, /* not "-0" */
        { 0.05, 3, "-1" },
        /* rounding with a carry */
        { 9.99, 5, "9.99" },
        { 9.99, 4, "10" },
        { 9.99, 3, "10" },
        { 9.99, 2, "-1" },
        { -9.99, 6, "-9.99" },
        { -9.99, 5, "-10" },
        { -9.99, 4, "-10" },
        { -9.99, 3, "-1" },
        { 9.96, 4, "10" },
        { 99.96, 6, "99.96" },
        { 99.96, 5, "100" },
        { 99.96, 4, "100" },
        { -1.5, 3, "-2" },
    };
    for (const auto &c : cases)
        EXPECT_EQ(fmt(c.v, c.room), c.text) << "value " << c.v << " room " << c.room;
}

TEST(Utils, format_number_respects_room)
{
    std::vector<double> values(std::begin(U1_VALUES), std::end(U1_VALUES));
    values.insert(values.end(), std::begin(U2_VALUES), std::end(U2_VALUES));
    values.insert(values.end(), { NAN, INFINITY, -INFINITY });

    for (double v : values) {
        for (unsigned room = 0; room <= 16; room++) {
            char buf[32];
            memset(buf, 0x5a, sizeof(buf));
            int rc = blot_format_number(buf, room, v);
            if (room <= 1) {
                EXPECT_EQ(rc, -1) << "value " << v << " room " << room;
            }
            if (rc != -1) {
                EXPECT_GT(rc, 0) << "value " << v << " room " << room;
                EXPECT_LT(rc, (int)room) << "value " << v << " room " << room;
                EXPECT_EQ(strnlen(buf, sizeof(buf)), (size_t)rc) << "value " << v << " room " << room;
            }
            for (size_t i = room; i < sizeof(buf); i++)
                ASSERT_EQ(buf[i], 0x5a) << "value " << v << " room " << room << " wrote byte " << i;
        }
    }
}

TEST(Utils, format_number_non_finite)
{
    std::string n = fmt(NAN, 16);
    EXPECT_TRUE(n == "nan" || n == "-nan") << n;
    EXPECT_EQ(fmt(INFINITY, 16), "inf");
    EXPECT_EQ(fmt(-INFINITY, 16), "-inf");
    for (double v : { (double)NAN, (double)INFINITY, -(double)INFINITY })
        EXPECT_EQ(fmt(v, 3), "-1") << v;
}

// BUG B16: https://git.jukie.net/bart/blot/-/work_items/10
TEST(Utils, DISABLED_format_number_tiny_not_zero)
{
    for (double v : { 1e-5, 0.0004, -0.0004 }) {
        std::string s = fmt(v, 16);
        EXPECT_NE(strtod(s.c_str(), NULL), 0.0) << v << " -> " << s;
        EXPECT_NE(s, "-0.000") << v;
    }
}

TEST(Utils, env_to_uint_valid)
{
    const struct {
        const char *text;
        unsigned expected;
    } cases[] = {
        { "12", 12 }, { "0x10", 16 }, { "010", 8 }, { " 7", 7 }, { "", 77 }, { NULL, 77 }, { "-1", 77 },
    };
    for (const auto &c : cases) {
        if (c.text)
            setenv(ENV, c.text, 1);
        else
            unsetenv(ENV);
        EXPECT_EQ(blot_env_to_uint(ENV, 77), c.expected) << (c.text ? c.text : "(unset)");
    }
    unsetenv(ENV);
}

// BUG B20: https://git.jukie.net/bart/blot/-/work_items/11
TEST(Utils, DISABLED_env_to_uint_garbage)
{
    for (const char *text : { "abc", "99999999999", "4294967296" }) {
        setenv(ENV, text, 1);
        EXPECT_EQ(blot_env_to_uint(ENV, 77), 77u) << text;
    }
    unsetenv(ENV);
}
