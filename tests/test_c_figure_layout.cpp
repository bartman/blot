/* limits, margins, sizes (#2, group F): observed through screen->mrg,
 * screen->dim and the rendered lines.  Flags F unless stated. */
#include "blot_test.hpp"

#include <cstdlib>

using namespace blot_test;

namespace {

/* the A6 formula */
unsigned tick_count(unsigned len, bool vertical)
{
    unsigned n = vertical ? len / 3 : len / 10;
    return (n & 1) ? n : n + 1;
}

/* every Y tick row: the text before the axis character is a complete,
 * right-aligned label whose value is the row's value */
void expect_y_labels_exact(const Rendered &r, double y_min, double y_max)
{
    ASSERT_TRUE(r.ok) << r.message;
    double range = y_max - y_min;
    unsigned rows = r.plot_rows();
    auto ticks = y_ticks(r);
    EXPECT_FALSE(ticks.empty());
    for (const auto &t : ticks) {
        const std::string &label = t.second;
        char *end = NULL;
        double v = strtod(label.c_str(), &end);
        EXPECT_TRUE(end && *end == 0 && !label.empty()) << "label '" << label << "'";
        /* right-aligned and within the margin.  It may fill it: the
         * screen formats a long label again for the margin instead of
         * cutting it (#13), and a cut would fail the value checks below */
        EXPECT_LE(label.size(), r.mrg.left - 2) << "label '" << label << "' wider than the margin";
        /* the tick's value lies in its row's cell [e, e + cell); eps
         * covers the label's rounding, so a label one row off fails */
        double e = y_min + t.first * range / rows, cell = range / rows;
        double eps = std::max(0.0006, 1e-5 * std::fabs(e));
        EXPECT_GE(v, e - eps) << "row " << t.first << " label '" << label << "' cell [" << e << "," << e + cell << ")";
        EXPECT_LT(v, e + cell + eps) << "row " << t.first << " label '" << label << "' cell [" << e << "," << e + cell << ")";
    }
}

/* every Y tick row: the label is a complete number (no '#' marker), and
 * its value is in the row's cell [e, e + cell), give or take half a unit
 * of the label's last digit: on a narrow margin the label keeps fewer
 * digits (#13), but it is never cut to a wrong number */
void expect_y_labels_in_cell(const Rendered &r, double y_min, double y_max)
{
    ASSERT_TRUE(r.ok) << r.message;
    double range = y_max - y_min, cell = range / r.plot_rows();
    auto ticks = y_ticks(r);
    EXPECT_FALSE(ticks.empty());
    for (const auto &t : ticks) {
        const std::string &label = t.second;
        char *end = NULL;
        double v = strtod(label.c_str(), &end);
        ASSERT_TRUE(end && *end == 0 && !label.empty()) << "row " << t.first << " label '" << label << "'";
        /* half a unit of the last printed digit */
        size_t epos = label.find('e'), dot = label.find('.');
        int exp10 = epos == std::string::npos ? 0 : atoi(label.c_str() + epos + 1);
        size_t mant_end = epos == std::string::npos ? label.size() : epos;
        int decimals = dot == std::string::npos ? 0 : (int)(mant_end - dot - 1);
        double half = 0.5 * pow(10, exp10 - decimals) * (1 + 1e-9);
        double e = y_min + t.first * cell;
        EXPECT_GE(v, e - half) << "row " << t.first << " label '" << label << "' cell [" << e << "," << e + cell << ")";
        EXPECT_LT(v, e + cell + half) << "row " << t.first << " label '" << label << "' cell [" << e << "," << e + cell << ")";
    }
}

/* a figure with a line across the limits */
struct LineAcross {
    double xs[2], ys[2];
    Fig f;
    LineAcross(unsigned cols, unsigned rows, double x0, double x1, double y0, double y1)
        : xs{ x0, x1 }, ys{ y0, y1 }, f(cols, rows)
    {
        f.lim(x0, x1, y0, y1);
        f.plot(BLOT_LINE, BLOT_DATA_DOUBLE, 2, xs, ys);
    }
};

const double F1_LIMITS[][3] = {
    /* y_min, y_max, mrg.left */
    { 0, 9, 8 },        { 0, 10, 9 },      { -1, 1, 9 },      { -10, 10, 10 },
    { 0, 99.9, 9 },     { 0, 100, 10 },    { 0, 1000, 11 },   { -1000, 0, 12 },
    { 0.001, 0.002, 8 }, { -0.5, 0.5, 9 }, { 0, 1e6, 14 },    { -1e6, 1e6, 15 },
};

/* sizes of the F11/F20 sweep */
const unsigned SWEEP_COLS[] = { 18, 19, 20, 21, 22, 23, 29, 30, 31, 39, 40, 41, 79, 80, 81,
                                100, 101, 255, 256, 257, 999, 1000 };
const unsigned SWEEP_ROWS[] = { 12, 13, 14, 15, 16, 17, 23, 24, 25, 50, 99, 100, 101 };

const gint32 DIAG_X[] = { 0, 9 }, DIAG_Y[] = { 0, 9 };

} // namespace

