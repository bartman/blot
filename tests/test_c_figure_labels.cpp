/* blot: the figure owns a copy of its custom X labels (#23)
 *
 * blot_figure_set_x_axis_labels() copies the array and the strings, so
 * the caller may free or reuse its own right after the call; the copy
 * is freed when the labels are replaced or cleared, and by
 * blot_figure_cleanup().  A test that reads freed memory when the figure
 * keeps the caller's pointers instead runs in RUN_ISOLATED: under ASan
 * it is a heap-use-after-free report, without it the labels are garbage
 * or the child crashes. */
#include "blot_test.hpp"

using namespace blot_test;

namespace {

/* four bars, one label under the left edge of each (as in
 * Golden.bar_with_custom_x_labels) */
const gint32 XS[] = { 0, 8, 16, 24 }, YS[] = { 6, 2, 9, 2 };

void plot_bars(Fig &f)
{
    f.lim(0, 31, 0, 11);
    f.plot(BLOT_BAR, BLOT_DATA_INT32, 4, XS, YS);
}

std::vector<std::string> rendered_labels(Fig &f)
{
    Rendered r = render(f, F);
    EXPECT_TRUE(r.ok) << r.message;
    return labels_of(x_ticks(r));
}

const std::vector<std::string> MONTHS = { "jan", "feb", "mar", "apr" };

/* a caller's heap copy of names: a NULL-terminated array (g_strfreev) */
char **heap_labels(const std::vector<std::string> &names)
{
    char **v = g_new0(char *, names.size() + 1);
    for (size_t i = 0; i < names.size(); i++)
        v[i] = g_strdup(names[i].c_str());
    return v;
}

/* overwrite and free the caller's copy: a figure that kept these
 * pointers can't show the original labels */
void scribble_and_free(char **v)
{
    for (char **p = v; *p; p++)
        memset(*p, 'X', strlen(*p));
    g_strfreev(v);
}

} // namespace

TEST(FigureLabels, copied_not_borrowed)
{
    Fig f(41, 14);
    char *labels[] = { (char *)"a", (char *)"b", (char *)"c" };

    ASSERT_TRUE(blot_figure_set_x_axis_labels(f, 3, labels, NULL));
    blot_figure *fig = f;
    ASSERT_EQ(fig->xlabels.count, 3u);
    ASSERT_NE(fig->xlabels.strings, nullptr);
    EXPECT_NE(fig->xlabels.strings, labels);
    for (int i = 0; i < 3; i++) {
        EXPECT_STREQ(fig->xlabels.strings[i], labels[i]);
        EXPECT_NE(fig->xlabels.strings[i], labels[i]) << "label " << i;
    }
}

TEST(FigureLabels, caller_changes_its_array_before_render)
{
    /* no freed memory: the caller reuses its buffers, which a figure
     * holding the caller's pointers would show */
    char buf[4][8] = { "jan", "feb", "mar", "apr" };
    char *labels[] = { buf[0], buf[1], buf[2], buf[3] };
    Fig f(41, 14);
    ASSERT_TRUE(blot_figure_set_x_axis_labels(f, 4, labels, NULL));

    strcpy(buf[0], "XXX");
    labels[1] = (char *)"YYY";
    plot_bars(f);
    EXPECT_EQ(rendered_labels(f), MONTHS);
}

TEST(FigureLabels, caller_frees_its_array_before_render)
{
    RUN_ISOLATED(10, {
        Fig f(41, 14);
        char **labels = heap_labels(MONTHS);
        ASSERT_TRUE(blot_figure_set_x_axis_labels(f, 4, labels, NULL));
        scribble_and_free(labels);

        plot_bars(f);
        EXPECT_EQ(rendered_labels(f), MONTHS);
    });
}

