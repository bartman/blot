/* blot: Blot::Figure layers whose data and label die, or change, right
 * after plot()/scatter()/line()/bar() (#24), and figures with such layers
 * that are moved (#20).
 *
 * blot_figure_plot() keeps the caller's pointers (see blot_figure.h).  The
 * C++ Figure gives it pointers into copies it owns: a vector passed by
 * value (moved in when it is an rvalue) and the label.  Only a std::span is
 * borrowed.  The tests that would read freed memory if the figure kept the
 * caller's pointers run in RUN_ISOLATED, as in test_cpp_figure_labels.cpp;
 * under ASan (ci/sanitize.sh) such a read is a heap-use-after-free. */
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

#include "blot.hpp"
#include "blot_test.hpp"

using blot_test::Cells;
using blot_test::Lines;
using blot_test::Rendered;

namespace {

const blot_render_flags F = blot_test::F;

/* FIX-A (blot_test.hpp): a 20x10 plot area, one cell per integer */
void fix_a(Blot::Figure &fig)
{
	fig.set_screen_size(28, 12);
	fig.set_x_limits(0, 19);
	fig.set_y_limits(0, 9);
}

/* the SC points of FIX-A, and where they land */
const std::vector<int> SC_X = { 0, 19, 10, 5 };
const std::vector<int> SC_Y = { 0,  9,  5, 2 };
const Cells SC_CELLS = { {0, 0}, {19, 9}, {10, 5}, {5, 2} };

/* ys only: x is the index 0..3 */
const Cells SC_Y_CELLS = { {0, 0}, {1, 9}, {2, 5}, {3, 2} };

template <typename T>
std::vector<T> as(const std::vector<int> &v)
{
	return std::vector<T>(v.begin(), v.end());
}

/* fill the heap with blocks of the sizes just freed, so that a layer that
 * points at freed memory reads these values, not stale ones */
void scribble()
{
	std::vector<std::vector<char>> junk;
	for (size_t n : { 8, 16, 24, 32, 48, 64 })
		for (int i = 0; i < 8; i++)
			junk.emplace_back(n, (char)0x5a);
	std::vector<std::string> strs;
	for (int i = 0; i < 16; i++)
		strs.emplace_back(32, 'Z');
}

Cells cells_of(Blot::Figure &fig)
{
	Rendered r = blot_test::render(&fig, F);
	EXPECT_TRUE(r.ok) << r.message;
	return blot_test::plot_cells(r);
}

/* the legend lines, without the colour; on a screen tall enough for a
 * legend, then back to the figure's own size */
Lines legend_of(Blot::Figure &fig)
{
	blot_dimensions dim = fig.dim;
	fig.set_screen_size(60, 24);
	Rendered r = blot_test::render(&fig, F | BLOT_RENDER_LEGEND_ABOVE);
	fig.set_screen_size(dim.cols, dim.rows);
	EXPECT_TRUE(r.ok) << r.message;
	Lines out;
	for (const auto &l : blot_test::strip_sgr(r.lines))
		if (l.find(L'\u25d8') != std::wstring::npos)	/* ◘ */
			out.push_back(blot_test::rtrim(l));
	return out;
}

std::wstring text_of(Blot::Figure &fig, blot_render_flags flags = F)
{
	Rendered r = blot_test::render(&fig, flags);
	EXPECT_TRUE(r.ok) << r.message;
	return r.text;
}

template <typename T>
void scatter_xy_from_temporaries()
{
	SCOPED_TRACE(Blot::data_type<T>());
	Blot::Figure fig;
	fix_a(fig);
	fig.scatter(as<T>(SC_X), as<T>(SC_Y), 1, std::string("pts").c_str());
	scribble();
	EXPECT_CELLS_EQ(cells_of(fig), SC_CELLS);
	EXPECT_EQ(fig.layers[0]->count, 4u);
}

template <typename T>
void scatter_y_from_temporary()
{
	Blot::Figure fig;
	fix_a(fig);
	fig.scatter(as<T>(SC_Y), 1, std::string("pts").c_str());
	scribble();
	EXPECT_CELLS_EQ(cells_of(fig), SC_Y_CELLS);
}

/* the same figure, from data that stays alive */
std::wstring reference(blot_plot_type type, bool with_xs)
{
	Blot::Figure fig;
	fix_a(fig);
	if (with_xs)
		fig.plot(type, std::span<const int>(SC_X), std::span<const int>(SC_Y), 3, "ref");
	else
		fig.plot(type, std::span<const int>(), std::span<const int>(SC_Y), 3, "ref");
	return text_of(fig, F | BLOT_RENDER_LEGEND_BELOW);
}

/* a templated entity, so that a call that doesn't compile is false rather
 * than an error */
template <typename Fig, typename Data, typename Label>
constexpr bool line_accepts = requires(Fig &fig, Data &&data, Label &&label) {
	fig.line(std::forward<Data>(data), 1, std::forward<Label>(label));
};
template <typename Fig, typename Data, typename Label>
constexpr bool line_xy_accepts = requires(Fig &fig, Data &&xs, Data &&ys, Label &&label) {
	fig.line(std::forward<Data>(xs), std::forward<Data>(ys), 1, std::forward<Label>(label));
};

} // namespace