TEST(Layout, left_margin_follows_y_magnitude)
{
    for (const auto &l : F1_LIMITS) {
        SCOPED_TRACE(testing::Message() << "y " << l[0] << ".." << l[1]);
        LineAcross la(80, 20, 0, 1, l[0], l[1]);
        Rendered r = render(la.f, F);
        ASSERT_TRUE(r.ok) << r.message;
        EXPECT_EQ(r.mrg.left, (unsigned)l[2]);
        EXPECT_EQ(r.mrg.bottom, 2u);
        EXPECT_EQ(r.mrg.right, 0u);
        EXPECT_EQ(r.mrg.top, 0u);
    }
}

TEST(Layout, axis_flags_remove_margins)
{
    Fig f;
    fix_a_scatter(f);
    Rendered r = render(f, F | BLOT_RENDER_NO_Y_AXIS);
    ASSERT_TRUE(r.ok) << r.message;
    EXPECT_EQ(r.mrg.left, 0u);
    EXPECT_EQ(r.mrg.bottom, 2u);

    r = render(f, F | BLOT_RENDER_NO_X_AXIS);
    ASSERT_TRUE(r.ok) << r.message;
    EXPECT_EQ(r.mrg.left, 8u);
    EXPECT_EQ(r.mrg.bottom, 0u);

    r = render(f, F | BLOT_RENDER_NO_X_AXIS | BLOT_RENDER_NO_Y_AXIS);
    ASSERT_TRUE(r.ok) << r.message;
    EXPECT_EQ(r.mrg.left, 0u);
    EXPECT_EQ(r.mrg.bottom, 0u);
}

TEST(Layout, y_labels_never_truncated)
{
    std::vector<std::pair<double, double>> limits;
    for (const auto &l : F1_LIMITS)
        limits.push_back({ l[0], l[1] });
    for (auto l : std::vector<std::pair<double, double>>{
             { -0.1, 0.1 }, { 0, 0.05 }, { -0.05, 100 }, { -1000, 0.05 }, { -9.9996, 9.9996 },
             { 0, 999.9999 }, { -99999.5, 99999.5 }, { -1e15, 1e15 }, { 0, 1e300 } })
        limits.push_back(l);

    const blot_dimensions sizes[] = { { 80, 20 }, { 40, 14 }, { 200, 60 } };
    for (const auto &sz : sizes) {
        for (const auto &l : limits) {
            SCOPED_TRACE(testing::Message() << sz.cols << "x" << sz.rows << " y " << l.first << ".." << l.second);
            LineAcross la(sz.cols, sz.rows, 0, 1, l.first, l.second);
            Rendered r = render(la.f, F);
            ASSERT_TRUE(r.ok) << r.message;
            for (unsigned i = 0; i < r.plot_rows(); i++)
                EXPECT_EQ(r.lines[r.mrg.top + i].size(), sz.cols) << "plot row " << i;
            expect_y_labels_exact(r, l.first, l.second);
        }
    }
}

/* #13 review M1: a 14-digit label used to be cut by the screen to a
 * number 10x off (11994000000000 -> 1199400000000) */
TEST(Layout, y_labels_large_values_narrow_screen)
{
    const double xs[] = { 0, 1 }, ys[] = { 1e13, 2e13 };
    Fig f(30, 14);
    f.plot(BLOT_LINE, BLOT_DATA_DOUBLE, 2, xs, ys);
    Rendered r = render(f, F);
    /* auto limits: the data padded by 0.1% of the range */
    expect_y_labels_in_cell(r, 1e13 - 1e10, 2e13 + 1e10);
}

