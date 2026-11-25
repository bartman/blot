/* data edge cases at figure level (#2, group E).  Flags F and auto limits
 * unless stated.  "Plot cell (c, r)" is a column and row inside the plot
 * area, row 0 at the bottom. */
#include "blot_test.hpp"

#include <cerrno>
#include <cfloat>
#include <cstdlib>

using namespace blot_test;

namespace {

/* render one layer on a fresh figure */
Rendered render_one(unsigned cols, unsigned rows, blot_plot_type type, blot_data_type dt,
                    size_t n, const void *xs, const void *ys,
                    blot_render_flags flags = F)
{
    Fig f(cols, rows);
    f.plot(type, dt, n, xs, ys);
    return render(f, flags);
}

std::set<unsigned> rows_of(const Cells &cs)
{
    std::set<unsigned> out;
    for (const auto &c : cs)
        out.insert(c.second);
    return out;
}

bool near(const Cells &cs, unsigned col, unsigned row, unsigned dist = 1)
{
    for (const auto &c : cs)
        if (std::abs((int)c.first - (int)col) <= (int)dist && std::abs((int)c.second - (int)row) <= (int)dist)
            return true;
    return false;
}

/* no label is nan, -nan, inf or -inf (#6) */
void expect_no_nan_label(const Rendered &r)
{
    for (const auto &t : y_ticks(r)) {
        EXPECT_EQ(t.second.find("nan"), std::string::npos) << "Y label " << t.second;
        EXPECT_EQ(t.second.find("inf"), std::string::npos) << "Y label " << t.second;
    }
    for (const auto &t : x_ticks(r)) {
        EXPECT_EQ(t.second.find("nan"), std::string::npos) << "X label " << t.second;
        EXPECT_EQ(t.second.find("inf"), std::string::npos) << "X label " << t.second;
    }
}

/* the top marked row of the column range [c0, c1] */
int top_row_in(const Cells &cs, unsigned c0, unsigned c1)
{
    int top = -1;
    for (const auto &c : cs)
        if (c.first >= c0 && c.first <= c1)
            top = std::max(top, (int)c.second);
    return top;
}

} // namespace

TEST(Edge, scatter_agrees_with_axes)
{
    const double v[] = { 0, 0.25, 0.5, 0.75, 1 };
    Rendered r = render_one(40, 16, BLOT_SCATTER, BLOT_DATA_DOUBLE, 5, v, v);
    ASSERT_TRUE(r.ok) << r.message;
    unsigned pc = r.plot_cols(), pr = r.plot_rows();
    Cells m = plot_cells(r);
    EXPECT_EQ(m.size(), 5u) << cells_str(m);
    EXPECT_TRUE(m.count({ pc - 1, pr - 1 })) << "(1,1) not in the top-right cell:" << cells_str(m);
    /* auto limits -0.001 .. 1.001 */
    unsigned cc = expected_cell(0.5, -0.001, 1.001, pc), cr = expected_cell(0.5, -0.001, 1.001, pr);
    EXPECT_TRUE(near(m, cc, cr)) << "(0.5,0.5) not near (" << cc << "," << cr << "):" << cells_str(m);
}