/* --- data from temporaries ------------------------------------------ */

TEST(CppFigureLayers, scatter_xy_from_temporaries_each_type)
{
	RUN_ISOLATED(10, {
		scatter_xy_from_temporaries<int16_t>();
		scatter_xy_from_temporaries<int32_t>();
		scatter_xy_from_temporaries<int64_t>();
		scatter_xy_from_temporaries<float>();
		scatter_xy_from_temporaries<double>();
	});
}

TEST(CppFigureLayers, scatter_y_from_temporary_each_type)
{
	RUN_ISOLATED(10, {
		scatter_y_from_temporary<int16_t>();
		scatter_y_from_temporary<int32_t>();
		scatter_y_from_temporary<int64_t>();
		scatter_y_from_temporary<float>();
		scatter_y_from_temporary<double>();
	});
}

TEST(CppFigureLayers, mixed_types_from_temporaries)
{
	RUN_ISOLATED(10, {
		Blot::Figure fig;
		fix_a(fig);
		fig.scatter(as<int16_t>(SC_X), as<double>(SC_Y), 1, "pts");
		scribble();
		EXPECT_CELLS_EQ(cells_of(fig), SC_CELLS);
	});
}

TEST(CppFigureLayers, line_and_bar_from_temporaries)
{
	/* each must render exactly like the same layer over live data */
	RUN_ISOLATED(10, {
		for (auto type : { BLOT_LINE, BLOT_BAR, BLOT_SCATTER }) {
			SCOPED_TRACE(type);
			std::wstring xy = reference(type, true), y = reference(type, false);

			Blot::Figure a;
			fix_a(a);
			Blot::Figure b;
			fix_a(b);
			switch (type) {
			case BLOT_LINE:
				a.line(as<int>(SC_X), as<int>(SC_Y), 3, std::string("ref").c_str());
				b.line(as<int>(SC_Y), 3, std::string("ref").c_str());
				break;
			case BLOT_BAR:
				a.bar(as<int>(SC_X), as<int>(SC_Y), 3, std::string("ref").c_str());
				b.bar(as<int>(SC_Y), 3, std::string("ref").c_str());
				break;
			default:
				a.scatter(as<int>(SC_X), as<int>(SC_Y), 3, std::string("ref").c_str());
				b.scatter(as<int>(SC_Y), 3, std::string("ref").c_str());
				break;
			}
			scribble();
			EXPECT_EQ(blot_test::to_utf8(text_of(a, F | BLOT_RENDER_LEGEND_BELOW)), blot_test::to_utf8(xy));
			EXPECT_EQ(blot_test::to_utf8(text_of(b, F | BLOT_RENDER_LEGEND_BELOW)), blot_test::to_utf8(y));
		}
	});
}

TEST(CppFigureLayers, lvalue_destroyed_before_render)
{
	RUN_ISOLATED(10, {
		Blot::Figure fig;
		fix_a(fig);
		{
			std::vector<double> xs = as<double>(SC_X), ys = as<double>(SC_Y);
			std::string label = "a label longer than the small string buffer";
			fig.scatter(xs, ys, 1, label.c_str());
		}
		scribble();
		EXPECT_CELLS_EQ(cells_of(fig), SC_CELLS);
		EXPECT_EQ(legend_of(fig), Lines({ L"◘  a label longer than the small string buffer" }));
	});
}

TEST(CppFigureLayers, lvalue_changed_after_plot)
{
	/* the figure shows the data as it was when it was plotted */
	Blot::Figure fig;
	fix_a(fig);
	std::vector<int32_t> xs = as<int32_t>(SC_X), ys = as<int32_t>(SC_Y);
	char label[] = "pts";
	fig.scatter(xs, ys, 1, label);
	xs.assign(xs.size(), 7);
	ys.assign(ys.size(), 7);
	label[0] = 'X';
	EXPECT_CELLS_EQ(cells_of(fig), SC_CELLS);
	EXPECT_EQ(legend_of(fig), Lines({ L"◘  pts" }));
}