TEST(Layout, y_labels_large_values_column_sweep)
{
    const std::pair<double, double> limits[] = {
        { 0, 12345678901.0 }, { 1e13, 2e13 }, { -2e13, -1e13 }, { 1e14, 3e14 },
        { -3e14, -1e14 }, { -1e15, 1e15 }, { 0, 9e15 },
    };
    for (unsigned cols = 20; cols <= 40; cols++)
        for (const auto &l : limits) {
            SCOPED_TRACE(testing::Message() << cols << "x14 y " << l.first << ".." << l.second);
            LineAcross la(cols, 14, 0, 1, l.first, l.second);
            Rendered r = render(la.f, F);
            expect_y_labels_in_cell(r, l.first, l.second);
        }
}

/* B15 (#10), fixed by #13: the screen used to cut these labels */
TEST(Layout, y_labels_on_narrow_screen)
{
    const double xs[] = { 0, 1 }, ys[] = { 0, 12345678901.0 };
    Fig f(20, 14);
    f.plot(BLOT_LINE, BLOT_DATA_DOUBLE, 2, xs, ys);
    Rendered r = render(f, F);
    /* auto limits: the data padded by 0.1% */
    double pad = 12345678901.0 * 0.001;
    expect_y_labels_exact(r, -pad, 12345678901.0 + pad);
}

TEST(Layout, single_value_padding)
{
    {
        /* 10% either side */
        const double xs[] = { 5 }, ys[] = { -250 };
        Fig f(40, 14);
        f.plot(BLOT_SCATTER, BLOT_DATA_DOUBLE, 1, xs, ys);
        Rendered r = render(f, F);
        ASSERT_TRUE(r.ok) << r.message;
        EXPECT_EQ(labels_of(x_ticks(r)), std::vector<std::string>({ "4.500", "4.833", "5.167" }));
        EXPECT_EQ(labels_of(y_ticks(r)),
                  std::vector<std::string>({ "-275.000", "-265.000", "-255.000", "-245.000", "-235.000" }));
    }
    {
        /* 0.1 either side of zero */
        const double xs[] = { 0 }, ys[] = { 0 };
        Fig f(40, 14);
        f.plot(BLOT_SCATTER, BLOT_DATA_DOUBLE, 1, xs, ys);
        Rendered r = render(f, F);
        ASSERT_TRUE(r.ok) << r.message;
        EXPECT_EQ(labels_of(x_ticks(r)), std::vector<std::string>({ "-0.067", "0.000", "0.067" }));
        EXPECT_EQ(labels_of(y_ticks(r)),
                  std::vector<std::string>({ "-0.080", "-0.040", "0.000", "0.040", "0.080" }));
    }
}

namespace {
const gint32 S3_X[] = { 0, 5, 10 }, S3_Y[] = { 0, 5, 10 };

double max_label(const std::vector<std::pair<unsigned, std::string>> &ticks)
{
    double m = -INFINITY;
    for (const auto &t : ticks)
        m = std::max(m, strtod(t.second.c_str(), NULL));
    return m;
}
} // namespace

// BUG B3: https://git.jukie.net/bart/blot/-/work_items/7
TEST(Layout, DISABLED_only_y_limits_are_honoured)
{
    Fig f(40, 16);
    f.ylim(0, 100);
    f.plot(BLOT_SCATTER, BLOT_DATA_INT32, 3, S3_X, S3_Y);
    Rendered r = render(f, F);
    ASSERT_TRUE(r.ok) << r.message;
    EXPECT_GE(max_label(y_ticks(r)), 50);
    EXPECT_EQ(labels_of(x_ticks(r)), std::vector<std::string>({ "0.000", "3.340", "6.680" }));
}

// BUG B3: https://git.jukie.net/bart/blot/-/work_items/7
TEST(Layout, DISABLED_only_x_limits_are_honoured)
{
    Fig f(40, 16);
    f.xlim(0, 100);
    f.plot(BLOT_SCATTER, BLOT_DATA_INT32, 3, S3_X, S3_Y);
    Rendered r = render(f, F);
    ASSERT_TRUE(r.ok) << r.message;
    EXPECT_GE(max_label(x_ticks(r)), 30);
}

TEST(Layout, explicit_limits_exact_ticks)
{
    LineAcross la(40, 14, 0, 1, -9.9996, 9.9996);
    Rendered r = render(la.f, F);
    ASSERT_TRUE(r.ok) << r.message;
    EXPECT_EQ(labels_of(y_ticks(r)), std::vector<std::string>({ "-8.000", "-4.000", "0.000", "4.000", "8.000" }));
    EXPECT_EQ(labels_of(x_ticks(r)), std::vector<std::string>({ "0.000", "0.333", "0.667" }));
}

