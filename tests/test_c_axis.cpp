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

/* --- tick positions, values and labels (#2, group A) -------------------
 *
 * blot_axis_new(is_vertical, true, 7, len, min, max, labels) and every
 * index read back with blot_axis_get_tick_at(). */

#include <algorithm>
#include <cmath>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace {

struct Tick {
    unsigned index;
    double value;
    std::string label;
};

/* an axis that is deleted at the end of the scope */
struct Axis {
    blot_axis *axs = NULL;
    std::string error;
    Axis(bool vertical, unsigned len, double min, double max, const blot_strv *labels = NULL)
    {
        GError *e = NULL;
        axs = blot_axis_new(vertical, true, 7, len, min, max, labels, &e);
        if (e)
            error = e->message;
        g_clear_error(&e);
    }
    ~Axis() { blot_axis_delete(axs); }
    Axis(const Axis &) = delete;
    Axis &operator=(const Axis &) = delete;

    std::vector<Tick> ticks() const
    {
        std::vector<Tick> out;
        for (unsigned i = 0; i < axs->screen_length; i++)
            if (const blot_axis_tick *t = blot_axis_get_tick_at(axs, i, NULL))
                out.push_back({ i, t->value, t->label });
        return out;
    }

    /* {index: label} of every tick */
    std::map<unsigned, std::string> labels() const
    {
        std::map<unsigned, std::string> out;
        for (const auto &t : ticks())
            out[t.index] = t.label;
        return out;
    }
};

/* the results (failures) recorded so far in the current test: a sweep
 * leaves a range at its first failing size, so that a failure doesn't
 * flood the log (as blot_test.hpp) */
int result_parts()
{
    return ::testing::UnitTest::GetInstance()->current_test_info()->result()->total_part_count();
}

std::map<unsigned, std::string> zip(const std::vector<unsigned> &idx, const std::vector<std::string> &lbl)
{
    std::map<unsigned, std::string> out;
    for (size_t i = 0; i < idx.size() && i < lbl.size(); i++)
        out[idx[i]] = lbl[i];
    return out;
}

const double SWEEP_RANGES[][2] = {
    { 0, 1 }, { 0, 9 }, { 0, 10 }, { -1, 2 }, { -7, 3 }, { 0, 1000 },
    { 0.001, 0.002 }, { -250, -200 }, { 5, 6 }, { 0, 100 },
};

} // namespace

TEST(Axis, horizontal_tick_positions_and_labels)
{
    Axis a(false, 60, 0, 60);
    ASSERT_TRUE(a.axs) << a.error;
    EXPECT_EQ(a.labels(), zip({ 0, 8, 17, 25, 34, 42, 51 },
                              { "0.000", "8.571", "17.143", "25.714", "34.286", "42.857", "51.429" }));
}

TEST(Axis, vertical_tick_positions_and_labels)
{
    {
        Axis a(true, 12, 0, 12);
        ASSERT_TRUE(a.axs) << a.error;
        EXPECT_EQ(a.labels(), zip({ 0, 2, 4, 7, 9 }, { "0.000", "2.400", "4.800", "7.200", "9.600" }));
    }
    {
        Axis a(true, 10, 0, 1);
        ASSERT_TRUE(a.axs) << a.error;
        EXPECT_EQ(a.labels(), zip({ 0, 3, 6 }, { "0.000", "0.333", "0.667" }));
    }
    {
        Axis a(true, 30, -3, -1);
        ASSERT_TRUE(a.axs) << a.error;
        EXPECT_EQ(a.labels(), zip({ 0, 2, 5, 8, 10, 13, 16, 19, 21, 24, 27 },
                                  { "-3.000", "-2.818", "-2.636", "-2.455", "-2.273", "-2.091",
                                    "-1.909", "-1.727", "-1.545", "-1.364", "-1.182" }));
    }
}

