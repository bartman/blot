#include <gtest/gtest.h>

#include "blot_color.h"

TEST(Color, mkcol_runtime_fg)
{
    colbuf(buf);

    ASSERT_STREQ(mkcol_runtime(buf, sizeof(buf), COL_FG_FMT, DIM_BLACK  ), "\x1B[38;5;0m");
    ASSERT_STREQ(mkcol_runtime(buf, sizeof(buf), COL_FG_FMT, DIM_RED    ), "\x1B[38;5;1m");
    ASSERT_STREQ(mkcol_runtime(buf, sizeof(buf), COL_FG_FMT, DIM_GREEN  ), "\x1B[38;5;2m");
    ASSERT_STREQ(mkcol_runtime(buf, sizeof(buf), COL_FG_FMT, DIM_YELLOW ), "\x1B[38;5;3m");
    ASSERT_STREQ(mkcol_runtime(buf, sizeof(buf), COL_FG_FMT, DIM_BLUE   ), "\x1B[38;5;4m");
    ASSERT_STREQ(mkcol_runtime(buf, sizeof(buf), COL_FG_FMT, DIM_MAGENTA), "\x1B[38;5;5m");
    ASSERT_STREQ(mkcol_runtime(buf, sizeof(buf), COL_FG_FMT, DIM_CYAN   ), "\x1B[38;5;6m");
    ASSERT_STREQ(mkcol_runtime(buf, sizeof(buf), COL_FG_FMT, DIM_WHITE  ), "\x1B[38;5;7m");

    ASSERT_STREQ(mkcol_runtime(buf, sizeof(buf), COL_FG_FMT, BRIGHT_BLACK  ), "\x1B[38;5;8m");
    ASSERT_STREQ(mkcol_runtime(buf, sizeof(buf), COL_FG_FMT, BRIGHT_RED    ), "\x1B[38;5;9m");
    ASSERT_STREQ(mkcol_runtime(buf, sizeof(buf), COL_FG_FMT, BRIGHT_GREEN  ), "\x1B[38;5;10m");
    ASSERT_STREQ(mkcol_runtime(buf, sizeof(buf), COL_FG_FMT, BRIGHT_YELLOW ), "\x1B[38;5;11m");
    ASSERT_STREQ(mkcol_runtime(buf, sizeof(buf), COL_FG_FMT, BRIGHT_BLUE   ), "\x1B[38;5;12m");
    ASSERT_STREQ(mkcol_runtime(buf, sizeof(buf), COL_FG_FMT, BRIGHT_MAGENTA), "\x1B[38;5;13m");
    ASSERT_STREQ(mkcol_runtime(buf, sizeof(buf), COL_FG_FMT, BRIGHT_CYAN   ), "\x1B[38;5;14m");
    ASSERT_STREQ(mkcol_runtime(buf, sizeof(buf), COL_FG_FMT, BRIGHT_WHITE  ), "\x1B[38;5;15m");
}

TEST(Color, mkcol_runtime_bg)
{
    colbuf(buf);

    ASSERT_STREQ(mkcol_runtime(buf, sizeof(buf), COL_BG_FMT, BLACK  ), "\x1B[48;5;0m");
    ASSERT_STREQ(mkcol_runtime(buf, sizeof(buf), COL_BG_FMT, RED    ), "\x1B[48;5;1m");
    ASSERT_STREQ(mkcol_runtime(buf, sizeof(buf), COL_BG_FMT, GREEN  ), "\x1B[48;5;2m");
    ASSERT_STREQ(mkcol_runtime(buf, sizeof(buf), COL_BG_FMT, YELLOW ), "\x1B[48;5;3m");
    ASSERT_STREQ(mkcol_runtime(buf, sizeof(buf), COL_BG_FMT, BLUE   ), "\x1B[48;5;4m");
    ASSERT_STREQ(mkcol_runtime(buf, sizeof(buf), COL_BG_FMT, MAGENTA), "\x1B[48;5;5m");
    ASSERT_STREQ(mkcol_runtime(buf, sizeof(buf), COL_BG_FMT, CYAN   ), "\x1B[48;5;6m");
    ASSERT_STREQ(mkcol_runtime(buf, sizeof(buf), COL_BG_FMT, WHITE  ), "\x1B[48;5;7m");
}

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