TEST(Layout, screen_size_bounds)
{
    const struct {
        unsigned cols, rows;
        bool ok;
    } cases[] = {
        { 9, 12, false }, { 12, 9, false }, { 1001, 10, false }, { 10, 1001, false },
        { 10, 10, true }, { 1000, 1000, true },
    };
    for (const auto &c : cases) {
        SCOPED_TRACE(testing::Message() << c.cols << "x" << c.rows);
        Fig f;
        GError *error = NULL;
        EXPECT_EQ(blot_figure_set_screen_size(f, c.cols, c.rows, &error), c.ok);
        if (c.ok) {
            EXPECT_TRUE(error == NULL);
        } else {
            ASSERT_TRUE(error != NULL);
            EXPECT_EQ(error->code, EINVAL);
        }
        g_clear_error(&error);
    }
}

TEST(Layout, minimum_renderable_sizes)
{
    const struct {
        unsigned cols, rows;
        blot_render_flags extra;
        bool ok;
    } cases[] = {
        { 18, 12, BLOT_RENDER_NONE, true },
        { 17, 12, BLOT_RENDER_NONE, false },  /* plot area under 10 columns */
        { 18, 11, BLOT_RENDER_NONE, false },  /* plot area under 10 rows */
        { 10, 10, BLOT_RENDER_NONE, false },
        { 10, 10, BLOT_RENDER_NO_X_AXIS | BLOT_RENDER_NO_Y_AXIS, true },
        { 10, 12, BLOT_RENDER_NO_Y_AXIS, true },
        { 18, 10, BLOT_RENDER_NO_X_AXIS, true },
        { 1000, 1000, BLOT_RENDER_NONE, true },
    };
    for (const auto &c : cases) {
        SCOPED_TRACE(testing::Message() << c.cols << "x" << c.rows << " flags " << c.extra);
        Fig f(c.cols, c.rows);
        f.lim(0, 9, 0, 9);
        f.plot(BLOT_LINE, BLOT_DATA_INT32, 2, DIAG_X, DIAG_Y);
        Rendered r = render(f, F | c.extra);
        ASSERT_EQ(r.ok, c.ok) << r.message;
        if (!c.ok) {
            EXPECT_EQ(r.code, EINVAL) << r.message;
            continue;
        }
        EXPECT_EQ(r.lines.size(), c.rows);
        for (unsigned i = 0; i < r.plot_rows(); i++)
            EXPECT_EQ(r.lines[r.mrg.top + i].size(), c.cols) << "plot row " << i;
        if (c.cols == 18 && c.rows == 12) {
            EXPECT_EQ(r.mrg.left, 8u);
            EXPECT_EQ(r.mrg.bottom, 2u);
        }
    }
}

TEST(Layout, size_sweep_invariants)
{
    const blot_render_flags flag_sets[] = { F, BLOT_RENDER_NO_COLOR | BLOT_RENDER_BRAILLE };
    for (auto flags : flag_sets) {
        for (unsigned cols : SWEEP_COLS) {
            for (unsigned rows : SWEEP_ROWS) {
                SCOPED_TRACE(testing::Message() << cols << "x" << rows << " flags " << flags);
                Fig f(cols, rows);
                f.lim(0, 9, 0, 9);
                f.plot(BLOT_LINE, BLOT_DATA_INT32, 2, DIAG_X, DIAG_Y);
                Rendered r = render(f, flags);
                ASSERT_TRUE(r.ok) << r.message;
                ASSERT_EQ(r.lines.size(), rows);
                unsigned prow = r.plot_rows(), pcol = r.plot_cols();
                for (unsigned i = 0; i <= prow; i++)   /* plot rows and the axis row */
                    ASSERT_EQ(r.lines[r.mrg.top + i].size(), cols) << "line " << r.mrg.top + i;
                ASSERT_LE(r.lines[r.mrg.top + prow + 1].size(), cols) << "label row";

                Cells marks = plot_cells(r);
                EXPECT_TRUE(marks.count({ 0, 0 })) << "bottom-left cell";
                bool far = false;
                for (const auto &c : marks)
                    far |= c.first == pcol - 1 || c.second == prow - 1;
                EXPECT_TRUE(far) << "no mark in the last column or the top row";

                EXPECT_EQ(y_ticks(r).size(), tick_count(prow, true));
                EXPECT_EQ(x_ticks(r).size(), tick_count(pcol, false));
                if (HasFailure())
                    return;
            }
        }
    }
}