TEST(Axis, zero_alignment)
{
    {
        Axis a(false, 100, -50, 50);
        ASSERT_TRUE(a.axs) << a.error;
        std::vector<unsigned> idx;
        for (const auto &t : a.ticks())
            idx.push_back(t.index);
        EXPECT_EQ(idx, std::vector<unsigned>({ 4, 13, 22, 31, 40, 50, 59, 68, 77, 86, 95 }));
        const blot_axis_tick *zero = blot_axis_get_tick_at(a.axs, 50, NULL);
        ASSERT_TRUE(zero);
        EXPECT_EQ(zero->value, 0.0);
        EXPECT_STREQ(zero->label, "0.000");
    }
    {
        Axis a(false, 52, -1.003, 2.003);
        ASSERT_TRUE(a.axs) << a.error;
        /* the last label has 4 columns before the edge (#9: 1.804 ran
         * one past it), so it is shortened; 3 * 0.6012 = 1.8036 */
        EXPECT_EQ(a.labels(), zip({ 6, 17, 27, 38, 48 }, { "-0.601", "0.000", "0.601", "1.202", "1.80" }));
    }
    {
        Axis a(true, 18, -1.003, 2.003);
        ASSERT_TRUE(a.axs) << a.error;
        EXPECT_EQ(a.labels(), zip({ 0, 3, 6, 8, 11, 13, 16 },
                                  { "-0.859", "-0.429", "0.000", "0.429", "0.859", "1.288", "1.718" }));
    }
    {
        Axis a(false, 31, -0.1, 0.1);
        ASSERT_TRUE(a.axs) << a.error;
        EXPECT_EQ(a.labels(), zip({ 5, 15, 25 }, { "-0.067", "0.000", "0.067" }));
    }
}

TEST(Axis, zero_tick_sweep)
{
    for (unsigned len = 10; len <= 120; len++) {
        for (bool vertical : { false, true }) {
            for (double min = -10; min < 0; min += 0.37) {
                for (double max = 0.5; max < 10; max += 0.61) {
                    Axis a(vertical, len, min, max);
                    ASSERT_TRUE(a.axs) << a.error << " len " << len << " [" << min << "," << max << "]";
                    bool found = false;
                    for (const auto &t : a.ticks()) {
                        if (std::fabs(t.value) >= 1e-9)
                            continue;
                        found = true;
                        EXPECT_EQ(strtod(t.label.c_str(), NULL), 0.0) << t.label;
                        EXPECT_NE(t.label[0], '-') << t.label;
                    }
                    ASSERT_TRUE(found) << (vertical ? "V" : "H") << " len " << len
                                       << " [" << min << "," << max << "]";
                }
            }
        }
    }
}

TEST(Axis, tick_value_matches_its_cell)
{
    for (unsigned len = 10; len <= 200; len++) {
        for (bool vertical : { false, true }) {
            for (const auto &rg : SWEEP_RANGES) {
                double min = rg[0], max = rg[1], range = max - min;
                Axis a(vertical, len, min, max);
                ASSERT_TRUE(a.axs) << a.error;
                double prev = -INFINITY;
                for (const auto &t : a.ticks()) {
                    double lo = min + t.index * range / len, hi = min + (t.index + 1) * range / len;
                    double tol = 1e-9 * range;
                    EXPECT_TRUE(lo - tol <= t.value && t.value < hi + tol)
                        << (vertical ? "V" : "H") << " len " << len << " [" << min << "," << max
                        << "] tick " << t.index << " value " << t.value << " not in [" << lo << "," << hi << ")";
                    EXPECT_GT(t.value, prev);
                    prev = t.value;
                }
            }
        }
    }
}

/* the ticks of the A6 formula with the zero alignment, as blot_axis_new()
 * lays them out: {cell, value} */
static std::vector<std::pair<unsigned, double>> formula_ticks(unsigned len, unsigned n, double min, double max)
{
    double d_jump = (max - min) / n, s_jump = (double)len / n, d_base = 0, s_base = 0;
    if (min < 0 && 0 < max) {
        double t_shift = -min / d_jump;
        unsigned ti = std::floor(t_shift);
        d_base = -((ti * d_jump) + min);
        s_base = (t_shift - ti) * s_jump;
    }
    std::vector<std::pair<unsigned, double>> out;
    for (unsigned i = 0; i < n; i++)
        out.push_back({ (unsigned)(s_base + i * s_jump), d_base + i * d_jump + min });
    return out;
}