TEST(FigureLabels, replace_twice)
{
    RUN_ISOLATED(10, {
        Fig f(41, 14);
        plot_bars(f);

        /* each set frees the copy it replaces (LeakSanitizer checks the
         * child), and each caller array is gone before the render */
        const std::vector<std::vector<std::string>> sets = {
            { "a", "b", "c", "d" },
            { "one", "two", "three", "four" },
            MONTHS,
        };
        for (const auto &names : sets) {
            char **labels = heap_labels(names);
            ASSERT_TRUE(blot_figure_set_x_axis_labels(f, names.size(), labels, NULL));
            scribble_and_free(labels);
            EXPECT_EQ(rendered_labels(f), names);
        }
    });
}

TEST(FigureLabels, replace_with_its_own_labels)
{
    /* the figure's current copy as the new labels: copied before the
     * old one is freed */
    RUN_ISOLATED(10, {
        Fig f(41, 14);
        plot_bars(f);
        char **labels = heap_labels(MONTHS);
        ASSERT_TRUE(blot_figure_set_x_axis_labels(f, 4, labels, NULL));
        scribble_and_free(labels);

        blot_figure *fig = f;
        ASSERT_TRUE(blot_figure_set_x_axis_labels(f, fig->xlabels.count, fig->xlabels.strings, NULL));
        EXPECT_EQ(rendered_labels(f), MONTHS);
    });
}

TEST(FigureLabels, clear_with_null)
{
    RUN_ISOLATED(10, {
        Fig f(41, 14);
        plot_bars(f);
        std::vector<std::string> numeric = rendered_labels(f);
        ASSERT_FALSE(numeric.empty());
        EXPECT_NE(numeric, MONTHS);

        char **labels = heap_labels(MONTHS);
        ASSERT_TRUE(blot_figure_set_x_axis_labels(f, 4, labels, NULL));
        scribble_and_free(labels);
        EXPECT_EQ(rendered_labels(f), MONTHS);

        /* NULL/0 clears them (and frees the copy): back to numbers */
        ASSERT_TRUE(blot_figure_set_x_axis_labels(f, 0, NULL, NULL));
        blot_figure *fig = f;
        EXPECT_EQ(fig->xlabels.count, 0u);
        EXPECT_EQ(fig->xlabels.strings, nullptr);
        EXPECT_EQ(rendered_labels(f), numeric);

        /* clearing what is already clear is fine */
        ASSERT_TRUE(blot_figure_set_x_axis_labels(f, 0, NULL, NULL));
        EXPECT_EQ(rendered_labels(f), numeric);
    });
}

TEST(FigureLabels, invalid_set_keeps_the_labels)
{
    RUN_ISOLATED(10, {
        Fig f(41, 14);
        plot_bars(f);
        char **labels = heap_labels(MONTHS);
        ASSERT_TRUE(blot_figure_set_x_axis_labels(f, 4, labels, NULL));
        scribble_and_free(labels);

        GError *error = NULL;
        char *other[] = { (char *)"x" };
        EXPECT_FALSE(blot_figure_set_x_axis_labels(f, 0, other, &error));
        EXPECT_TRUE(error != NULL);
        g_clear_error(&error);
        EXPECT_FALSE(blot_figure_set_x_axis_labels(f, 1, NULL, &error));
        EXPECT_TRUE(error != NULL);
        g_clear_error(&error);

        EXPECT_EQ(rendered_labels(f), MONTHS);
    });
}

TEST(FigureLabels, freed_by_cleanup)
{
    /* LeakSanitizer (the child's check) sees the copy if cleanup
     * doesn't free it; also a figure on the stack (init/cleanup) */
    RUN_ISOLATED(10, {
        blot_figure fig;
        ASSERT_TRUE(blot_figure_init(&fig, NULL));
        char **labels = heap_labels(MONTHS);
        ASSERT_TRUE(blot_figure_set_x_axis_labels(&fig, 4, labels, NULL));
        g_strfreev(labels);
        blot_figure_cleanup(&fig);

        blot_figure *heap = blot_figure_new(NULL);
        ASSERT_TRUE(heap != NULL);
        labels = heap_labels(MONTHS);
        ASSERT_TRUE(blot_figure_set_x_axis_labels(heap, 4, labels, NULL));
        g_strfreev(labels);
        blot_figure_delete(heap);
    });
}