TEST(Layout, terminal_size_fallback)
{
    RUN_ISOLATED(10, {
        GError *error = NULL;
        ASSERT_TRUE(blot_terminal_set_size({ 80, 24 }, &error));
        Fig f;  /* no set_screen_size */
        f.lim(0, 9, 0, 9);
        f.plot(BLOT_LINE, BLOT_DATA_INT32, 2, DIAG_X, DIAG_Y);
        Rendered r = render(f, F);
        ASSERT_TRUE(r.ok) << r.message;
        EXPECT_EQ(r.dim.cols, 78u);
        EXPECT_EQ(r.dim.rows, 22u);
        EXPECT_EQ(r.lines.size(), 22u);

        /* the 2-column border is not taken below the minimum size */
        ASSERT_TRUE(blot_terminal_set_size({ 10, 10 }, &error));
        r = render(f, F | BLOT_RENDER_NO_X_AXIS | BLOT_RENDER_NO_Y_AXIS);
        ASSERT_TRUE(r.ok) << r.message;
        EXPECT_EQ(r.dim.cols, 10u);
        EXPECT_EQ(r.dim.rows, 10u);

        const blot_dimensions bad[] = { { 9, 24 }, { 1001, 24 }, { 80, 9 }, { 80, 1001 } };
        for (const auto &d : bad) {
            EXPECT_FALSE(blot_terminal_set_size(d, &error)) << d.cols << "x" << d.rows;
            ASSERT_TRUE(error != NULL);
            EXPECT_EQ(error->code, ERANGE);
            g_clear_error(&error);
        }
    });
}

TEST(Layout, terminal_size_from_environment)
{
    RUN_ISOLATED(10, {
        setenv("COLUMNS", "100", 1);
        setenv("LINES", "40", 1);
        Fig f;
        f.lim(0, 9, 0, 9);
        f.plot(BLOT_LINE, BLOT_DATA_INT32, 2, DIAG_X, DIAG_Y);
        Rendered r = render(f, F);
        ASSERT_TRUE(r.ok) << r.message;
        EXPECT_EQ(r.dim.cols, 98u);
        EXPECT_EQ(r.dim.rows, 38u);
    });
}

/* not in the design's list: no size set, none in the environment, and
 * stdout is not a terminal: the render fails instead of guessing */
TEST(Layout, terminal_size_unknown_is_an_error)
{
    RUN_ISOLATED(10, {
        unsetenv("COLUMNS");
        unsetenv("LINES");
        ASSERT_TRUE(freopen("/dev/null", "w", stdout) != NULL);
        Fig f;
        f.lim(0, 9, 0, 9);
        f.plot(BLOT_LINE, BLOT_DATA_INT32, 2, DIAG_X, DIAG_Y);
        Rendered r = render(f, F);
        EXPECT_FALSE(r.ok);
        EXPECT_EQ(r.code, ENOTTY) << r.message;
    });
}

// BUG B19: https://git.jukie.net/bart/blot/-/work_items/11
TEST(Layout, DISABLED_terminal_environment_out_of_range)
{
    /* the size is cached on first use: one child per case */
    const char *cases[][2] = { { "1", "1" }, { "5000", "24" } };
    for (const auto &c : cases) {
        SCOPED_TRACE(testing::Message() << "COLUMNS=" << c[0] << " LINES=" << c[1]);
        RUN_ISOLATED(10, {
            setenv("COLUMNS", c[0], 1);
            setenv("LINES", c[1], 1);
            Fig f;
            f.lim(0, 9, 0, 9);
            f.plot(BLOT_LINE, BLOT_DATA_INT32, 2, DIAG_X, DIAG_Y);
            Rendered r = render(f, F);
            if (r.ok) {
                EXPECT_LE(r.dim.cols, (unsigned)BLOT_MAX_COLS);
                EXPECT_LE(r.dim.rows, (unsigned)BLOT_MAX_ROWS);
                EXPECT_GE(r.dim.cols, (unsigned)BLOT_MIN_COLS);
                EXPECT_GE(r.dim.rows, (unsigned)BLOT_MIN_ROWS);
            }
        });
    }
}

// BUG B10: https://git.jukie.net/bart/blot/-/work_items/11
TEST(Layout, DISABLED_render_accepts_null_error)
{
    RUN_ISOLATED(10, {
        Fig f;
        fix_a_scatter(f);
        blot_screen *scr = blot_figure_render(f, F, NULL);
        EXPECT_TRUE(scr != NULL);
        blot_screen_delete(scr);
    });
}