TEST(Axis, tick_count_formula)
{
    for (unsigned len = 10; len <= 200; len++) {
        for (bool vertical : { false, true }) {
            for (const auto &rg : SWEEP_RANGES) {
                Axis a(vertical, len, rg[0], rg[1]);
                ASSERT_TRUE(a.axs) << a.error;
                unsigned n = vertical ? len / 3 : len / 10;
                if (!(n & 1))
                    n++;
                auto ticks = a.ticks();
                SCOPED_TRACE(testing::Message() << (vertical ? "V" : "H") << " len " << len
                                                << " [" << rg[0] << "," << rg[1] << "]");
                if (!vertical && len < 24 && rg[0] == 0.001 && ticks.size() == 2) {
                    /* #10: 0.00133 needs 6 columns of 5, and 0.001 for it
                     * is 13 half-cells off: two ticks, 0.001 and 0.0015 */
                    EXPECT_EQ(n, 3u);
                    continue;
                }
                if (ticks.size() + 1 == n && !vertical && rg[0] < 0 && 0 < rg[1]) {
                    /* #9: the zero alignment pushed the last tick so close
                     * to the edge that no good text of its value fits;
                     * that one tick is dropped, the others keep their
                     * cells and values */
                    auto f = formula_ticks(len, n, rg[0], rg[1]);
                    for (unsigned i = 0; i + 1 < n; i++) {
                        EXPECT_EQ(ticks[i].index, f[i].first) << "tick " << i;
                        EXPECT_EQ(ticks[i].value, f[i].second) << "tick " << i;
                    }
                    /* the dropped one was in the last tick spacing */
                    EXPECT_GE(f[n - 1].first + std::floor((double)len / n), len);
                    continue;
                }
                EXPECT_EQ(ticks.size(), n);
            }
        }
    }
}

TEST(Axis, horizontal_labels_fit_spacing)
{
    std::vector<std::pair<double, double>> ranges;
    for (const auto &rg : SWEEP_RANGES)
        ranges.push_back({ rg[0], rg[1] });
    ranges.push_back({ 0, 1e6 });
    ranges.push_back({ -1e6, 0 });
    ranges.push_back({ 0, 1e12 });

    for (unsigned len = 24; len <= 200; len++) {
        for (const auto &rg : ranges) {
            Axis a(false, len, rg.first, rg.second);
            ASSERT_TRUE(a.axs) << a.error << " len " << len << " [" << rg.first << "," << rg.second << "]";
            auto ticks = a.ticks();
            ASSERT_FALSE(ticks.empty());
            unsigned spacing = len / ticks.size();
            for (const auto &t : ticks)
                EXPECT_LT(t.label.size(), spacing)
                    << "len " << len << " [" << rg.first << "," << rg.second << "] label " << t.label;
        }
    }
}

// #9 (B14): creation failed
TEST(Axis, narrow_axis_with_long_labels)
{
    {
        Axis a(false, 20, -200100, -99900);
        EXPECT_TRUE(a.axs) << a.error;
    }
    {
        Axis a(false, 21, -1e100, 1e100);
        EXPECT_TRUE(a.axs) << a.error;
    }
}

// #9 (B14): with room for 6 characters -200100 and -166700 were both -2e+05
TEST(Axis, x_labels_are_distinct)
{
    Axis a(false, 21, -200100, -99900);
    ASSERT_TRUE(a.axs) << a.error;
    std::set<std::string> seen;
    for (const auto &t : a.ticks())
        EXPECT_TRUE(seen.insert(t.label).second) << "duplicate label " << t.label;
    /* fewer ticks, each with the value of its cell */
    EXPECT_EQ(a.labels(), zip({ 0, 10 }, { "-200100.0", "-150000.0" }));
}

// B16, fixed in #10: https://git.jukie.net/bart/blot/-/work_items/10
TEST(Axis, labels_are_distinct)
{
    Axis a(true, 12, 1e-9, 3e-9);
    ASSERT_TRUE(a.axs) << a.error;
    std::set<std::string> seen;
    for (const auto &t : a.ticks())
        EXPECT_TRUE(seen.insert(t.label).second) << "duplicate label " << t.label;
}