TEST(CppFigureLayers, rvalue_moved_in_lvalue_copied)
{
	/* std::move() hands the buffer over, without a copy; an lvalue is
	 * copied, and the caller keeps it */
	Blot::Figure fig;
	fix_a(fig);
	std::vector<int32_t> xs = as<int32_t>(SC_X), ys = as<int32_t>(SC_Y);
	const int32_t *ys_data = ys.data();
	fig.scatter(xs, std::move(ys), 1, "pts");
	EXPECT_NE(fig.layers[0]->xs, xs.data());
	EXPECT_EQ(fig.layers[0]->ys, ys_data);
	EXPECT_EQ(xs, as<int32_t>(SC_X));
	EXPECT_CELLS_EQ(cells_of(fig), SC_CELLS);
}

TEST(CppFigureLayers, failed_plot_adds_nothing)
{
	/* the copies made for a layer that isn't added are freed (LeakSanitizer
	 * checks), and the figure still renders its other layers */
	Blot::Figure fig;
	fix_a(fig);
	fig.scatter(as<int32_t>(SC_X), as<int32_t>(SC_Y), 1, "pts");
	EXPECT_THROW(fig.scatter(std::vector<int32_t>{ 1, 2 }, as<int32_t>(SC_Y), 2, "bad"), Blot::Exception);
	EXPECT_THROW(fig.line(std::vector<double>(), 3, "empty"), Blot::Exception);
	EXPECT_EQ(fig.layer_count, 1u);
	EXPECT_CELLS_EQ(cells_of(fig), SC_CELLS);
	EXPECT_EQ(legend_of(fig), Lines({ L"◘  pts" }));
}

TEST(CppFigureLayers, span_is_borrowed)
{
	/* a std::span is a view: the figure reads the caller's data at render
	 * time (the cli plots its buffers this way, without a copy) */
	Blot::Figure fig;
	fix_a(fig);
	std::vector<int32_t> xs = as<int32_t>(SC_X), ys = as<int32_t>(SC_Y);
	fig.plot(BLOT_SCATTER, std::span<const int32_t>(xs), std::span<const int32_t>(ys), 1, "pts");
	EXPECT_EQ(fig.layers[0]->xs, xs.data());
	EXPECT_EQ(fig.layers[0]->ys, ys.data());
	EXPECT_CELLS_EQ(cells_of(fig), SC_CELLS);
	ys[1] = 0;
	EXPECT_CELLS_EQ(cells_of(fig), Cells({ {0, 0}, {19, 0}, {10, 5}, {5, 2} }));
}

/* --- labels --------------------------------------------------------- */

TEST(CppFigureLayers, label_from_temporary_c_str)
{
	RUN_ISOLATED(10, {
		Blot::Figure fig;
		fix_a(fig);
		fig.scatter(SC_X, SC_Y, 1, std::string("a label longer than the small string buffer").c_str());
		fig.line(SC_X, SC_Y, 2, std::string("short").c_str());
		scribble();
		EXPECT_EQ(legend_of(fig), Lines({
			L"◘  a label longer than the small string buffer",
			L"◘  short",
		}));
	});
}

TEST(CppFigureLayers, label_of_span_layer_is_copied)
{
	RUN_ISOLATED(10, {
		Blot::Figure fig;
		fix_a(fig);
		fig.plot(BLOT_SCATTER, std::span<const int>(SC_X), std::span<const int>(SC_Y), 1,
			 std::string("a label longer than the small string buffer").c_str());
		scribble();
		EXPECT_EQ(legend_of(fig), Lines({ L"◘  a label longer than the small string buffer" }));
	});
}

TEST(CppFigureLayers, null_label)
{
	Blot::Figure fig;
	fix_a(fig);
	fig.scatter(SC_X, SC_Y, 1, nullptr);
	fig.scatter(SC_Y, 2, NULL);
	EXPECT_STREQ(fig.layers[0]->label, "");
	EXPECT_STREQ(fig.layers[1]->label, "");
	EXPECT_EQ(legend_of(fig), Lines());
}

/* --- moves ---------------------------------------------------------- */