namespace {

/* for every Y tick of the limits, a figure with a mark at the tick value:
 * the mark's row is within 1 of the tick's row */
void expect_marks_on_ticks(blot_plot_type type)
{
    const double ylims[][2] = { { 0, 1 }, { 0, 9 }, { -250, -200 } };
    const double xs2[] = { 0, 1 }, xs1[] = { 0.5 };
    for (const auto &yl : ylims) {
        Rendered axes;
        {
            const double ys[] = { yl[0], yl[1] };
            Fig f(40, 16);
            f.lim(0, 1, yl[0], yl[1]);
            f.plot(BLOT_LINE, BLOT_DATA_DOUBLE, 2, xs2, ys);
            axes = render(f, F);
            ASSERT_TRUE(axes.ok) << axes.message;
        }
        auto ticks = y_ticks(axes);
        ASSERT_EQ(ticks.size(), 5u) << "Y ticks of y " << yl[0] << ".." << yl[1];
        for (const auto &t : ticks) {
            double v = strtod(t.second.c_str(), NULL);
            SCOPED_TRACE(testing::Message() << "y " << yl[0] << ".." << yl[1] << " tick row " << t.first
                                            << " value " << v << " plot " << type);
            const double ys[] = { v, v };
            Fig f(40, 16);
            f.lim(0, 1, yl[0], yl[1]);
            if (type == BLOT_LINE)
                f.plot(BLOT_LINE, BLOT_DATA_DOUBLE, 2, xs2, ys);
            else
                f.plot(type, BLOT_DATA_DOUBLE, 1, xs1, ys);
            Rendered r = render(f, F);
            ASSERT_TRUE(r.ok) << r.message;
            Cells m = plot_cells(r);
            ASSERT_FALSE(m.empty());
            int row = type == BLOT_LINE ? (int)m.begin()->second : top_row_in(m, 0, r.plot_cols());
            if (type == BLOT_LINE) {
                EXPECT_EQ(rows_of(m).size(), 1u) << cells_str(m);
            }
            /* the tick's row; or the row of the label's value, when the
             * label's 3-decimal rounding crosses into the next cell */
            int label_row = expected_cell(v, yl[0], yl[1], r.plot_rows());
            EXPECT_TRUE(row == (int)t.first || row == label_row)
                << "mark in row " << row << ", tick row " << t.first << ":" << cells_str(m);
        }
    }
}

} // namespace

// BUG: https://git.jukie.net/bart/blot/-/work_items/14
// a line maps y with round(v * (rows-1) / range), the Y axis with
// floor(v * rows / range); a point on a tick value lands one row off its
// tick (y 0..1, 40x16: 0.2 on row 3, its tick on row 2)
TEST(Edge, DISABLED_point_on_tick_value_lands_on_tick_line)
{
    expect_marks_on_ticks(BLOT_LINE);
}

TEST(Edge, point_on_tick_value_lands_on_tick_scatter_bar)
{
    expect_marks_on_ticks(BLOT_SCATTER);
    expect_marks_on_ticks(BLOT_BAR);
}

TEST(Edge, scatter_and_line_share_cells)
{
    const gint32 v[] = { 0, 5, 10 };
    for (blot_plot_type type : { BLOT_LINE, BLOT_SCATTER }) {
        SCOPED_TRACE(type);
        Rendered r = render_one(40, 16, type, BLOT_DATA_INT32, 3, v, v);
        ASSERT_TRUE(r.ok) << r.message;
        Cells m = plot_cells(r);
        EXPECT_TRUE(m.count({ 0, 0 })) << cells_str(m);
        EXPECT_TRUE(m.count({ r.plot_cols() - 1, r.plot_rows() - 1 })) << cells_str(m);
    }
}

TEST(Edge, wide_range_scatter_keeps_all_points)
{
    const gint32 v[] = { 0, 500, 1000 };
    Rendered r = render_one(40, 16, BLOT_SCATTER, BLOT_DATA_INT32, 3, v, v);
    ASSERT_TRUE(r.ok) << r.message;
    Cells m = plot_cells(r);
    EXPECT_EQ(m.size(), 3u) << cells_str(m);
    EXPECT_TRUE(rows_of(m).count(r.plot_rows() - 1)) << cells_str(m);
}

TEST(Edge, wide_range_bar_keeps_tallest)
{
    const gint32 v[] = { 0, 500, 1000 };
    Rendered r = render_one(40, 16, BLOT_BAR, BLOT_DATA_INT32, 3, v, v);
    ASSERT_TRUE(r.ok) << r.message;
    Cells m = plot_cells(r);
    EXPECT_TRUE(rows_of(m).count(r.plot_rows() - 1)) << cells_str(m);
}

TEST(Edge, single_point_is_centred)
{
    const double xs[] = { 5 }, ys[] = { -250 };
    Rendered r = render_one(40, 14, BLOT_SCATTER, BLOT_DATA_DOUBLE, 1, xs, ys);
    ASSERT_TRUE(r.ok) << r.message;
    ASSERT_EQ(r.plot_cols(), 29u);
    ASSERT_EQ(r.plot_rows(), 12u);
    EXPECT_EQ(plot_cells(r), Cells({ { 14, 6 } })) << cells_str(plot_cells(r));
}

/* bar X uses a slot per integer of the X range, not the axis mapping (#3):
 * on this numeric axis the bar starts in column 7, not 14 */