/* #10: tiny ranges, exact.  The labels have the digits the tick spacing
 * needs (they were all 0.000, or 0.001 for 0.0012 and 0.0014), and zero
 * is not negative */
TEST(Axis, tiny_range_labels_exact)
{
    {
        Axis a(true, 12, 1e-9, 3e-9);
        ASSERT_TRUE(a.axs) << a.error;
        EXPECT_EQ(a.labels(), zip({ 0, 2, 4, 7, 9 }, { "1e-09", "1.4e-09", "1.8e-09", "2.2e-09", "2.6e-09" }));
    }
    {
        Axis a(false, 50, 0.001, 0.002);
        ASSERT_TRUE(a.axs) << a.error;
        EXPECT_EQ(a.labels(), zip({ 0, 10, 20, 30, 40 }, { "0.001", "0.0012", "0.0014", "0.0016", "0.0018" }));
    }
    {
        /* three ticks have 6 columns each: 0.00133 doesn't fit, and
         * 0.001 is 13 half-cells off; two ticks */
        Axis a(false, 20, 0.001, 0.002);
        ASSERT_TRUE(a.axs) << a.error;
        EXPECT_EQ(a.labels(), zip({ 0, 10 }, { "0.001", "0.0015" }));
    }
    {
        Axis a(true, 12, -0.0005, 0.0005);
        ASSERT_TRUE(a.axs) << a.error;
        EXPECT_EQ(a.labels(), zip({ 1, 3, 6, 8, 10 }, { "-0.0004", "-0.0002", "0.000", "0.0002", "0.0004" }));
    }
}

/* #10: every label of a vertical axis (with all of its 15 columns) names
 * a value within half a cell of its tick; none is a negative zero */
TEST(Axis, y_labels_within_half_a_cell_sweep)
{
    const double RANGES[][2] = {
        { 1e-9, 3e-9 }, { -3e-9, -1e-9 }, { 0.001, 0.002 }, { -0.0005, 0.0005 }, { -0.01003, 0.02003 },
        { -0.0019955, 0.0194945 }, { 0, 1 }, { -7.01, 3.01 }, { 1e9, 5.2e9 }, { -200100, -99900 },
        { 1e6, 1e6 + 0.001 }, { -1e100, 1e100 }, { 1e-300, 3e-300 },
    };
    for (const auto &rg : RANGES) {
        int parts = result_parts();
        for (unsigned len = 3; len <= 200 && result_parts() == parts; len++) {
            SCOPED_TRACE(testing::Message() << "V len " << len << " [" << rg[0] << "," << rg[1] << "]");
            Axis a(true, len, rg[0], rg[1]);
            ASSERT_TRUE(a.axs) << a.error;
            double cell = (rg[1] - rg[0]) / len;
            std::set<std::string> seen;
            for (const auto &t : a.ticks()) {
                char *end = NULL;
                double v = strtod(t.label.c_str(), &end);
                EXPECT_TRUE(!t.label.empty() && *end == 0) << "tick " << t.index << " label '" << t.label << "'";
                EXPECT_LE(std::fabs(v - t.value), 0.5 * cell * (1 + 1e-9))
                    << "tick " << t.index << " " << t.value << " label '" << t.label << "'";
                EXPECT_FALSE(t.label[0] == '-' && v == 0) << "tick " << t.index << " label '" << t.label << "'";
                EXPECT_LE(t.label.size(), 15u);
                EXPECT_TRUE(seen.insert(t.label).second) << "duplicate label " << t.label;
            }
        }
    }
}

/* #10: Y labels in a narrow margin (blot_axis_new_with_room): each fits
 * the columns and names a value within half a cell of its tick, or no
 * further off than its text with all 15 columns; else the ticks are
 * fewer, down to one at the start of the axis, whose label is empty when
 * even that has no such text */
