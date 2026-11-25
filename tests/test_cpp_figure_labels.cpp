/* blot: Blot::Figure::set_x_axis_labels() from vectors and strings that
 * die right after the call, and figures with labels that are moved
 * (#23, the moves are #20).
 *
 * The figure owns a copy of the labels (blot_figure_set_x_axis_labels
 * copies them), and a move hands that copy over.  The tests that would
 * read freed memory if the figure kept the caller's pointers run in
 * RUN_ISOLATED (see tests/test_c_figure_labels.cpp). */
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include "blot.hpp"
#include "blot_test.hpp"

namespace {

const blot_render_flags F = blot_test::F;

/* four bars, one label under the left edge of each */
const std::vector<int32_t> XS = { 0, 8, 16, 24 };
const std::vector<int32_t> YS = { 6, 2, 9, 2 };

const std::vector<std::string> MONTHS = { "jan", "feb", "mar", "apr" };
const std::vector<std::string> DAYS = { "mon", "tue", "wed", "thu" };

void plot_bars(Blot::Figure &fig)
{
	fig.set_screen_size(41, 14);
	fig.set_x_limits(0, 31);
	fig.set_y_limits(0, 11);
	fig.bar(XS, YS, 1, nullptr);
}

std::vector<std::string> rendered_labels(Blot::Figure &fig)
{
	blot_test::Rendered r = blot_test::render(fig.c_ptr(), F);
	EXPECT_TRUE(r.ok) << r.message;
	return blot_test::labels_of(blot_test::x_ticks(r));
}

/* the strings and the vector of pointers are gone when this returns,
 * and were overwritten before that */
void set_from_temporaries(Blot::Figure &fig, const std::vector<std::string> &names)
{
	std::vector<std::string> strs(names);
	std::vector<const char *> ptrs;
	for (const auto &s : strs)
		ptrs.push_back(s.c_str());
	fig.set_x_axis_labels(ptrs);
	for (auto &s : strs)
		s.assign(s.size(), 'X');
	ptrs.assign(ptrs.size(), "YYY");
}

/* a self-move without -Wself-move noise */
template <typename T>
void self_move(T &obj)
{
	T &alias = obj;
	obj = std::move(alias);
}

} // namespace

TEST(CppFigureLabels, temporary_vector)
{
	RUN_ISOLATED(10, {
		Blot::Figure fig;
		plot_bars(fig);
		/* the vector is a temporary, freed at the end of the statement */
		fig.set_x_axis_labels(std::vector<const char *>{ "jan", "feb", "mar", "apr" });
		EXPECT_EQ(rendered_labels(fig), MONTHS);
	});
}

TEST(CppFigureLabels, temporary_strings)
{
	RUN_ISOLATED(10, {
		Blot::Figure fig;
		plot_bars(fig);
		set_from_temporaries(fig, MONTHS);
		EXPECT_EQ(rendered_labels(fig), MONTHS);
	});
}

TEST(CppFigureLabels, replace_twice)
{
	RUN_ISOLATED(10, {
		Blot::Figure fig;
		plot_bars(fig);
		set_from_temporaries(fig, { "a", "b", "c", "d" });
		EXPECT_EQ(rendered_labels(fig), std::vector<std::string>({ "a", "b", "c", "d" }));
		set_from_temporaries(fig, DAYS);
		EXPECT_EQ(rendered_labels(fig), DAYS);
		set_from_temporaries(fig, MONTHS);
		EXPECT_EQ(rendered_labels(fig), MONTHS);
	});
}

TEST(CppFigureLabels, move_construct)
{
	RUN_ISOLATED(10, {
		Blot::Figure a;
		plot_bars(a);
		set_from_temporaries(a, MONTHS);
		char **strings = a.c_ptr()->xlabels.strings;

		/* the copy moves with the layers, the source has none */
		Blot::Figure b(std::move(a));
		EXPECT_EQ(b.c_ptr()->xlabels.count, 4u);
		EXPECT_EQ(b.c_ptr()->xlabels.strings, strings);
		EXPECT_EQ(a.c_ptr()->xlabels.count, 0u);
		EXPECT_EQ(a.c_ptr()->xlabels.strings, nullptr);
		EXPECT_EQ(rendered_labels(b), MONTHS);

		/* the moved-from figure gets labels of its own */
		plot_bars(a);
		set_from_temporaries(a, DAYS);
		EXPECT_EQ(rendered_labels(a), DAYS);
		EXPECT_EQ(rendered_labels(b), MONTHS);
	});
}

TEST(CppFigureLabels, move_assign)
{
	RUN_ISOLATED(10, {
		Blot::Figure a;
		plot_bars(a);
		set_from_temporaries(a, MONTHS);
		char **strings = a.c_ptr()->xlabels.strings;

		/* b's own labels are freed (LeakSanitizer checks the child) */
		Blot::Figure b;
		plot_bars(b);
		set_from_temporaries(b, DAYS);
		b = std::move(a);
		EXPECT_EQ(b.c_ptr()->xlabels.strings, strings);
		EXPECT_EQ(a.c_ptr()->xlabels.count, 0u);
		EXPECT_EQ(a.c_ptr()->xlabels.strings, nullptr);
		EXPECT_EQ(rendered_labels(b), MONTHS);

		self_move(b);
		EXPECT_EQ(b.c_ptr()->xlabels.strings, strings);
		EXPECT_EQ(rendered_labels(b), MONTHS);
	});
}

TEST(CppFigureLabels, figures_in_a_growing_vector)
{
	/* each push_back may move all of them: every label copy is moved,
	 * freed once, by the figure that has it last */
	RUN_ISOLATED(10, {
		std::vector<Blot::Figure> figs;
		for (int i = 0; i < 6; i++) {
			Blot::Figure fig;
			plot_bars(fig);
			set_from_temporaries(fig, i % 2 ? DAYS : MONTHS);
			figs.push_back(std::move(fig));
		}
		for (size_t i = 0; i < figs.size(); i++)
			EXPECT_EQ(rendered_labels(figs[i]), i % 2 ? DAYS : MONTHS) << "figure " << i;
	});
}
