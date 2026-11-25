#include "blot_test.hpp"

#include "blot_color.h"

TEST(Color, mkcol_constant_fg)
{
    ASSERT_STREQ(mkcol_constant(COL_FG_PREFIX, DIM_BLACK  ), "\x1B[38;5;0m");
    ASSERT_STREQ(mkcol_constant(COL_FG_PREFIX, DIM_RED    ), "\x1B[38;5;1m");
    ASSERT_STREQ(mkcol_constant(COL_FG_PREFIX, DIM_GREEN  ), "\x1B[38;5;2m");
    ASSERT_STREQ(mkcol_constant(COL_FG_PREFIX, DIM_YELLOW ), "\x1B[38;5;3m");
    ASSERT_STREQ(mkcol_constant(COL_FG_PREFIX, DIM_BLUE   ), "\x1B[38;5;4m");
    ASSERT_STREQ(mkcol_constant(COL_FG_PREFIX, DIM_MAGENTA), "\x1B[38;5;5m");
    ASSERT_STREQ(mkcol_constant(COL_FG_PREFIX, DIM_CYAN   ), "\x1B[38;5;6m");
    ASSERT_STREQ(mkcol_constant(COL_FG_PREFIX, DIM_WHITE  ), "\x1B[38;5;7m");

    ASSERT_STREQ(mkcol_constant(COL_FG_PREFIX, BRIGHT_BLACK  ), "\x1B[38;5;8m");
    ASSERT_STREQ(mkcol_constant(COL_FG_PREFIX, BRIGHT_RED    ), "\x1B[38;5;9m");
    ASSERT_STREQ(mkcol_constant(COL_FG_PREFIX, BRIGHT_GREEN  ), "\x1B[38;5;10m");
    ASSERT_STREQ(mkcol_constant(COL_FG_PREFIX, BRIGHT_YELLOW ), "\x1B[38;5;11m");
    ASSERT_STREQ(mkcol_constant(COL_FG_PREFIX, BRIGHT_BLUE   ), "\x1B[38;5;12m");
    ASSERT_STREQ(mkcol_constant(COL_FG_PREFIX, BRIGHT_MAGENTA), "\x1B[38;5;13m");
    ASSERT_STREQ(mkcol_constant(COL_FG_PREFIX, BRIGHT_CYAN   ), "\x1B[38;5;14m");
    ASSERT_STREQ(mkcol_constant(COL_FG_PREFIX, BRIGHT_WHITE  ), "\x1B[38;5;15m");
}

TEST(Color, mkcol_constant_bg)
{
    ASSERT_STREQ(mkcol_constant(COL_BG_PREFIX, BLACK  ), "\x1B[48;5;0m");
    ASSERT_STREQ(mkcol_constant(COL_BG_PREFIX, RED    ), "\x1B[48;5;1m");
    ASSERT_STREQ(mkcol_constant(COL_BG_PREFIX, GREEN  ), "\x1B[48;5;2m");
    ASSERT_STREQ(mkcol_constant(COL_BG_PREFIX, YELLOW ), "\x1B[48;5;3m");
    ASSERT_STREQ(mkcol_constant(COL_BG_PREFIX, BLUE   ), "\x1B[48;5;4m");
    ASSERT_STREQ(mkcol_constant(COL_BG_PREFIX, MAGENTA), "\x1B[48;5;5m");
    ASSERT_STREQ(mkcol_constant(COL_BG_PREFIX, CYAN   ), "\x1B[48;5;6m");
    ASSERT_STREQ(mkcol_constant(COL_BG_PREFIX, WHITE  ), "\x1B[48;5;7m");
}

TEST(Color, runtime_fg)
{
    std::vector<const char *> expected = {
        "\x1B[38;5;0m",
        "\x1B[38;5;1m",
        "\x1B[38;5;2m",
        "\x1B[38;5;3m",
        "\x1B[38;5;4m",
        "\x1B[38;5;5m",
        "\x1B[38;5;6m",
        "\x1B[38;5;7m",

        "\x1B[38;5;8m",
        "\x1B[38;5;9m",
        "\x1B[38;5;10m",
        "\x1B[38;5;11m",
        "\x1B[38;5;12m",
        "\x1B[38;5;13m",
        "\x1B[38;5;14m",
        "\x1B[38;5;15m",
    };

    std::vector<std::string> generated;
    for (int col=0; col<(int)expected.size(); ++col) {
        generated.push_back(fg(col));
    }

    for (int col=0; col<(int)expected.size(); ++col) {
        ASSERT_STREQ(generated[col].c_str(), expected[col]) << " col=" << col;
    }
}