TEST(Layout, render_error_codes)
{
    Fig f(28, 12);
    Rendered r = render(f, F);      /* no layers */
    EXPECT_FALSE(r.ok);
    EXPECT_EQ(r.code, EINVAL) << r.message;

    f.lim(0, 19, 0, 9);
    f.plot(BLOT_SCATTER, BLOT_DATA_INT32, 4, SC_X, SC_Y, 1, "pts");
    r = render(f, BLOT_RENDER_BRAILLE | BLOT_RENDER_NO_UNICODE);
    EXPECT_FALSE(r.ok);
    EXPECT_EQ(r.code, EINVAL) << r.message;

    r = render(f, F);
    EXPECT_TRUE(r.ok) << r.message;
    EXPECT_EQ(r.lines.size(), 12u);
}

namespace {
void expect_no_line_wider(const Rendered &r, unsigned cols)
{
    ASSERT_TRUE(r.ok) << r.message;
    for (size_t i = 0; i < r.lines.size(); i++)
        EXPECT_LE(r.lines[i].size(), cols) << "line " << i << " |" << to_utf8(r.lines[i]) << "|";
}
} // namespace

// BUG B13: https://git.jukie.net/bart/blot/-/work_items/9
TEST(Layout, DISABLED_x_label_row_fits_zero_crossing)
{
    {
        const double v[] = { -1, 0, 2 };
        Fig f(60, 20);
        f.plot(BLOT_LINE, BLOT_DATA_DOUBLE, 3, v, v);
        expect_no_line_wider(render(f, F), 60);
    }
    {
        const double v[] = { -7, 3 };
        Fig f(73, 23);
        f.plot(BLOT_LINE, BLOT_DATA_DOUBLE, 2, v, v);
        expect_no_line_wider(render(f, F), 73);
    }
}

// BUG B13: https://git.jukie.net/bart/blot/-/work_items/9
TEST(Layout, DISABLED_long_custom_x_labels)
{
    const gint32 ys[] = { 3, 1, 4, 1 };
    const char *names[] = { "first-label-xxxx", "second-label-xxx", "third-label-xxxx", "fourth-label-xxx" };
    char *labels[4];
    for (int i = 0; i < 4; i++)
        labels[i] = (char *)names[i];
    Fig f(40, 14);
    ASSERT_TRUE(blot_figure_set_x_axis_labels(f, 4, labels, NULL));
    f.plot(BLOT_BAR, BLOT_DATA_INT32, 4, NULL, ys);
    Rendered r = render(f, F);
    expect_no_line_wider(r, 40);
    auto ticks = x_ticks(r);
    ASSERT_EQ(ticks.size(), 4u);
    for (int i = 0; i < 4; i++) {
        const std::string &got = ticks[i].second;
        EXPECT_FALSE(got.empty()) << "label " << i << " missing at column " << ticks[i].first;
        EXPECT_EQ(std::string(names[i]).compare(0, got.size(), got), 0)
            << "label " << i << " at column " << ticks[i].first << " is '" << got << "'";
    }
}

// BUG B14: https://git.jukie.net/bart/blot/-/work_items/9
TEST(Layout, DISABLED_narrow_screen_large_x_values)
{
    const double xs[] = { -200000, -100000 }, ys[] = { 1, 2 };
    Fig f(28, 14);
    f.plot(BLOT_SCATTER, BLOT_DATA_DOUBLE, 2, xs, ys);
    Rendered r = render(f, F);
    EXPECT_TRUE(r.ok) << r.message;
}

// BUG B5: https://git.jukie.net/bart/blot/-/work_items/4
TEST(Layout, DISABLED_line_reaches_top_right_at_every_size)
{
    unsigned bad = 0, total = 0;
    std::ostringstream which;
    for (unsigned cols : SWEEP_COLS) {
        for (unsigned rows : SWEEP_ROWS) {
            Fig f(cols, rows);
            f.lim(0, 9, 0, 9);
            f.plot(BLOT_LINE, BLOT_DATA_INT32, 2, DIAG_X, DIAG_Y);
            Rendered r = render(f, F);
            ASSERT_TRUE(r.ok) << r.message;
            total++;
            if (!plot_cells(r).count({ r.plot_cols() - 1, r.plot_rows() - 1 })) {
                bad++;
                which << " " << cols << "x" << rows;
            }
        }
    }
    EXPECT_EQ(bad, 0u) << "top-right plot cell empty at " << bad << " of " << total << " sizes:" << which.str();
}