TEST(Axis, y_labels_in_room_sweep)
{
    const double RANGES[][2] = {
        { 1e-9, 3e-9 }, { -3e-9, -1e-9 }, { 0.001, 0.002 }, { -0.0005, 0.0005 }, { -0.01003, 0.02003 },
        { 0, 1 }, { -7.01, 3.01 }, { 1e9, 5.2e9 }, { -200100, -99900 }, { 0, 12345678901.0 },
        { 1e6, 1e6 + 0.001 }, { -1e100, 1e100 }, { -1e15, 1e15 },
    };
    for (const auto &rg : RANGES) {
        int parts = result_parts();
        for (unsigned cols = 0; cols <= 16; cols++) {
            for (unsigned len = 3; len <= 60 && result_parts() == parts; len++) {
                SCOPED_TRACE(testing::Message() << "V len " << len << " cols " << cols << " [" << rg[0] << ","
                                                << rg[1] << "]");
                GError *e = NULL;
                blot_axis *axs = blot_axis_new_with_room(true, true, 7, len, rg[0], rg[1], NULL, cols, &e);
                ASSERT_TRUE(axs) << e->message;
                double half_cell = 0.5 * (rg[1] - rg[0]) / len;
                std::set<std::string> seen;
                unsigned n = 0;
                for (unsigned i = 0; i < len; i++) {
                    const blot_axis_tick *t = blot_axis_get_tick_at(axs, i, NULL);
                    if (!t)
                        continue;
                    n++;
                    std::string label = t->label;
                    EXPECT_LE(label.size(), std::min(cols, 15u)) << "label '" << label << "'";
                    if (label.empty()) {
                        /* only a lone tick at the start */
                        EXPECT_EQ(i, 0u);
                        continue;
                    }
                    char full[17];
                    ASSERT_GE(blot_format_number_within(full, 16, t->value, half_cell), 0);
                    double v = strtod(label.c_str(), NULL);
                    double err = std::fabs(v - t->value), full_err = std::fabs(strtod(full, NULL) - t->value);
                    EXPECT_LE(err, std::max(half_cell, full_err * (1 + 1e-9)))
                        << "tick " << i << " " << t->value << " label '" << label << "' full '" << full << "'";
                    EXPECT_FALSE(label[0] == '-' && v == 0) << "label '" << label << "'";
                    EXPECT_TRUE(seen.insert(label).second) << "duplicate label " << label;
                }
                EXPECT_GE(n, 1u);
                if (seen.empty()) {
                    EXPECT_EQ(n, 1u);
                }
                blot_axis_delete(axs);
            }
        }
    }
}

TEST(Axis, custom_labels_evenly_spaced)
{
    /* no zero alignment with custom labels */
    char *abc[] = { (char *)"a", (char *)"b", (char *)"c" };
    blot_strv l3 = { 3, abc };
    Axis a(false, 30, -5, 5, &l3);
    ASSERT_TRUE(a.axs) << a.error;
    EXPECT_EQ(a.labels(), zip({ 0, 10, 20 }, { "a", "b", "c" }));

    char *abcd[] = { (char *)"a", (char *)"b", (char *)"c", (char *)"d" };
    blot_strv l4 = { 4, abcd };
    Axis b(false, 32, 0, 3, &l4);
    ASSERT_TRUE(b.axs) << b.error;
    EXPECT_EQ(b.labels(), zip({ 0, 8, 16, 24 }, { "a", "b", "c", "d" }));
}

TEST(Axis, more_custom_labels_than_cells)
{
    char *ag[] = { (char *)"a", (char *)"b", (char *)"c", (char *)"d", (char *)"e", (char *)"f", (char *)"g" };
    blot_strv l7 = { 7, ag };
    Axis a(false, 5, 0, 7, &l7);
    ASSERT_TRUE(a.axs) << a.error;
    /* #9: 7 labels can't each have a tick in 5 cells (several labels
     * rounded to one cell and the last one won, hiding a and d); every
     * third label gets a tick, at its own cell, with a blank between */
    EXPECT_EQ(a.labels(), zip({ 0, 2, 4 }, { "a", "d", "g" }));
}

/* #9 review M1: a label names a value within half a cell (range / len)
 * of its tick, or the tick is dropped or the ticks are fewer */
