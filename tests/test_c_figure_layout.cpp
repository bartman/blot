/* limits, margins, sizes (#2, group F): observed through screen->mrg,
 * screen->dim and the rendered lines.  Flags F unless stated. */
#include "blot_test.hpp"

#include <cstdlib>
#include <pty.h>
#include <sys/ioctl.h>
#include <thread>

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

/* #10, the rule of the X labels (#9) for the Y labels: every Y tick has a
 * label in the margin that names a value within half a cell of the tick's
 * row; no two are the same and none is a negative zero */
void expect_y_labels_half_cell(const Rendered &r, double y_min, double y_max)
{
    ASSERT_TRUE(r.ok) << r.message;
    double range = y_max - y_min, cell = range / r.plot_rows();
    auto ticks = y_ticks(r);
    EXPECT_FALSE(ticks.empty());
    std::set<std::string> seen;
    for (const auto &t : ticks) {
        const std::string &label = t.second;
        char *end = NULL;
        double v = strtod(label.c_str(), &end);
        EXPECT_TRUE(!label.empty() && end && *end == 0) << "row " << t.first << " label '" << label << "'";
        EXPECT_LE(label.size(), r.mrg.left - 2) << "label '" << label << "' wider than the margin";
        double lo = y_min + t.first * cell, eps = 0.5 * cell + 1e-9 * std::fabs(range);
        EXPECT_GE(v, lo - eps) << "row " << t.first << " label '" << label << "' cell [" << lo << "," << lo + cell << ")";
        EXPECT_LT(v, lo + cell + eps) << "row " << t.first << " label '" << label << "' cell [" << lo << "," << lo + cell << ")";
        EXPECT_FALSE(label[0] == '-' && v == 0) << "row " << t.first << " label '" << label << "'";
        EXPECT_TRUE(seen.insert(label).second) << "duplicate label " << label;
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
    { 0.001, 0.002, 9 }, { -0.5, 0.5, 9 }, { 0, 1e6, 14 },    { -1e6, 1e6, 15 },
};

/* sizes of the F11/F20 sweep */
const unsigned SWEEP_COLS[] = { 20, 21, 22, 23, 29, 30, 31, 39, 40, 41, 79, 80, 81,
                                100, 101, 255, 256, 257, 999, 1000 };
const unsigned SWEEP_ROWS[] = { 12, 13, 14, 15, 16, 17, 23, 24, 25, 50, 99, 100, 101 };

const gint32 DIAG_X[] = { 0, 9 }, DIAG_Y[] = { 0, 9 };

/* stdout is a pty whose size the test sets (an isolated child): read its
 * master end in the background, so that a child printing more than the
 * pty buffer (gtest's failure messages) doesn't block until the alarm */
void drain_pty(int master)
{
    std::thread([master]() {
        char buf[4096];
        while (read(master, buf, sizeof(buf)) > 0)
            ;
    }).detach();
}

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

/* #10: the Y labels of a sweep of sizes and data, by the rule of the X
 * labels (#9): within half a cell of their row.  The narrow screens cap
 * the margin at half the columns, where the screen used to format a label
 * again for the room with no such bound (#13), and the tiny ranges gave
 * 0.000 for every tick. */
TEST(Layout, y_labels_within_half_a_cell_sweep)
{
    const double RANGES[][2] = {
        { 0, 12345678901.0 }, { 1e13, 2e13 }, { -3e14, -1e14 }, { -1e15, 1e15 }, { 0, 9e15 },
        { -0.01003, 0.02003 }, { -0.0019955, 0.0194945 }, { 0.001, 0.002 }, { -0.0005, 0.0005 },
        { 1e-9, 3e-9 }, { -3e-9, -1e-9 }, { 1e6, 1e6 + 0.001 }, { -7, 3 }, { -1e100, 1e100 },
    };
    const unsigned ROWS[] = { 12, 14, 17, 25, 50 };
    for (const auto &rg : RANGES) {
        int parts = result_parts();
        for (unsigned rows : ROWS)
            for (unsigned cols = 20; cols <= 60 && result_parts() == parts; cols++) {
                SCOPED_TRACE(testing::Message() << cols << "x" << rows << " y " << rg[0] << ".." << rg[1]);
                LineAcross la(cols, rows, 0, 1, rg[0], rg[1]);
                Rendered r = render(la.f, F);
                expect_y_labels_half_cell(r, rg[0], rg[1]);
            }
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

TEST(Layout, only_y_limits_are_honoured)
{
    Fig f(40, 16);
    f.ylim(0, 100);
    f.plot(BLOT_SCATTER, BLOT_DATA_INT32, 3, S3_X, S3_Y);
    Rendered r = render(f, F);
    ASSERT_TRUE(r.ok) << r.message;
    EXPECT_GE(max_label(y_ticks(r)), 50);
    EXPECT_EQ(labels_of(x_ticks(r)), std::vector<std::string>({ "0.000", "3.340", "6.680" }));
}

TEST(Layout, only_x_limits_are_honoured)
{
    Fig f(40, 16);
    f.xlim(0, 100);
    f.plot(BLOT_SCATTER, BLOT_DATA_INT32, 3, S3_X, S3_Y);
    Rendered r = render(f, F);
    ASSERT_TRUE(r.ok) << r.message;
    EXPECT_GE(max_label(x_ticks(r)), 30);
}

/* #7: an axis with limits uses exactly them, and the other axis gets the
 * auto range it would get with no limits at all (0.1% padding) */
TEST(Layout, one_sided_limits_match_both_set)
{
    const double pad = 10 * 0.001; /* auto range of 0..10 */
    {
        Fig one(40, 16), both(40, 16);
        one.ylim(0, 100);
        both.lim(0 - pad, 10 + pad, 0, 100);
        one.plot(BLOT_SCATTER, BLOT_DATA_INT32, 3, S3_X, S3_Y);
        both.plot(BLOT_SCATTER, BLOT_DATA_INT32, 3, S3_X, S3_Y);
        Rendered r1 = render(one, F), r2 = render(both, F);
        ASSERT_TRUE(r1.ok) << r1.message;
        ASSERT_TRUE(r2.ok) << r2.message;
        EXPECT_LINES_EQ(r1.lines, r2.lines);
    }
    {
        Fig one(40, 16), both(40, 16);
        one.xlim(0, 100);
        both.lim(0, 100, 0 - pad, 10 + pad);
        one.plot(BLOT_SCATTER, BLOT_DATA_INT32, 3, S3_X, S3_Y);
        both.plot(BLOT_SCATTER, BLOT_DATA_INT32, 3, S3_X, S3_Y);
        Rendered r1 = render(one, F), r2 = render(both, F);
        ASSERT_TRUE(r1.ok) << r1.message;
        ASSERT_TRUE(r2.ok) << r2.message;
        EXPECT_LINES_EQ(r1.lines, r2.lines);
    }
}

/* #7: the free axis is auto-ranged over all of the data, including points
 * outside the other axis' limits; only the labels are checked, so this
 * doesn't depend on how points outside the limits are drawn */
TEST(Layout, one_sided_limits_auto_range_all_data)
{
    Fig none(40, 16);
    none.plot(BLOT_SCATTER, BLOT_DATA_INT32, 3, S3_X, S3_Y);
    Rendered rn = render(none, F);
    ASSERT_TRUE(rn.ok) << rn.message;
    {
        Fig f(40, 16);
        f.xlim(0, 4);
        f.plot(BLOT_SCATTER, BLOT_DATA_INT32, 3, S3_X, S3_Y);
        Rendered r = render(f, F);
        ASSERT_TRUE(r.ok) << r.message;
        EXPECT_EQ(labels_of(y_ticks(r)), labels_of(y_ticks(rn)));
        EXPECT_EQ(labels_of(x_ticks(r)), std::vector<std::string>({ "0.000", "1.333", "2.667" }));
    }
    {
        Fig f(40, 16);
        f.ylim(0, 4);
        f.plot(BLOT_SCATTER, BLOT_DATA_INT32, 3, S3_X, S3_Y);
        Rendered r = render(f, F);
        ASSERT_TRUE(r.ok) << r.message;
        EXPECT_EQ(labels_of(x_ticks(r)), labels_of(x_ticks(rn)));
    }
}

/* #7: X and Y data differ, so ranging the free axis from the other
 * coordinate (or from the fixed limits) changes its labels */
TEST(Layout, one_sided_limits_free_axis_uses_its_own_data)
{
    const gint32 xs[] = { 0, 5, 10 }, ys[] = { -200, 0, 300 };
    Fig none(40, 16);
    none.plot(BLOT_SCATTER, BLOT_DATA_INT32, 3, xs, ys);
    Rendered rn = render(none, F);
    ASSERT_TRUE(rn.ok) << rn.message;
    EXPECT_EQ(labels_of(x_ticks(rn)), std::vector<std::string>({ "0.000", "3.340", "6.680" }));
    {
        Fig f(40, 16);
        f.ylim(0, 100);
        f.plot(BLOT_SCATTER, BLOT_DATA_INT32, 3, xs, ys);
        Rendered r = render(f, F);
        ASSERT_TRUE(r.ok) << r.message;
        EXPECT_EQ(labels_of(x_ticks(r)), labels_of(x_ticks(rn)));
        EXPECT_EQ(labels_of(y_ticks(r)),
                  std::vector<std::string>({ "0.000", "20.000", "40.000", "60.000", "80.000" }));
    }
    {
        Fig f(40, 16);
        f.xlim(0, 100);
        f.plot(BLOT_SCATTER, BLOT_DATA_INT32, 3, xs, ys);
        Rendered r = render(f, F);
        ASSERT_TRUE(r.ok) << r.message;
        EXPECT_EQ(labels_of(y_ticks(r)), labels_of(y_ticks(rn)));
        EXPECT_EQ(labels_of(x_ticks(r)), std::vector<std::string>({ "0.000", "33.333", "66.667" }));
    }
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
        /* the minimum is 20x12 (#17): the plot's 10x10 and the axes */
        { 19, 12, false }, { 20, 11, false }, { 10, 10, false }, { 28, 10, false },
        { 1001, 12, false }, { 20, 1001, false },
        { 20, 12, true }, { 1000, 1000, true },
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
    /* the setter rejects anything smaller than 20x12 (#17), and the
     * minimum renders with either axis or none */
    const struct {
        unsigned cols, rows;
        blot_render_flags extra;
        bool ok;
    } cases[] = {
        { 20, 12, BLOT_RENDER_NONE, true },
        { 20, 12, BLOT_RENDER_NO_X_AXIS | BLOT_RENDER_NO_Y_AXIS, true },
        { 20, 12, BLOT_RENDER_NO_Y_AXIS, true },
        { 20, 12, BLOT_RENDER_NO_X_AXIS, true },
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
        if (c.cols == 20 && c.rows == 12 && c.extra == BLOT_RENDER_NONE) {
            EXPECT_EQ(r.mrg.left, 8u);
            EXPECT_EQ(r.mrg.bottom, 2u);
            EXPECT_EQ(r.plot_cols(), 12u);
            EXPECT_EQ(r.plot_rows(), 10u);
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
        ASSERT_TRUE(blot_terminal_set_size({ 20, 12 }, &error));
        r = render(f, F);
        ASSERT_TRUE(r.ok) << r.message;
        EXPECT_EQ(r.dim.cols, 20u);
        EXPECT_EQ(r.dim.rows, 12u);

        const blot_dimensions bad[] = { { 19, 24 }, { 1001, 24 }, { 80, 11 }, { 80, 1001 } };
        for (const auto &d : bad) {
            EXPECT_FALSE(blot_terminal_set_size(d, &error)) << d.cols << "x" << d.rows;
            ASSERT_TRUE(error != NULL);
            EXPECT_EQ(error->code, EINVAL);
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

// B19, fixed in #11: https://git.jukie.net/bart/blot/-/work_items/11
TEST(Layout, terminal_environment_out_of_range)
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

// B10, fixed in #11: https://git.jukie.net/bart/blot/-/work_items/11
TEST(Layout, render_accepts_null_error)
{
    RUN_ISOLATED(10, {
        Fig f;
        fix_a_scatter(f);
        blot_screen *scr = blot_figure_render(f, F, NULL);
        EXPECT_TRUE(scr != NULL);
        blot_screen_delete(scr);
    });
}

/* B19 (#11): COLUMNS and LINES that aren't a number (or 0, or too large
 * for an unsigned) are ignored, and the size comes from the tty, here
 * /dev/null, which has none */
TEST(Layout, terminal_environment_invalid_is_ignored)
{
    const char *cases[][2] = {
        { "0", "24" }, { "80", "0" }, { "abc", "24" }, { "80x", "24" }, { "4294967306", "24" },
    };
    for (const auto &c : cases) {
        SCOPED_TRACE(testing::Message() << "COLUMNS=" << c[0] << " LINES=" << c[1]);
        RUN_ISOLATED(10, {
            setenv("COLUMNS", c[0], 1);
            setenv("LINES", c[1], 1);
            ASSERT_TRUE(freopen("/dev/null", "w", stdout) != NULL);
            blot_dimensions dim = {};
            GError *error = NULL;
            EXPECT_FALSE(blot_terminal_get_size(&dim, &error));
            ASSERT_TRUE(error != NULL);
            EXPECT_EQ(error->code, ENOTTY) << error->message;
            g_clear_error(&error);

            Fig f;
            f.lim(0, 9, 0, 9);
            f.plot(BLOT_LINE, BLOT_DATA_INT32, 2, DIAG_X, DIAG_Y);
            Rendered r = render(f, F);
            EXPECT_FALSE(r.ok);
            EXPECT_EQ(r.code, ENOTTY) << r.message;
        });
    }
}

/* B19 (#11 review L2): COLUMNS and LINES outside the range
 * blot_terminal_set_size() accepts are clamped into it before they are
 * used and cached, as the tty size is; no tty is needed */
TEST(Layout, terminal_environment_out_of_range_is_clamped)
{
    const struct {
        const char *cols, *rows;
        unsigned get_cols, get_rows;    /* blot_terminal_get_size() */
        unsigned exp_cols, exp_rows;    /* the screen, less the border */
    } cases[] = {
        { "19", "24", 20, 24, 20, 22 },
        { "1", "1", 20, 12, 20, 12 },
        { "1001", "24", 1000, 24, 998, 22 },
        { "80", "5000", 80, 1000, 78, 998 },
    };
    for (const auto &c : cases) {
        SCOPED_TRACE(testing::Message() << "COLUMNS=" << c.cols << " LINES=" << c.rows);
        RUN_ISOLATED(10, {
            setenv("COLUMNS", c.cols, 1);
            setenv("LINES", c.rows, 1);
            ASSERT_TRUE(freopen("/dev/null", "w", stdout) != NULL);
            blot_dimensions dim = {};
            EXPECT_TRUE(blot_terminal_get_size(&dim, NULL));
            EXPECT_EQ(dim.cols, c.get_cols);
            EXPECT_EQ(dim.rows, c.get_rows);

            Fig f;
            f.lim(0, 9, 0, 9);
            f.plot(BLOT_LINE, BLOT_DATA_INT32, 2, DIAG_X, DIAG_Y);
            Rendered r = render(f, F | BLOT_RENDER_NO_X_AXIS | BLOT_RENDER_NO_Y_AXIS);
            ASSERT_TRUE(r.ok) << r.message;
            EXPECT_EQ(r.dim.cols, c.exp_cols);
            EXPECT_EQ(r.dim.rows, c.exp_rows);
            EXPECT_EQ(r.lines.size(), c.exp_rows);
        });
    }
}

/* the terminal's own size, less the 2-cell border, within the minimum and
 * the maximum: COLUMNS/LINES at the edges of the range, and a tty that is
 * smaller or larger than blot can draw (B19, #11: 1x1 underflowed).  The
 * minimum renders with the axes too (#17) */
TEST(Layout, terminal_size_is_kept_in_range)
{
    const struct {
        const char *cols, *rows;
        unsigned short tty_cols, tty_rows;
        unsigned exp_cols, exp_rows;
    } cases[] = {
        { "1000", "1000", 0, 0, 998, 998 },
        { "20", "12", 0, 0, 20, 12 },
        { NULL, NULL, 1, 1, 20, 12 },
        { NULL, NULL, 0, 0, 20, 12 },
        { NULL, NULL, 5000, 3000, 1000, 1000 },
        { NULL, NULL, 82, 26, 80, 24 },
    };
    for (const auto &c : cases) {
        SCOPED_TRACE(testing::Message() << "COLUMNS=" << (c.cols ? c.cols : "-") << " LINES="
                     << (c.rows ? c.rows : "-") << " tty=" << c.tty_cols << "x" << c.tty_rows);
        RUN_ISOLATED(10, {
            if (c.cols) {
                setenv("COLUMNS", c.cols, 1);
                setenv("LINES", c.rows, 1);
                ASSERT_TRUE(freopen("/dev/null", "w", stdout) != NULL);
            } else {
                unsetenv("COLUMNS");
                unsetenv("LINES");
                int master = -1, slave = -1;
                struct winsize ws = {};
                ws.ws_col = c.tty_cols;
                ws.ws_row = c.tty_rows;
                ASSERT_EQ(openpty(&master, &slave, NULL, NULL, &ws), 0);
                drain_pty(master);
                ASSERT_EQ(dup2(slave, STDOUT_FILENO), STDOUT_FILENO);
            }
            Fig f;
            f.lim(0, 9, 0, 9);
            f.plot(BLOT_LINE, BLOT_DATA_INT32, 2, DIAG_X, DIAG_Y);
            Rendered r = render(f, F);
            ASSERT_TRUE(r.ok) << r.message;
            EXPECT_EQ(r.dim.cols, c.exp_cols);
            EXPECT_EQ(r.dim.rows, c.exp_rows);
            EXPECT_EQ(r.lines.size(), c.exp_rows);
        });
    }
}

/* #11 review L1: an error the caller has already set is kept, not freed
 * and replaced (glib warns about it, as g_propagate_error() does) */
TEST(Layout, render_keeps_callers_error)
{
    RUN_ISOLATED(10, {
        Fig f(28, 12);      /* no layers: the render fails */
        GError *pre = g_error_new_literal(g_quark_from_static_string("test"), 42, "pre-set");
        GError *error = pre;
        EXPECT_TRUE(blot_figure_render(f, F, &error) == NULL);
        ASSERT_TRUE(error == pre);
        EXPECT_EQ(error->code, 42);
        EXPECT_STREQ(error->message, "pre-set");
        g_clear_error(&error);
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

// #9 (B13): the label row was 62 and 78 columns wide
TEST(Layout, x_label_row_fits_zero_crossing)
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

// #9 (B13): labels 2 and 3 were hidden by labels 1 and 4
TEST(Layout, long_custom_x_labels)
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

// #9 (B14): the render failed
TEST(Layout, narrow_screen_large_x_values)
{
    const double xs[] = { -200000, -100000 }, ys[] = { 1, 2 };
    Fig f(28, 14);
    f.plot(BLOT_SCATTER, BLOT_DATA_DOUBLE, 2, xs, ys);
    Rendered r = render(f, F);
    EXPECT_TRUE(r.ok) << r.message;
}

namespace {
/* the X-axis line and the label line, right-trimmed */
Lines x_rows(const Rendered &r)
{
    Lines l = rtrim(strip_sgr(r.lines));
    unsigned ax = r.mrg.top + r.plot_rows();
    if (ax + 1 >= l.size())
        return {};
    return { l[ax], l[ax + 1] };
}

/* Every X tick has a label that starts under it and is a number that
 * names a value within half a cell of the tick's cell (the rule of the
 * axis, #9 review); the labels are all different; no line is wider than
 * the screen. */
void expect_x_labels_exact(const Rendered &r, double x_min, double x_max)
{
    ASSERT_TRUE(r.ok) << r.message;
    for (size_t i = 0; i < r.lines.size(); i++)
        EXPECT_LE(r.lines[i].size(), r.dim.cols) << "line " << i << " |" << to_utf8(r.lines[i]) << "|";
    double range = x_max - x_min, cell = range / r.plot_cols();
    auto ticks = x_ticks(r);
    EXPECT_FALSE(ticks.empty());
    std::set<std::string> seen;
    for (const auto &t : ticks) {
        const std::string &label = t.second;
        char *end = NULL;
        double v = strtod(label.c_str(), &end);
        EXPECT_TRUE(!label.empty() && end && *end == 0) << "column " << t.first << " label '" << label << "'";
        double lo = x_min + t.first * cell, eps = 0.5 * cell + 1e-9 * std::fabs(range);
        EXPECT_GE(v, lo - eps) << "column " << t.first << " label '" << label << "'";
        EXPECT_LT(v, lo + cell + eps) << "column " << t.first << " label '" << label << "'";
        EXPECT_TRUE(seen.insert(label).second) << "duplicate label " << label;
    }
}
} // namespace

/* #10 review L1: an autoscaled axis whose minimum is a tiny positive
 * value (1.09e-7, 4.1e-5) labels its first tick 0.000, as before: within
 * half a cell, fixed point is taken even when it is less precise than
 * %.1g (not 1.1e-07, 4.1e-05) */
TEST(Layout, near_zero_minimum_labels_exact)
{
    for (double lo : { 0.00099911, 0.00104 }) {
        SCOPED_TRACE(testing::Message() << "y " << lo << "..1");
        const double xs[] = { 0, 1 }, ys[] = { lo, 1 };
        Fig f(40, 14);
        f.plot(BLOT_LINE, BLOT_DATA_DOUBLE, 2, xs, ys);
        Rendered r = render(f, F);
        ASSERT_TRUE(r.ok) << r.message;
        EXPECT_EQ(r.mrg.left, 8u);
        EXPECT_EQ(labels_of(y_ticks(r)), std::vector<std::string>({ "0.000", "0.200", "0.400", "0.601", "0.801" }));
        EXPECT_EQ(labels_of(x_ticks(r)), std::vector<std::string>({ "0.000", "0.334", "0.668" }));
    }
}

/* #10 review L2: the last resort of the Y-axis, in a figure.  On the
 * smallest screen (20 columns, #17) the margin is 10 and a label has 8
 * columns; 1700001234 has no text in 8 that is within half a cell (1.7e+09
 * is 1234 off, half a cell is 82), so one tick, at the bottom, with no
 * label rather than a misleading one (master: 1.7e+09 on every tick) */
TEST(Layout, y_axis_lone_tick_without_label)
{
    LineAcross la(20, 24, 0, 1, 1700001234, 1700004834);
    Rendered r = render(la.f, F);
    ASSERT_TRUE(r.ok) << r.message;
    EXPECT_EQ(r.mrg.left, 10u);
    auto ticks = y_ticks(r);
    ASSERT_EQ(ticks.size(), 1u);
    EXPECT_EQ(ticks[0].first, 0u);
    EXPECT_EQ(ticks[0].second, "");
    Lines l = strip_sgr(r.lines);
    for (unsigned row = 0; row < r.plot_rows(); row++)
        EXPECT_EQ(l[r.mrg.top + row].substr(0, 10), row + 1 == r.plot_rows() ? L"         *" : L"         |") << row;
}

/* #10: tiny ranges, exact: the labels have the digits the tick spacing
 * needs (they were all 0.000, or 0.001 for 0.0012 and 0.0014), the left
 * margin is as wide as they need, and zero is not negative */
TEST(Layout, tiny_range_labels_exact)
{
    const struct {
        double lo, hi;
        unsigned left;
        std::vector<std::string> y;
        const wchar_t *axis, *labels;
    } cases[] = {
        { 1e-9, 3e-9, 9, { "1e-09", "1.4e-09", "1.8e-09", "2.2e-09", "2.6e-09" },
          L"        +*---------*---------*---------*---------*----------",
          L"         1e-09     1.4e-09   1.8e-09   2.2e-09   2.6e-09" },
        { 0.001, 0.002, 8, { "0.001", "0.0012", "0.0014", "0.0016", "0.0018" },
          L"       +*---------*---------*----------*---------*----------",
          L"        0.001     0.0012    0.0014     0.0016    0.0018" },
        { -0.0005, 0.0005, 9, { "-0.0004", "-0.0002", "0.000", "0.0002", "0.0004" },
          L"        +-----*---------*---------*---------*---------*-----",
          L"              -0.0004   -0.0002   0.000     0.0002    0.0004" },
    };
    for (const auto &c : cases) {
        SCOPED_TRACE(testing::Message() << c.lo << ".." << c.hi);
        LineAcross la(60, 14, c.lo, c.hi, c.lo, c.hi);
        Rendered r = render(la.f, F);
        ASSERT_TRUE(r.ok) << r.message;
        EXPECT_EQ(r.mrg.left, c.left);
        EXPECT_EQ(labels_of(y_ticks(r)), c.y);
        EXPECT_LINES_EQ(x_rows(r), Lines({ c.axis, c.labels }));
    }
}

/* #9: the boundary widths of B14, exact.  The plot is 8 columns in.  Up to
 * 19 plot columns there is one tick, at x_min; from 20 the three ticks of
 * the formula need 6-character labels, -2e+05 for both -200100 and -166700,
 * so there are two: x_min and the middle, -150000 (the limits are the data
 * padded by 0.1% of the range). */
TEST(Layout, x_labels_large_negative_values_exact)
{
    const double xs[] = { -200000, -100000 }, ys[] = { 1, 2 };
    const struct {
        unsigned cols;
        const wchar_t *axis, *labels;
    } cases[] = {
        { 20, L"       +*-----------", L"        -200100.000" },
        { 21, L"       +*------------", L"        -200100.000" },
        { 22, L"       +*-------------", L"        -200100.000" },
        { 23, L"       +*--------------", L"        -200100.000" },
        { 24, L"       +*---------------", L"        -200100.000" },
        { 25, L"       +*----------------", L"        -200100.000" },
        { 26, L"       +*-----------------", L"        -200100.000" },
        { 27, L"       +*------------------", L"        -200100.000" },
        { 28, L"       +*---------*---------", L"        -200100.0 -150000.0" },
        { 29, L"       +*---------*----------", L"        -200100.0 -150000.0" },
        { 30, L"       +*----------*----------", L"        -200100.00 -150000.00" },
        { 31, L"       +*----------*-----------", L"        -200100.00 -150000.00" },
    };
    for (const auto &c : cases) {
        SCOPED_TRACE(testing::Message() << c.cols << "x14");
        Fig f(c.cols, 14);
        f.plot(BLOT_SCATTER, BLOT_DATA_DOUBLE, 2, xs, ys);
        Rendered r = render(f, F);
        ASSERT_TRUE(r.ok) << r.message;
        EXPECT_LINES_EQ(x_rows(r), Lines({ c.axis, c.labels }));
        expect_x_labels_exact(r, -200100, -99900);
    }
}

/* #9: INT64 extremes; the left margin is capped at half the screen.  At 40
 * columns the five ticks of the formula don't fit 6-character labels, so
 * there are two: x_min and 0 (the zero alignment). */
TEST(Layout, x_labels_int64_extremes_exact)
{
    const gint64 v[] = { G_MININT64, 0, G_MAXINT64 };
    Fig f(40, 14);
    f.plot(BLOT_SCATTER, BLOT_DATA_INT64, 3, v, v);
    Rendered r = render(f, F);
    ASSERT_TRUE(r.ok) << r.message;
    EXPECT_LINES_EQ(x_rows(r), Lines({ L"                   +*---------*---------",
                                       L"                    -9.24e+18 0.000" }));
}

/* #9: the zero alignment moves the ticks right; the last label is cut
 * to the edge (1.80) or, with no room for any text, its tick is dropped
 * (2.863 at 73 columns), never written past the screen */
TEST(Layout, x_labels_zero_crossing_exact)
{
    {
        const double v[] = { -1, 0, 2 };
        Fig f(60, 20);
        f.plot(BLOT_LINE, BLOT_DATA_DOUBLE, 3, v, v);
        Rendered r = render(f, F);
        EXPECT_LINES_EQ(x_rows(r),
                        Lines({ L"        +------*----------*---------*---------*---------*---",
                                L"               -0.601     0.000     0.601     1.202     1.80" }));
    }
    {
        const double v[] = { -7, 3 };
        Fig f(73, 23);
        f.plot(BLOT_LINE, BLOT_DATA_DOUBLE, 2, v, v);
        Rendered r = render(f, F);
        EXPECT_LINES_EQ(x_rows(r),
                        Lines({ L"        +--------*--------*--------*--------*--------*--------*----------",
                                L"                 -5.726   -4.294   -2.863   -1.431   0.000    1.431" }));
    }
}

/* #9: long custom labels are cut before the next tick less a blank (7 of
 * 8 columns; the last one to the edge), each starts at its own tick; too
 * many labels for the columns: every k-th gets a tick */
TEST(Layout, x_custom_labels_exact)
{
    const gint32 ys[] = { 3, 1, 4, 1 };
    {
        const char *names[] = { "first-label-xxxx", "second-label-xxx", "third-label-xxxx", "fourth-label-xxx" };
        Fig f(40, 14);
        ASSERT_TRUE(blot_figure_set_x_axis_labels(f, 4, (char **)names, NULL));
        f.plot(BLOT_BAR, BLOT_DATA_INT32, 4, NULL, ys);
        Rendered r = render(f, F);
        EXPECT_LINES_EQ(x_rows(r), Lines({ L"       +*-------*-------*-------*-------",
                                           L"        first-l second- third-l fourth-l" }));
    }
    {
        /* 30 labels on 32 columns: every second one */
        std::vector<std::string> s;
        std::vector<char *> names;
        for (int i = 0; i < 30; i++)
            s.push_back(std::string(1, (char)('A' + i % 26)));
        for (auto &n : s)
            names.push_back((char *)n.c_str());
        Fig f(40, 14);
        ASSERT_TRUE(blot_figure_set_x_axis_labels(f, names.size(), names.data(), NULL));
        f.plot(BLOT_BAR, BLOT_DATA_INT32, 4, NULL, ys);
        Rendered r = render(f, F);
        ASSERT_TRUE(r.ok) << r.message;
        /* ticks 0 2 4 ... 28 at cells floor(i * 32 / 30) */
        EXPECT_LINES_EQ(x_rows(r), Lines({ L"       +*-*-*-*-*-*-*-*--*-*-*-*-*-*-*--",
                                           L"        A C E G I K M O  Q S U W Y A C" }));
    }
}

/* #9: a sweep of widths and data; every X label row fits the screen, each
 * tick's label is the value of its cell, and no two are the same.  The
 * ranges that abort without the fix are in the isolated test below.  #10:
 * also the ranges where %.3f is coarser than half a cell ([0.001,0.002]
 * gave 0.001 for both 0.0010 and 0.0014) and tiny ones. */
TEST(Layout, x_labels_fit_and_match_their_cells)
{
    const double RANGES[][2] = {
        { -200000, -100000 }, { -1, 2 }, { -7, 3 }, { 0, 1 }, { -1e100, 1e100 },
        { -0.5, 1e6 }, { 1e9, 5.2e9 }, { 0.001, 0.002 }, { -0.01003, 0.02003 },
        { 1e-9, 3e-9 }, { -0.0005, 0.0005 }, { 1e6, 1e6 + 0.001 },
    };
    for (const auto &rg : RANGES) {
        int parts = result_parts();
        for (unsigned cols = BLOT_MIN_COLS; cols <= 130 && result_parts() == parts; cols++) {
            SCOPED_TRACE(testing::Message() << "x [" << rg[0] << "," << rg[1] << "] " << cols << "x14");
            const double xs[] = { rg[0], rg[1] }, ys[] = { 1, 2 };
            Fig f(cols, 14);
            f.lim(rg[0], rg[1], 1, 2);
            f.plot(BLOT_LINE, BLOT_DATA_DOUBLE, 2, xs, ys);
            Rendered r = render(f, F);
            expect_x_labels_exact(r, rg[0], rg[1]);
        }
    }
}

/* #9: the same sweep for |x| >= 1e12, whose 17-18 character labels on a
 * one-tick axis aborted the process: isolated, so the other tests run */
TEST(Layout, x_labels_fit_and_match_their_cells_large)
{
    RUN_ISOLATED(30, {
        const double RANGES[][2] = { { 1e12, 2e12 }, { -2e12, -1e12 }, { 1e17, 2e17 }, { -2e17, -1e17 } };
        for (const auto &rg : RANGES) {
            for (unsigned cols = BLOT_MIN_COLS; cols <= 130; cols++) {
                SCOPED_TRACE(testing::Message() << "x [" << rg[0] << "," << rg[1] << "] " << cols << "x14");
                const double xs[] = { rg[0], rg[1] }, ys[] = { 1, 2 };
                Fig f(cols, 14);
                f.lim(rg[0], rg[1], 1, 2);
                f.plot(BLOT_LINE, BLOT_DATA_DOUBLE, 2, xs, ys);
                Rendered r = render(f, F);
                expect_x_labels_exact(r, rg[0], rg[1]);
            }
        }
    });
}

/* #9 and the #13 review: with one X tick (plot widths under 20) the label
 * room was the whole axis, not BLOT_AXIS_LABEL_MAX, and a 17-18 character
 * label (|x| >= 1e12) tripped an assertion and aborted the process.  From
 * the minimum screen (#17): Y 1..2 (an 8-column margin) gives plot widths
 * 12 and up, Y -1e6..1e6 (a margin capped at half the screen) 10 and up */
TEST(Layout, long_x_label_on_one_tick_does_not_abort)
{
    RUN_ISOLATED(10, {
        const double RANGES[][2] = { { 1e12, 2e12 }, { -2e12, -1e12 }, { 1e17, 2e17 }, { -2e17, -1e17 } };
        const double YLIMS[][2] = { { 1, 2 }, { -1e6, 1e6 } };
        std::set<unsigned> one_tick_widths;
        for (const auto &rg : RANGES) {
            for (const auto &yl : YLIMS) {
                for (unsigned cols = BLOT_MIN_COLS; cols <= 40; cols++) {
                    SCOPED_TRACE(testing::Message() << "x [" << rg[0] << "," << rg[1] << "] y ["
                                 << yl[0] << "," << yl[1] << "] " << cols << "x14");
                    const double xs[] = { rg[0], rg[1] }, ys[] = { yl[0], yl[1] };
                    Fig f(cols, 14);
                    f.lim(rg[0], rg[1], yl[0], yl[1]);
                    f.plot(BLOT_SCATTER, BLOT_DATA_DOUBLE, 2, xs, ys);
                    Rendered r = render(f, F);
                    EXPECT_TRUE(r.ok) << r.message;
                    auto ticks = x_ticks(r);
                    if (r.ok && ticks.size() == 1)
                        one_tick_widths.insert(r.plot_cols());
                    for (const auto &t : ticks)
                        EXPECT_LE(t.second.size(), 16u) << t.second;
                }
            }
        }
        /* every one-tick plot width, down to the minimum plot */
        for (unsigned w = BLOT_MIN_PLOT_COLS; w < 20; w++)
            EXPECT_TRUE(one_tick_widths.count(w)) << "no one-tick render at plot width " << w;
    });
}

TEST(Layout, line_reaches_top_right_at_every_size)
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

/* #17: the setter accepted sizes (down to 10x10) that the render then
 * failed with EINVAL once an axis was drawn: the plot needs its minimum
 * plus the axes.  Every size near the minimum, set on the figure or on the
 * terminal, is either rejected or renders with every combination of the
 * axis and legend flags. */
namespace {

const gint32 MS_X[] = { 0, 9 }, MS_Y0[] = { 0, 9 }, MS_Y1[] = { 9, 0 };

/* two labelled layers; wide: Y labels that want more than half the screen */
void min_size_plot(Fig &f, bool wide)
{
    if (wide)
        f.lim(0, 9, -1e6, 1e6);
    else
        f.lim(0, 9, 0, 9);
    f.plot(BLOT_LINE, BLOT_DATA_INT32, 2, MS_X, MS_Y0, 1, "up");
    f.plot(BLOT_SCATTER, BLOT_DATA_INT32, 2, MS_X, MS_Y1, 2, "down");
}

/* every combination of NO_X_AXIS, NO_Y_AXIS and the legend flags renders
 * exactly rows lines no wider than cols, keeps the plot at its minimum or
 * more, and the legend gets the rows the plot can spare, up to its 2 lines
 * per legend */
void expect_renders_with_all_flags(blot_figure *f, unsigned cols, unsigned rows)
{
    const blot_render_flags bits[] = { BLOT_RENDER_NO_X_AXIS, BLOT_RENDER_NO_Y_AXIS,
                                       BLOT_RENDER_LEGEND_ABOVE, BLOT_RENDER_LEGEND_BELOW,
                                       BLOT_RENDER_LEGEND_DETAILS };
    const unsigned n = sizeof(bits) / sizeof(bits[0]);
    for (unsigned m = 0; m < (1u << n); m++) {
        unsigned flags = F;
        for (unsigned b = 0; b < n; b++)
            if (m & (1u << b))
                flags |= bits[b];
        SCOPED_TRACE(testing::Message() << cols << "x" << rows << " flags 0x" << std::hex << flags);
        Rendered r = render(f, (blot_render_flags)flags);
        ASSERT_TRUE(r.ok) << "accepted, but the render failed: " << r.message;
        ASSERT_EQ(r.lines.size(), rows);
        Lines lines = strip_sgr(r.lines);
        for (size_t i = 0; i < lines.size(); i++)
            ASSERT_LE(lines[i].size(), cols) << "line " << i << " |" << to_utf8(lines[i]) << "|";
        EXPECT_GE(r.plot_rows(), (unsigned)BLOT_MIN_PLOT_ROWS);
        EXPECT_GE(r.plot_cols(), (unsigned)BLOT_MIN_PLOT_COLS);

        /* a requested axis is drawn, not dropped to make room */
        unsigned top = r.mrg.top, prow = r.plot_rows();
        if (!(flags & BLOT_RENDER_NO_Y_AXIS)) {
            ASSERT_GE(r.mrg.left, 2u);
            for (unsigned y = top; y < top + prow; y++) {
                ASSERT_GE(lines[y].size(), r.mrg.left) << "line " << y;
                wchar_t a = lines[y][r.mrg.left - 1];
                EXPECT_TRUE(a == L'|' || a == L'*') << "line " << y << " |" << to_utf8(lines[y]) << "|";
            }
        }
        if (!(flags & BLOT_RENDER_NO_X_AXIS)) {
            /* the axis line under the plot, then its label row */
            ASSERT_GE(r.mrg.bottom, 2u);
            ASSERT_GT(lines.size(), top + prow + 1);
            const std::wstring &axis = lines[top + prow];
            ASSERT_EQ(axis.size(), cols) << "|" << to_utf8(axis) << "|";
            for (unsigned x = r.mrg.left; x < cols - r.mrg.right; x++)
                EXPECT_TRUE(axis[x] == L'-' || axis[x] == L'*') << "column " << x << " |" << to_utf8(axis) << "|";
            if (!(flags & BLOT_RENDER_NO_Y_AXIS)) {
                EXPECT_EQ(axis[r.mrg.left - 1], L'+') << "|" << to_utf8(axis) << "|";
            }
            EXPECT_FALSE(x_ticks(r).empty()) << "no X tick";
        }

        unsigned x_rows = (flags & BLOT_RENDER_NO_X_AXIS) ? 0 : 2;
        unsigned legends = !!(flags & BLOT_RENDER_LEGEND_ABOVE) + !!(flags & BLOT_RENDER_LEGEND_BELOW);
        unsigned spare = rows - x_rows - BLOT_MIN_PLOT_ROWS;
        unsigned marks = 0;
        for (const auto &l : lines)
            if (l.find((wchar_t)0x25D8) != std::wstring::npos)    /* ◘ */
                marks++;
        EXPECT_EQ(marks, std::min(2 * legends, spare));
        if (::testing::Test::HasFailure())
            return;
    }
}

} // namespace

// #17: https://git.jukie.net/bart/blot/-/work_items/17
TEST(Layout, min_size_reproducer)
{
    Fig f;
    min_size_plot(f, false);
    GError *error = NULL;
    if (!blot_figure_set_screen_size(f, 28, 10, &error)) {
        ASSERT_TRUE(error != NULL);
        EXPECT_EQ(error->code, EINVAL);
        g_clear_error(&error);
        return;
    }
    Rendered r = render(f, F);
    ASSERT_TRUE(r.ok) << "28x10 accepted, but the render failed: " << r.message;
    EXPECT_EQ(r.lines.size(), 10u);
}

// #17: https://git.jukie.net/bart/blot/-/work_items/17
TEST(Layout, accepted_screen_sizes_render)
{
    for (bool wide : { false, true }) {
        for (unsigned cols = 1; cols <= BLOT_MIN_COLS + 4; cols++) {
            for (unsigned rows = 1; rows <= BLOT_MIN_ROWS + 4; rows++) {
                SCOPED_TRACE(testing::Message() << (wide ? "wide" : "narrow") << " Y labels");
                Fig f;
                min_size_plot(f, wide);
                GError *error = NULL;
                if (!blot_figure_set_screen_size(f, cols, rows, &error)) {
                    ASSERT_TRUE(error != NULL);
                    EXPECT_EQ(error->code, EINVAL);
                    g_clear_error(&error);
                    continue;
                }
                expect_renders_with_all_flags(f, cols, rows);
                if (HasFailure())
                    return;
            }
        }
    }
}

// #17: https://git.jukie.net/bart/blot/-/work_items/17
TEST(Layout, accepted_terminal_sizes_render)
{
    /* the terminal size is process-wide: one child for the sweep */
    RUN_ISOLATED(60, {
        for (unsigned cols = 1; cols <= BLOT_MIN_COLS + 4; cols++) {
            for (unsigned rows = 1; rows <= BLOT_MIN_ROWS + 4; rows++) {
                GError *error = NULL;
                if (!blot_terminal_set_size({ cols, rows }, &error)) {
                    ASSERT_TRUE(error != NULL);
                    EXPECT_EQ(error->code, EINVAL);
                    g_clear_error(&error);
                    continue;
                }
                /* the screen is the terminal less its border, within the
                 * minimum */
                unsigned sc = std::max(cols - 2, (unsigned)BLOT_MIN_COLS);
                unsigned sr = std::max(rows - 2, (unsigned)BLOT_MIN_ROWS);
                for (bool wide : { false, true }) {
                    Fig f;  /* no set_screen_size */
                    min_size_plot(f, wide);
                    expect_renders_with_all_flags(f, sc, sr);
                    if (HasFailure())
                        return;
                }
            }
        }
    });
}

// #17: https://git.jukie.net/bart/blot/-/work_items/17
TEST(Layout, clamped_terminal_sizes_render)
{
    /* COLUMNS/LINES and a tty below the minimum are clamped to it; the
     * size is cached on first use: one child per case */
    for (bool tty : { false, true }) {
        SCOPED_TRACE(tty ? "tty 1x1" : "COLUMNS=1 LINES=1");
        RUN_ISOLATED(30, {
            if (!tty) {
                setenv("COLUMNS", "1", 1);
                setenv("LINES", "1", 1);
                ASSERT_TRUE(freopen("/dev/null", "w", stdout) != NULL);
            } else {
                unsetenv("COLUMNS");
                unsetenv("LINES");
                int master = -1, slave = -1;
                struct winsize ws = {};
                ws.ws_col = 1;
                ws.ws_row = 1;
                ASSERT_EQ(openpty(&master, &slave, NULL, NULL, &ws), 0);
                drain_pty(master);
                ASSERT_EQ(dup2(slave, STDOUT_FILENO), STDOUT_FILENO);
            }
            for (bool wide : { false, true }) {
                Fig f;
                min_size_plot(f, wide);
                expect_renders_with_all_flags(f, BLOT_MIN_COLS, BLOT_MIN_ROWS);
            }
        });
    }
}