TEST(Color, runtime_bg)
{
    std::vector<const char *> expected = {
        "\x1B[48;5;0m",
        "\x1B[48;5;1m",
        "\x1B[48;5;2m",
        "\x1B[48;5;3m",
        "\x1B[48;5;4m",
        "\x1B[48;5;5m",
        "\x1B[48;5;6m",
        "\x1B[48;5;7m",
    };

    std::vector<std::string> generated;
    for (int col=0; col<(int)expected.size(); ++col) {
        generated.push_back(bg(col));
    }

    for (int col=0; col<(int)expected.size(); ++col) {
        ASSERT_STREQ(generated[col].c_str(), expected[col]) << " col=" << col;
    }
}

TEST(Color, constant_fg)
{
    ASSERT_STREQ(fg(DIM_BLACK  ), "\x1B[38;5;0m");
    ASSERT_STREQ(fg(DIM_RED    ), "\x1B[38;5;1m");
    ASSERT_STREQ(fg(DIM_GREEN  ), "\x1B[38;5;2m");
    ASSERT_STREQ(fg(DIM_YELLOW ), "\x1B[38;5;3m");
    ASSERT_STREQ(fg(DIM_BLUE   ), "\x1B[38;5;4m");
    ASSERT_STREQ(fg(DIM_MAGENTA), "\x1B[38;5;5m");
    ASSERT_STREQ(fg(DIM_CYAN   ), "\x1B[38;5;6m");
    ASSERT_STREQ(fg(DIM_WHITE  ), "\x1B[38;5;7m");

    ASSERT_STREQ(fg(BRIGHT_BLACK  ), "\x1B[38;5;8m");
    ASSERT_STREQ(fg(BRIGHT_RED    ), "\x1B[38;5;9m");
    ASSERT_STREQ(fg(BRIGHT_GREEN  ), "\x1B[38;5;10m");
    ASSERT_STREQ(fg(BRIGHT_YELLOW ), "\x1B[38;5;11m");
    ASSERT_STREQ(fg(BRIGHT_BLUE   ), "\x1B[38;5;12m");
    ASSERT_STREQ(fg(BRIGHT_MAGENTA), "\x1B[38;5;13m");
    ASSERT_STREQ(fg(BRIGHT_CYAN   ), "\x1B[38;5;14m");
    ASSERT_STREQ(fg(BRIGHT_WHITE  ), "\x1B[38;5;15m");
}

TEST(Color, constant_bg)
{
    ASSERT_STREQ(bg(BLACK  ), "\x1B[48;5;0m");
    ASSERT_STREQ(bg(RED    ), "\x1B[48;5;1m");
    ASSERT_STREQ(bg(GREEN  ), "\x1B[48;5;2m");
    ASSERT_STREQ(bg(YELLOW ), "\x1B[48;5;3m");
    ASSERT_STREQ(bg(BLUE   ), "\x1B[48;5;4m");
    ASSERT_STREQ(bg(MAGENTA), "\x1B[48;5;5m");
    ASSERT_STREQ(bg(CYAN   ), "\x1B[48;5;6m");
    ASSERT_STREQ(bg(WHITE  ), "\x1B[48;5;7m");
}

/* #11: fg() and bg() of a runtime colour, every value; the colour is taken
 * modulo 256 as before */
TEST(Color, runtime_all_values)
{
    for (int col = 0; col < 512; col++) {
        volatile int c = col;
        char efg[COL_BUF_LEN], ebg[COL_BUF_LEN];
        snprintf(efg, sizeof(efg), "\x1B[38;5;%dm", col & 0xFF);
        snprintf(ebg, sizeof(ebg), "\x1B[48;5;%dm", col & 0xFF);
        ASSERT_STREQ(fg(c), efg) << " col=" << col;
        ASSERT_STREQ(bg(c), ebg) << " col=" << col;
    }
}

/* B17 (#11): fg() of a runtime colour used to alloca() 16 bytes per call,
 * released only when the calling function returns: 2 million calls in one
 * function overflowed the default 8 MB stack.  It needs a default stack
 * limit to fail without the fix: under ulimit -s 64 MB or more (or
 * unlimited) it passes either way. */
TEST(Color, runtime_colour_in_a_loop_uses_no_stack)
{
    RUN_ISOLATED(30, {
        volatile int col = 1;
        unsigned long sum = 0;
        for (unsigned i = 0; i < 2000000; i++)
            sum += strlen(fg(col)) + strlen(bg(col));
        EXPECT_NE(sum, 0u);
    });
}