TEST(Axis, x_labels_within_half_a_cell)
{
    {
        /* 0.018036 at 48 has 4 columns: 0.02 is 3.4 cells off, the tick
         * is dropped (master: 0.018, one column past the axis) */
        Axis a(false, 52, -0.01003, 0.02003);
        ASSERT_TRUE(a.axs) << a.error;
        EXPECT_EQ(a.labels(), zip({ 6, 17, 27, 38 }, { "-0.006", "0.000", "0.006", "0.012" }));
    }
    {
        /* three ticks have 5 characters each: 2e+09 for 2.4e9 is 2 cells
         * off; two ticks, 1e9 and 3.1e9 */
        Axis a(false, 21, 1e9, 5.2e9);
        ASSERT_TRUE(a.axs) << a.error;
        EXPECT_EQ(a.labels(), zip({ 0, 10 }, { "1e+09", "3.1e+09" }));
    }
}

/* #9 re-review M1b: when even the full text is more than half a cell
 * off (%.3f is the formatter's first choice), a shorter text may be no
 * further off: 0.01842 at 66 has 4 columns, 0.02 is 5 half-cells off and
 * the full 0.018 less than 3, so the tick is dropped.  #10: the full text
 * has the digits half a cell (0.0001535) needs: 0.0092 for 0.00921, not
 * 0.009, 1.4 half-cells off (0.0123 for 0.01228, 0.0153 for 0.01535) */
TEST(Axis, x_label_no_worse_than_full_text)
{
    Axis a(false, 70, -0.0019955, 0.0194945);
    ASSERT_TRUE(a.axs) << a.error;
    EXPECT_EQ(a.labels(), zip({ 6, 16, 26, 36, 46, 56 }, { "0.000", "0.003", "0.006", "0.0092", "0.0123", "0.0153" }));
}

/* #9: every X number label names a value within half a cell of its tick,
 * or, when even the formatter's text with all the room is further off,
 * no further than that; and it is in reach of the tick's own cell.  Also
 * the ranges where %.3f is coarser than a cell.  #10: with the digits the
 * tick spacing needs, the full text is within half a cell in all of
 * these, so each label is too; none is a negative zero */
TEST(Axis, x_labels_no_worse_than_needed_sweep)
{
    const double RANGES[][2] = {
        { -0.01003, 0.02003 }, { -0.0019955, 0.0194945 }, { 0.001, 0.002 }, { -7.01, 3.01 },
        { -1.003, 2.003 }, { 1e9, 5.2e9 }, { -200100, -99900 }, { -1e100, 1e100 },
        { 1e-9, 3e-9 }, { -0.0005, 0.0005 }, { 1e6, 1e6 + 0.001 },
    };
    for (const auto &rg : RANGES) {
        int parts = result_parts();
        for (unsigned len = 10; len <= 200 && result_parts() == parts; len++) {
            SCOPED_TRACE(testing::Message() << "H len " << len << " [" << rg[0] << "," << rg[1] << "]");
            Axis a(false, len, rg[0], rg[1]);
            ASSERT_TRUE(a.axs) << a.error;
            double cell = (rg[1] - rg[0]) / len;
            std::set<std::string> seen;
            for (const auto &t : a.ticks()) {
                double v = strtod(t.label.c_str(), NULL);
                EXPECT_LE(std::fabs(v - t.value), 0.5 * cell * (1 + 1e-9))
                    << "tick " << t.index << " " << t.value << " label '" << t.label << "'";
                EXPECT_FALSE(t.label[0] == '-' && v == 0) << "tick " << t.index << " label '" << t.label << "'";
                EXPECT_LE(t.label.size(), 16u);
                EXPECT_TRUE(seen.insert(t.label).second) << "duplicate label " << t.label;
            }
        }
    }
}

/* #9 review M2: custom X labels are written whole when there is room, the
 * 16-character cap is for numbers */
TEST(Axis, long_custom_x_labels_whole)
{
    char *w[] = { (char *)"a-very-long-category-name-1", (char *)"a-very-long-category-name-2" };
    blot_strv l = { 2, w };
    Axis a(false, 92, 0, 2, &l);
    ASSERT_TRUE(a.axs) << a.error;
    EXPECT_EQ(a.labels(), zip({ 0, 46 }, { w[0], w[1] }));
}