TEST(CppFigureLayers, move_construct_keeps_data)
{
	RUN_ISOLATED(10, {
		Blot::Figure a;
		fix_a(a);
		a.scatter(as<double>(SC_X), as<double>(SC_Y), 1, std::string("pts").c_str());
		Blot::Figure b(std::move(a));
		scribble();
		EXPECT_EQ(a.layer_count, 0u);
		EXPECT_CELLS_EQ(cells_of(b), SC_CELLS);
		EXPECT_EQ(legend_of(b), Lines({ L"◘  pts" }));
	});
}

TEST(CppFigureLayers, move_assign_keeps_data)
{
	RUN_ISOLATED(10, {
		Blot::Figure a;
		fix_a(a);
		a.scatter(as<int64_t>(SC_X), as<int64_t>(SC_Y), 1, std::string("pts").c_str());
		/* b's own layer and data are freed (LeakSanitizer checks) */
		Blot::Figure b;
		fix_a(b);
		b.line(as<float>(SC_Y), 2, std::string("old").c_str());
		b = std::move(a);
		scribble();
		EXPECT_EQ(a.layer_count, 0u);
		EXPECT_CELLS_EQ(cells_of(b), SC_CELLS);
		EXPECT_EQ(legend_of(b), Lines({ L"◘  pts" }));

		/* the moved-from figure plots data of its own */
		fix_a(a);
		a.scatter(as<int16_t>(SC_Y), 1, std::string("ys").c_str());
		EXPECT_CELLS_EQ(cells_of(a), SC_Y_CELLS);
		EXPECT_CELLS_EQ(cells_of(b), SC_CELLS);
	});
}

TEST(CppFigureLayers, figures_in_a_growing_vector)
{
	RUN_ISOLATED(10, {
		std::vector<Blot::Figure> figs;
		for (int i = 0; i < 6; i++) {
			Blot::Figure fig;
			fix_a(fig);
			if (i % 2)
				fig.scatter(as<float>(SC_Y), 1, std::to_string(i).c_str());
			else
				fig.scatter(as<float>(SC_X), as<float>(SC_Y), 1, std::to_string(i).c_str());
			figs.push_back(std::move(fig));
			scribble();
		}
		for (size_t i = 0; i < figs.size(); i++) {
			SCOPED_TRACE(i);
			EXPECT_CELLS_EQ(cells_of(figs[i]), i % 2 ? SC_Y_CELLS : SC_CELLS);
			EXPECT_EQ(legend_of(figs[i]), Lines({ L"◘  " + std::to_wstring(i) }));
		}
	});
}

/* --- what compiles -------------------------------------------------- */

TEST(CppFigureLayers, overloads)
{
	using Fig = Blot::Figure;
	using V = std::vector<double>;

	/* data: lvalues, const lvalues and rvalues */
	EXPECT_TRUE((line_accepts<Fig, V &, const char *>));
	EXPECT_TRUE((line_accepts<Fig, const V &, const char *>));
	EXPECT_TRUE((line_accepts<Fig, V, const char *>));
	EXPECT_TRUE((line_xy_accepts<Fig, V, const char *>));

	/* labels: a C string, nullptr, and a std::string, temporary or not */
	EXPECT_TRUE((line_accepts<Fig, V, std::nullptr_t>));
	EXPECT_TRUE((line_accepts<Fig, V, std::string>));
	EXPECT_TRUE((line_accepts<Fig, V, const std::string &>));
	EXPECT_TRUE((line_xy_accepts<Fig, V, std::string>));
	EXPECT_TRUE((line_accepts<Fig, V, std::string_view>));

	/* not a label */
	EXPECT_FALSE((line_accepts<Fig, V, int *>));
}

/* a template, so that it compiles where a std::string label doesn't */
template <typename Fig>
void labels_from_strings()
{
	if constexpr (line_xy_accepts<Fig, std::vector<int>, std::string>) {
		Fig fig;
		fix_a(fig);
		fig.scatter(SC_X, SC_Y, 1, std::string("a label longer than the small string buffer"));
		std::string s = "short";
		fig.line(SC_Y, 2, s);
		s = "changed";
		fig.bar(SC_Y, 3, std::string_view("view"));
		scribble();
		EXPECT_EQ(legend_of(fig), Lines({
			L"◘  a label longer than the small string buffer",
			L"◘  short",
			L"◘  view",
		}));
	} else {
		ADD_FAILURE() << "a std::string label doesn't compile";
	}
}

TEST(CppFigureLayers, label_from_temporary_string)
{
	RUN_ISOLATED(10, {
		labels_from_strings<Blot::Figure>();
	});
}