// BUG B21: https://git.jukie.net/bart/blot/-/work_items/12
TEST(Edge, DISABLED_single_point_bar_is_centred)
{
    const double xs[] = { 5 }, ys[] = { -250 };
    Rendered r = render_one(40, 14, BLOT_BAR, BLOT_DATA_DOUBLE, 1, xs, ys);
    ASSERT_TRUE(r.ok) << r.message;
    Cells m = plot_cells(r);
    ASSERT_FALSE(m.empty());
    EXPECT_EQ(m.begin()->first, 14u) << cells_str(m);
}

TEST(Edge, single_point_line_is_visible)
{
    const double xs[] = { 5 }, ys[] = { -250 };
    Rendered r = render_one(40, 14, BLOT_LINE, BLOT_DATA_DOUBLE, 1, xs, ys);
    ASSERT_TRUE(r.ok) << r.message;
    EXPECT_EQ(plot_cells(r), Cells({ { 14, 6 } })) << cells_str(plot_cells(r));
}

TEST(Edge, identical_points_line)
{
    const double xs[] = { -3, -3, -3 }, ys[] = { 7, 7, 7 };
    Rendered r = render_one(40, 14, BLOT_LINE, BLOT_DATA_DOUBLE, 3, xs, ys);
    ASSERT_TRUE(r.ok) << r.message;
    ASSERT_EQ(r.plot_cols(), 32u);
    ASSERT_EQ(r.plot_rows(), 12u);
    EXPECT_EQ(plot_cells(r), Cells({ { 16, 6 } })) << cells_str(plot_cells(r));
    EXPECT_EQ(labels_of(y_ticks(r)), std::vector<std::string>({ "6.300", "6.580", "6.860", "7.140", "7.420" }));
    EXPECT_EQ(labels_of(x_ticks(r)), std::vector<std::string>({ "-3.300", "-3.100", "-2.900" }));
}

/* B6 (#6) */
TEST(Edge, nan_does_not_shrink_limits)
{
    const double xs[] = { 0, 1, 2, 3 }, ys[] = { 100, NAN, 1, 2 };
    Rendered r = render_one(40, 14, BLOT_SCATTER, BLOT_DATA_DOUBLE, 4, xs, ys);
    ASSERT_TRUE(r.ok) << r.message;
    EXPECT_EQ(plot_cells(r).size(), 3u) << cells_str(plot_cells(r));
    double top = -INFINITY;
    for (const auto &t : y_ticks(r))
        top = std::max(top, strtod(t.second.c_str(), NULL));
    EXPECT_GT(top, 50);
}

/* B6 (#6) */
TEST(Edge, nan_last_keeps_numeric_labels)
{
    const double xs[] = { 0, 1, 2, 3 }, ys[] = { 1, 2, 3, NAN };
    Rendered r = render_one(40, 14, BLOT_SCATTER, BLOT_DATA_DOUBLE, 4, xs, ys);
    ASSERT_TRUE(r.ok) << r.message;
    expect_no_nan_label(r);
    Cells m = plot_cells(r);
    EXPECT_EQ(m.size(), 3u) << cells_str(m);
    EXPECT_EQ(rows_of(m).size(), 3u) << cells_str(m);
}

/* B6 (#6): all-NaN data is "no data", see all_non_finite_data_is_no_data */
TEST(Edge, all_nan_data)
{
    RUN_ISOLATED(5, {
        const double xs[] = { 0, 1, 2 }, ys[] = { NAN, NAN, NAN };
        Rendered r = render_one(40, 14, BLOT_LINE, BLOT_DATA_DOUBLE, 3, xs, ys);
        if (r.ok) {
            EXPECT_TRUE(plot_cells(r).empty()) << cells_str(plot_cells(r));
            expect_no_nan_label(r);
        }
    });
}

/* B7 (#6) */
TEST(Edge, infinity_in_data)
{
    RUN_ISOLATED(5, {
        const double xs[] = { 0, 1, 2, 3 }, ys[] = { 1, 2, INFINITY, 3 };
        Rendered r = render_one(40, 14, BLOT_SCATTER, BLOT_DATA_DOUBLE, 4, xs, ys);
        ASSERT_TRUE(r.ok) << r.message;
        expect_no_nan_label(r);
        Cells m = plot_cells(r);
        /* the 3 finite points are in 3 different rows */
        EXPECT_GE(rows_of(m).size(), 3u) << cells_str(m);
    });
}