/* #9 review L1: cut custom labels never become ambiguous: the smallest
 * step that makes them so; a '~' only on a lone label longer than the
 * axis */
TEST(Axis, cut_custom_x_labels_not_ambiguous)
{
    char *d[] = { (char *)"2024-01-15", (char *)"2024-01-16", (char *)"2024-01-17", (char *)"2024-01-18" };
    blot_strv l = { 4, d };
    {
        /* 7 columns each would give 2024-01 four times: every other
         * label, whole */
        Axis a(false, 32, 0, 4, &l);
        ASSERT_TRUE(a.axs) << a.error;
        EXPECT_EQ(a.labels(), zip({ 0, 16 }, { "2024-01-15", "2024-01-17" }));
    }
    {
        /* every other label: 2024-01 next to 2024-01-; every third:
         * 2024-01-15 next to 202; only the first, whole */
        Axis a(false, 16, 0, 4, &l);
        ASSERT_TRUE(a.axs) << a.error;
        EXPECT_EQ(a.labels(), zip({ 0 }, { "2024-01-15" }));
    }
    {
        /* even one label is longer than the axis: cut, and marked */
        char *w[] = { (char *)"a-label-longer-than-the-axis-1", (char *)"a-label-longer-than-the-axis-2" };
        blot_strv lw = { 2, w };
        Axis a(false, 12, 0, 2, &lw);
        ASSERT_TRUE(a.axs) << a.error;
        EXPECT_EQ(a.labels(), zip({ 0 }, { "a-label-lon~" }));
    }
    {
        /* different prefixes stay plain cuts */
        char *n[] = { (char *)"first-label-xxxx", (char *)"second-label-xxx", (char *)"third-label-xxxx",
                      (char *)"fourth-label-xxx" };
        blot_strv ln = { 4, n };
        Axis a(false, 32, 0, 4, &ln);
        ASSERT_TRUE(a.axs) << a.error;
        EXPECT_EQ(a.labels(), zip({ 0, 8, 16, 24 }, { "first-l", "second-", "third-l", "fourth-l" }));
    }
}

TEST(Axis, large_axis_label_storage)
{
    for (bool vertical : { true, false }) {
        Axis a(vertical, 1000, -1e15, 1e15);
        ASSERT_TRUE(a.axs) << a.error;
        for (unsigned i = 0; i < 1000; i++) {
            const blot_axis_tick *t = blot_axis_get_tick_at(a.axs, i, NULL);
            if (!t)
                continue;
            EXPECT_LE(strnlen(t->label, 64), 15u) << t->label;
        }
    }
}

// BUG B6, B7: https://git.jukie.net/bart/blot/-/work_items/6
TEST(Axis, DISABLED_non_finite_limits_rejected)
{
    const double lim[][2] = { { NAN, 1 }, { 0, NAN }, { 0, INFINITY }, { -INFINITY, 0 } };
    for (const auto &l : lim) {
        SCOPED_TRACE(testing::Message() << "[" << l[0] << "," << l[1] << "]");
        GError *error = NULL;
        blot_axis *axs = blot_axis_new(true, true, 7, 20, l[0], l[1], NULL, &error);
        EXPECT_TRUE(axs == NULL);
        EXPECT_TRUE(error != NULL);
        g_clear_error(&error);
        blot_axis_delete(axs);
    }
}

TEST(Axis, invisible_axis)
{
    GError *error = NULL;
    blot_axis *axs = blot_axis_new(false, false, 7, 50, 2, 8, NULL, &error);
    ASSERT_TRUE(axs != NULL);
    for (unsigned i = 0; i < 60; i++)
        EXPECT_TRUE(blot_axis_get_tick_at(axs, i, NULL) == NULL) << i;
    EXPECT_EQ(axs->screen_length, 50u);
    EXPECT_EQ(axs->data_min, 2);
    EXPECT_EQ(axs->data_max, 8);
    blot_axis_delete(axs);
}
