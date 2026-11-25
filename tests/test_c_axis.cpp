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
        EXPECT_EQ(a.labels(), zip({ 6, 17, 27, 38, 48 }, { "-0.601", "0.000", "0.601", "1.202", "1.804" }));
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
                EXPECT_EQ(a.ticks().size(), n)
                    << (vertical ? "V" : "H") << " len " << len << " [" << rg[0] << "," << rg[1] << "]";
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

// BUG B14: https://git.jukie.net/bart/blot/-/work_items/9
TEST(Axis, DISABLED_narrow_axis_with_long_labels)
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

// BUG B14: https://git.jukie.net/bart/blot/-/work_items/9
// BUG B16: https://git.jukie.net/bart/blot/-/work_items/10
TEST(Axis, DISABLED_labels_are_distinct)
{
    const struct {
        bool vertical;
        unsigned len;
        double min, max;
    } cases[] = {
        { false, 21, -200100, -99900 },
        { true, 12, 1e-9, 3e-9 },
    };
    for (const auto &c : cases) {
        SCOPED_TRACE(testing::Message() << (c.vertical ? "V" : "H") << " len " << c.len
                                        << " [" << c.min << "," << c.max << "]");
        Axis a(c.vertical, c.len, c.min, c.max);
        if (!a.axs) {
            ADD_FAILURE() << a.error;
            continue;
        }
        std::set<std::string> seen;
        for (const auto &t : a.ticks())
            EXPECT_TRUE(seen.insert(t.label).second) << "duplicate label " << t.label;
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
    auto ticks = a.ticks();
    ASSERT_EQ(ticks.size(), 5u);
    /* every index has a tick, and the labels keep their order (today
     * b c e f g: several labels round to one cell, the last one wins) */
    std::string order;
    for (const auto &t : ticks)
        order += t.label;
    EXPECT_TRUE(std::is_sorted(order.begin(), order.end())) << order;
    EXPECT_EQ(std::set<char>(order.begin(), order.end()).size(), order.size()) << order;
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