/* #6: data with no finite value on an auto-ranged axis is "no data":
 * the render fails with ENOENT, like a figure whose layers are empty */
TEST(Edge, all_non_finite_data_is_no_data)
{
    const double N = NAN, I = INFINITY;
    const double fin[] = { 0, 1, 2 };
    const std::vector<double> bad[] = { { N, N, N }, { I, I, I }, { -I, N, I } };
    for (blot_plot_type type : { BLOT_SCATTER, BLOT_LINE, BLOT_BAR }) {
        for (const auto &b : bad) {
            SCOPED_TRACE(testing::Message() << "plot " << type << " " << b[0] << " " << b[1] << " " << b[2]);
            RUN_ISOLATED(5, {
                Rendered r = render_one(40, 14, type, BLOT_DATA_DOUBLE, 3, fin, b.data());
                EXPECT_FALSE(r.ok);
                EXPECT_EQ(r.code, ENOENT) << r.message;
                r = render_one(40, 14, type, BLOT_DATA_DOUBLE, 3, b.data(), fin);
                EXPECT_FALSE(r.ok);
                EXPECT_EQ(r.code, ENOENT) << r.message;
                r = render_one(40, 14, type, BLOT_DATA_DOUBLE, 3, NULL, b.data());
                EXPECT_FALSE(r.ok);
                EXPECT_EQ(r.code, ENOENT) << r.message;
            });
        }
    }
}

/* #6: a layer with no finite y next to one with data: the limits and the
 * picture are those of the other layer alone.  Limits are per coordinate,
 * so the finite x of the first layer still count; they are inside the
 * other layer's x range here. */
TEST(Edge, non_finite_layer_beside_finite_layer)
{
    const double xs[] = { 0, 1, 2, 3 }, ys[] = { 4, 1, 7, 2 };
    const double nx[] = { 1, 2, NAN }, ny[] = { NAN, INFINITY, -INFINITY };

    Fig alone(40, 14);
    alone.plot(BLOT_LINE, BLOT_DATA_DOUBLE, 4, xs, ys);
    Rendered want = render(alone, F);
    ASSERT_TRUE(want.ok) << want.message;

    Fig both(40, 14);
    both.plot(BLOT_SCATTER, BLOT_DATA_DOUBLE, 3, nx, ny);
    both.plot(BLOT_LINE, BLOT_DATA_DOUBLE, 4, xs, ys);
    Rendered got = render(both, F);
    ASSERT_TRUE(got.ok) << got.message;
    EXPECT_LINES_EQ(got.lines, want.lines);
}

/* #6: a line {NaN, 5, NaN} draws the 5 as one cell, as a scatter does;
 * a NaN inside a line leaves a gap */
TEST(Edge, line_nan_gaps)
{
    const double xs[] = { 0, 1, 2 }, ys[] = { NAN, 5, NAN };
    Rendered line = render_one(40, 14, BLOT_LINE, BLOT_DATA_DOUBLE, 3, xs, ys);
    Rendered scat = render_one(40, 14, BLOT_SCATTER, BLOT_DATA_DOUBLE, 3, xs, ys);
    ASSERT_TRUE(line.ok) << line.message;
    ASSERT_TRUE(scat.ok) << scat.message;
    expect_no_nan_label(line);
    EXPECT_EQ(plot_cells(line).size(), 1u) << cells_str(plot_cells(line));
    EXPECT_EQ(plot_cells(line), plot_cells(scat)) << cells_str(plot_cells(line));

    /* 0 1 NaN 3 4 on the diagonal: two runs, no mark in the gap's columns */
    const double gx[] = { 0, 1, 2, 3, 4 }, gy[] = { 0, 1, NAN, 3, 4 };
    Rendered gap = render_one(40, 14, BLOT_LINE, BLOT_DATA_DOUBLE, 5, gx, gy);
    ASSERT_TRUE(gap.ok) << gap.message;
    expect_no_nan_label(gap);
    Cells m = plot_cells(gap);
    unsigned pc = gap.plot_cols();
    unsigned c1 = expected_cell(1, -0.004, 4.004, pc), c3 = expected_cell(3, -0.004, 4.004, pc);
    ASSERT_FALSE(m.empty());
    /* within a column of the points' own (the line's X mapping is #14's) */
    for (const auto &c : m)
        EXPECT_TRUE(c.first <= c1 + 1 || c.first + 1 >= c3) << cells_str(m);
    EXPECT_GT(c3, c1 + 8);
    EXPECT_GT(m.size(), 4u) << cells_str(m);
}

