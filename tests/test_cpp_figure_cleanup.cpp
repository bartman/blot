/* blot: Blot::Figure's destructor after an explicit blot_figure_cleanup()
 * or a move (#28; the moves are #20, the owned labels #23).
 *
 * A caller may clean a Blot::Figure up through the C API, on its c_ptr();
 * the destructor and the move-assign clean up again afterwards.
 * blot_figure_cleanup() leaves the figure empty, so that frees nothing
 * twice.  RUN_ISOLATED: a double free is an ASan report in the child,
 * or a glibc abort, or nothing, without ASan. */
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include "blot.hpp"
#include "blot_test.hpp"

namespace {

const std::vector<int32_t> XS = { 0, 8, 16, 24 };
const std::vector<int32_t> YS = { 6, 2, 9, 2 };

const std::vector<std::string> MONTHS = { "jan", "feb", "mar", "apr" };
const std::vector<std::string> DAYS = { "mon", "tue", "wed", "thu" };

void fill(Blot::Figure &fig, const std::vector<std::string> &names)
{
	fig.set_screen_size(41, 14);
	fig.set_x_limits(0, 31);
	fig.set_y_limits(0, 11);
	fig.bar(XS, YS, 1, nullptr);
	fig.scatter(XS, YS, 2, nullptr);
	std::vector<const char *> ptrs;
	for (const auto &s : names)
		ptrs.push_back(s.c_str());
	fig.set_x_axis_labels(ptrs);
}

void expect_empty(const Blot::Figure &fig)
{
	EXPECT_EQ(fig.c_ptr()->layers, nullptr);
	EXPECT_EQ(fig.c_ptr()->layer_count, 0u);
	EXPECT_EQ(fig.c_ptr()->xlabels.strings, nullptr);
	EXPECT_EQ(fig.c_ptr()->xlabels.count, 0u);
}

std::vector<std::string> rendered_labels(Blot::Figure &fig)
{
	blot_test::Rendered r = blot_test::render(fig.c_ptr(), blot_test::F);
	EXPECT_TRUE(r.ok) << r.message;
	return blot_test::labels_of(blot_test::x_ticks(r));
}

} // namespace

TEST(CppFigureCleanup, destructor_after_explicit_cleanup)
{
	RUN_ISOLATED(10, {
		Blot::Figure fig;
		fill(fig, MONTHS);
		blot_figure_cleanup(fig.c_ptr());
		expect_empty(fig);
		/* ~Figure() cleans up again */
	});
}

TEST(CppFigureCleanup, reuse_after_explicit_cleanup)
{
	RUN_ISOLATED(10, {
		Blot::Figure fig;
		fill(fig, MONTHS);
		blot_figure_cleanup(fig.c_ptr());
		fill(fig, DAYS);
		EXPECT_EQ(fig.c_ptr()->layer_count, 2u);
		EXPECT_EQ(rendered_labels(fig), DAYS);
	});
}

TEST(CppFigureCleanup, destructors_after_move_construct)
{
	RUN_ISOLATED(10, {
		Blot::Figure a;
		fill(a, MONTHS);
		{
			Blot::Figure b(std::move(a));
			expect_empty(a);
			EXPECT_EQ(rendered_labels(b), MONTHS);
			blot_figure_cleanup(b.c_ptr());
			/* ~b cleans up again */
		}
		blot_figure_cleanup(a.c_ptr());
		/* ~a cleans up again */
	});
}

TEST(CppFigureCleanup, move_assign_into_cleaned_up_figure)
{
	/* the move-assign cleans up its target, which was cleaned up already */
	RUN_ISOLATED(10, {
		Blot::Figure a, b;
		fill(a, MONTHS);
		fill(b, DAYS);
		blot_figure_cleanup(b.c_ptr());
		b = std::move(a);
		expect_empty(a);
		EXPECT_EQ(rendered_labels(b), MONTHS);
	});
}

TEST(CppFigureCleanup, move_assign_from_cleaned_up_figure)
{
	RUN_ISOLATED(10, {
		Blot::Figure a, b;
		fill(a, MONTHS);
		fill(b, DAYS);
		blot_figure_cleanup(a.c_ptr());
		b = std::move(a);
		expect_empty(a);
		expect_empty(b);
	});
}
