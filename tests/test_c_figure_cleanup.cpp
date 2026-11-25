/* blot: blot_figure_cleanup() is idempotent (#28)
 *
 * cleanup frees the layers and the figure's copy of the X labels, and
 * leaves the figure with none (layers=NULL, layer_count=0, no labels),
 * so a second cleanup, a blot_figure_delete() after a cleanup, or an
 * init after it frees nothing twice.  The tests that would free twice
 * when cleanup leaves the old pointers behind run in RUN_ISOLATED: under
 * ASan it is a double-free report, without it glibc may abort or say
 * nothing at all. */
#include "blot_test.hpp"

using namespace blot_test;

namespace {

const gint32 XS[] = { 0, 8, 16, 24 }, YS[] = { 6, 2, 9, 2 };

const std::vector<std::string> MONTHS = { "jan", "feb", "mar", "apr" };
const std::vector<std::string> DAYS = { "mon", "tue", "wed", "thu" };

/* two layers and four labels: every owned field has something in it */
void fill(blot_figure *fig, const std::vector<std::string> &names)
{
    GError *error = NULL;
    ASSERT_TRUE(blot_figure_set_screen_size(fig, 41, 14, &error));
    ASSERT_TRUE(blot_figure_set_x_limits(fig, 0, 31, &error));
    ASSERT_TRUE(blot_figure_set_y_limits(fig, 0, 11, &error));
    ASSERT_TRUE(blot_figure_plot(fig, BLOT_BAR, BLOT_DATA_INT32, 4, XS, YS, 1, NULL, &error));
    ASSERT_TRUE(blot_figure_plot(fig, BLOT_SCATTER, BLOT_DATA_INT32, 4, XS, YS, 2, NULL, &error));

    std::vector<char *> labels;
    for (const auto &n : names)
        labels.push_back((char *)n.c_str());
    ASSERT_TRUE(blot_figure_set_x_axis_labels(fig, labels.size(), labels.data(), &error));
    ASSERT_EQ(error, nullptr);
    ASSERT_EQ(fig->layer_count, 2u);
    ASSERT_EQ(fig->xlabels.count, names.size());
}

void expect_empty(const blot_figure *fig)
{
    EXPECT_EQ(fig->layers, nullptr);
    EXPECT_EQ(fig->layer_count, 0u);
    EXPECT_EQ(fig->xlabels.strings, nullptr);
    EXPECT_EQ(fig->xlabels.count, 0u);
}

std::vector<std::string> rendered_labels(blot_figure *fig)
{
    Rendered r = render(fig, F);
    EXPECT_TRUE(r.ok) << r.message;
    return labels_of(x_ticks(r));
}

} // namespace

TEST(FigureCleanup, twice_with_layers_and_labels)
{
    RUN_ISOLATED(10, {
        blot_figure fig;
        ASSERT_TRUE(blot_figure_init(&fig, NULL));
        fill(&fig, MONTHS);

        blot_figure_cleanup(&fig);
        expect_empty(&fig);

        /* frees nothing again */
        blot_figure_cleanup(&fig);
        expect_empty(&fig);
    });
}

TEST(FigureCleanup, twice_when_empty)
{
    RUN_ISOLATED(10, {
        blot_figure fig;
        ASSERT_TRUE(blot_figure_init(&fig, NULL));
        blot_figure_cleanup(&fig);
        blot_figure_cleanup(&fig);
        expect_empty(&fig);
    });
}

TEST(FigureCleanup, delete_after_cleanup)
{
    /* an error path that cleans up, then the owner deletes the figure */
    RUN_ISOLATED(10, {
        blot_figure *fig = blot_figure_new(NULL);
        ASSERT_NE(fig, nullptr);
        fill(fig, MONTHS);

        blot_figure_cleanup(fig);
        expect_empty(fig);
        blot_figure_delete(fig);
    });
}

TEST(FigureCleanup, init_after_cleanup_then_reuse)
{
    RUN_ISOLATED(10, {
        blot_figure fig;
        ASSERT_TRUE(blot_figure_init(&fig, NULL));
        fill(&fig, MONTHS);
        EXPECT_EQ(rendered_labels(&fig), MONTHS);
        blot_figure_cleanup(&fig);

        ASSERT_TRUE(blot_figure_init(&fig, NULL));
        expect_empty(&fig);
        fill(&fig, DAYS);
        EXPECT_EQ(rendered_labels(&fig), DAYS);

        blot_figure_cleanup(&fig);
        blot_figure_cleanup(&fig);
        expect_empty(&fig);
    });
}