/* #6: ranges up to DBL_MAX wide are drawn, every point in its place, with
 * no nan or inf label */
TEST(Edge, huge_range)
{
    const double D = DBL_MAX;
    const std::vector<double> data[] = { { 0, 1e308 }, { 0, D }, { -D, 0 }, { -D / 2, D / 2 }, { 1e308, D } };
    for (blot_plot_type type : { BLOT_SCATTER, BLOT_LINE }) {
        for (const auto &d : data) {
            SCOPED_TRACE(testing::Message() << "plot " << type << " " << d[0] << ".." << d[1]);
            RUN_ISOLATED(5, {
                const double xs[] = { 0, 1 };
                Rendered r = render_one(40, 14, type, BLOT_DATA_DOUBLE, 2, xs, d.data());
                ASSERT_TRUE(r.ok) << r.message;
                expect_no_nan_label(r);
                Cells m = plot_cells(r);
                /* the bottom-left and the top-right cell of the plot */
                EXPECT_TRUE(m.count({ 0, 0 })) << cells_str(m);
                EXPECT_TRUE(m.count({ r.plot_cols() - 1, r.plot_rows() - 1 })) << cells_str(m);
                if (type == BLOT_SCATTER)
                    EXPECT_EQ(m.size(), 2u) << cells_str(m);
            });
        }
    }

    /* one value at the largest double */
    RUN_ISOLATED(5, {
        const double xs[] = { 0 }, ys[] = { D };
        Rendered r = render_one(40, 14, BLOT_SCATTER, BLOT_DATA_DOUBLE, 1, xs, ys);
        ASSERT_TRUE(r.ok) << r.message;
        expect_no_nan_label(r);
        EXPECT_EQ(plot_cells(r).size(), 1u) << cells_str(plot_cells(r));
    });
}

/* #6: data further apart than the largest double (max - min is inf) has no
 * axis: the render fails with ERANGE, it does not draw nan labels */
TEST(Edge, range_wider_than_dbl_max)
{
    const double D = DBL_MAX;
    const double xs[] = { 0, 1 }, ys[] = { -D, D };
    RUN_ISOLATED(5, {
        Rendered r = render_one(40, 14, BLOT_SCATTER, BLOT_DATA_DOUBLE, 2, xs, ys);
        EXPECT_FALSE(r.ok);
        EXPECT_EQ(r.code, ERANGE) << r.message;
        r = render_one(40, 14, BLOT_SCATTER, BLOT_DATA_DOUBLE, 2, ys, xs);
        EXPECT_FALSE(r.ok);
        EXPECT_EQ(r.code, ERANGE) << r.message;
    });
}

TEST(Edge, huge_values)
{
    const double v[] = { -1e300, 1e300 };
    Rendered r = render_one(60, 14, BLOT_LINE, BLOT_DATA_DOUBLE, 2, v, v);
    ASSERT_TRUE(r.ok) << r.message;
    Cells m = plot_cells(r);
    EXPECT_TRUE(m.count({ 0, 0 })) << cells_str(m);
    bool last_col = false;
    for (const auto &c : m)
        last_col |= c.first == r.plot_cols() - 1;
    EXPECT_TRUE(last_col) << cells_str(m);
    EXPECT_EQ(labels_of(y_ticks(r)),
              std::vector<std::string>({ "-8.016e+299", "-4.008e+299", "0.000", "4.008e+299", "8.016e+299" }));
}

namespace {
const double TINY[] = { 1e-9, 2e-9, 3e-9 };
} // namespace

TEST(Edge, tiny_values_geometry)
{
    Rendered r = render_one(60, 14, BLOT_LINE, BLOT_DATA_DOUBLE, 3, TINY, TINY);
    ASSERT_TRUE(r.ok) << r.message;
    Cells m = plot_cells(r);
    EXPECT_TRUE(m.count({ 0, 0 })) << cells_str(m);
    bool last_col = false;
    for (const auto &c : m)
        last_col |= c.first == r.plot_cols() - 1;
    EXPECT_TRUE(last_col) << cells_str(m);
    EXPECT_EQ(r.mrg.left, 8u);
}

// BUG B16: https://git.jukie.net/bart/blot/-/work_items/10
TEST(Edge, DISABLED_tiny_values_labels_distinct)
{
    Rendered r = render_one(60, 14, BLOT_LINE, BLOT_DATA_DOUBLE, 3, TINY, TINY);
    ASSERT_TRUE(r.ok) << r.message;
    for (const auto &ticks : { y_ticks(r), x_ticks(r) }) {
        std::set<std::string> seen;
        for (const auto &t : ticks)
            EXPECT_TRUE(seen.insert(t.second).second) << "duplicate label " << t.second;
    }
}

namespace {
const gint64 I64[] = { G_MININT64, 0, G_MAXINT64 };
} // namespace

TEST(Edge, int64_extremes_line)
{
    Rendered r = render_one(60, 14, BLOT_LINE, BLOT_DATA_INT64, 3, I64, I64);
    ASSERT_TRUE(r.ok) << r.message;
    Cells m = plot_cells(r);
    EXPECT_TRUE(m.count({ 0, 0 })) << cells_str(m);
    EXPECT_TRUE(m.count({ r.plot_cols() - 1, r.plot_rows() - 1 })) << cells_str(m);
    auto labels = labels_of(y_ticks(r));
    EXPECT_NE(std::find(labels.begin(), labels.end(), "0.000"), labels.end());
}

// BUG B14: https://git.jukie.net/bart/blot/-/work_items/9
TEST(Edge, DISABLED_int64_extremes_scatter_narrow)
{
    Rendered r = render_one(40, 14, BLOT_SCATTER, BLOT_DATA_INT64, 3, I64, I64);
    EXPECT_TRUE(r.ok) << r.message;
}

TEST(Edge, data_on_limit_boundaries)
{
    const gint32 xs[] = { 0, 19, 19, 0, 0 }, ys[] = { 0, 0, 9, 9, 0 };
    Fig f;
    fix_a(f);
    f.plot(BLOT_LINE, BLOT_DATA_INT32, 5, xs, ys);
    Rendered r = render(f, F);
    ASSERT_TRUE(r.ok) << r.message;
    Cells border;
    for (unsigned c = 0; c < 20; c++) {
        border.insert({ c, 0 });
        border.insert({ c, 9 });
    }
    for (unsigned rw = 0; rw < 10; rw++) {
        border.insert({ 0, rw });
        border.insert({ 19, rw });
    }
    ASSERT_EQ(border.size(), 56u);
    EXPECT_CELLS_EQ(plot_cells(r), border);
}

TEST(Edge, all_types_render_identically)
{
    const gint16 x16[] = { 0, 19, 10, 5 }, y16[] = { 0, 9, 5, 2 };
    const gint64 x64[] = { 0, 19, 10, 5 }, y64[] = { 0, 9, 5, 2 };
    const float xf[] = { 0, 19, 10, 5 }, yf[] = { 0, 9, 5, 2 };
    const double xd[] = { 0, 19, 10, 5 }, yd[] = { 0, 9, 5, 2 };
    const struct {
        const char *name;
        blot_data_type dt;
        const void *xs, *ys;
    } cases[] = {
        { "INT16", BLOT_DATA_INT16, x16, y16 },
        { "INT32", BLOT_DATA_INT32, SC_X, SC_Y },
        { "INT64", BLOT_DATA_INT64, x64, y64 },
        { "FLOAT", BLOT_DATA_FLOAT, xf, yf },
        { "DOUBLE", BLOT_DATA_DOUBLE, xd, yd },
    };
    for (const auto &c : cases) {
        SCOPED_TRACE(c.name);
        Fig f;
        fix_a(f);
        f.plot(BLOT_SCATTER, c.dt, 4, c.xs, c.ys, 1, "pts");
        Rendered r = render(f, F);
        ASSERT_TRUE(r.ok) << r.message;
        EXPECT_LINES_EQ(rtrim(r.lines), G1);
    }
}
