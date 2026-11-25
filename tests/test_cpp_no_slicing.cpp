/* blot: a Blot::Figure can't be sliced into its blot_figure (#41).
 *
 * The blot_figure of a Figure points to the layers and X labels the
 * Figure owns.  When the Figure was a blot_figure (public base), it
 * converted to a blot_figure& or blot_figure* without a word, and
 * "blot_figure raw = fig;" copied the struct out of it: a second
 * blot_figure with the same pointers, which a blot_figure_cleanup(&raw)
 * frees under the Figure's feet (a double free when the Figure dies).
 * Making the Figure non-copyable (#20) didn't stop that: the copy is of
 * the base.  These checks are at compile time: the file doesn't build
 * while any of it is possible. */
#include <gtest/gtest.h>
#include <cstdint>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "blot.hpp"
#include "blot_test.hpp"

/* no conversion to the C struct, by reference or by pointer */
static_assert(!std::is_convertible_v<Blot::Figure &, blot_figure &>,
	      "a Figure must not bind to a blot_figure& (#41)");
static_assert(!std::is_convertible_v<const Blot::Figure &, const blot_figure &>,
	      "a Figure must not bind to a const blot_figure& (#41)");
static_assert(!std::is_convertible_v<Blot::Figure *, blot_figure *>,
	      "a Figure* must not convert to a blot_figure* (#41)");
static_assert(!std::is_convertible_v<const Blot::Figure *, const blot_figure *>,
	      "a Figure* must not convert to a const blot_figure* (#41)");

/* the struct can't be copied out of a Figure: blot_figure raw = fig;
 * blot_figure raw(fig); raw = fig; nor from a temporary or a moved one */
static_assert(!std::is_convertible_v<Blot::Figure &, blot_figure>,
	      "blot_figure raw = fig; must not compile (#41)");
static_assert(!std::is_constructible_v<blot_figure, Blot::Figure &>,
	      "blot_figure raw(fig); must not compile (#41)");
static_assert(!std::is_constructible_v<blot_figure, const Blot::Figure &>,
	      "blot_figure raw(cfig); must not compile (#41)");
static_assert(!std::is_constructible_v<blot_figure, Blot::Figure &&>,
	      "blot_figure raw(std::move(fig)); must not compile (#41)");
static_assert(!std::is_assignable_v<blot_figure &, Blot::Figure &>,
	      "raw = fig; must not compile (#41)");
static_assert(!std::is_assignable_v<blot_figure &, const Blot::Figure &>,
	      "raw = cfig; must not compile (#41)");
static_assert(!std::is_assignable_v<blot_figure &, Blot::Figure &&>,
	      "raw = std::move(fig); must not compile (#41)");

/* the C API gets the figure from c_ptr(), explicitly */
static_assert(std::is_same_v<decltype(std::declval<Blot::Figure &>().c_ptr()), blot_figure *>);
static_assert(std::is_same_v<decltype(std::declval<const Blot::Figure &>().c_ptr()), const blot_figure *>);
static_assert(noexcept(std::declval<Blot::Figure &>().c_ptr()));

/* Dimensions stays a blot_dimensions, on purpose: it owns nothing, so a
 * sliced copy of it is a whole, independent copy of the size.  If
 * blot_dimensions ever gets more than its two unsigned (a pointer, say),
 * look again at #41 before changing this. */
static_assert(std::is_trivially_copyable_v<blot_dimensions>);
static_assert(sizeof(blot_dimensions) == 2 * sizeof(unsigned));
static_assert(std::is_convertible_v<Blot::Dimensions &, blot_dimensions &>);

namespace {

/* with a legend, to see the label of each layer */
const blot_render_flags F = blot_test::F | BLOT_RENDER_LEGEND_ABOVE;

/* C data: the C API keeps the pointers, so they outlive every render */
const int32_t C_XS[] = { 0, 10, 20, 30 };
const int32_t C_YS[] = { 1, 7, 3, 9 };
const std::vector<int32_t> XS = { 0, 10, 20, 30 };
const std::vector<int32_t> YS = { 9, 3, 7, 1 };

std::wstring cpp_text(Blot::Figure &fig)
{
	Blot::Screen scr = fig.render(F);
	size_t size = 0;
	const wchar_t *txt = scr.get_text(size);
	return std::wstring(txt, size);
}

} // namespace

/* c_ptr() is the Figure's own blot_figure: what the C++ API set shows
 * there, what the C API sets shows in the Figure, and the two render the
 * same */
TEST(CppFigureCPtr, c_api_on_c_ptr)
{
	Blot::Figure fig;
	const Blot::Figure &cfig = fig;
	ASSERT_NE(fig.c_ptr(), nullptr);
	EXPECT_EQ(fig.c_ptr(), cfig.c_ptr());

	/* C++ in, C out */
	fig.set_screen_size(60, 24);
	fig.set_x_limits(0, 30);
	fig.scatter(XS, YS, 1, "from-cpp");
	EXPECT_EQ(cfig.c_ptr()->dim.cols, 60u);
	EXPECT_EQ(cfig.c_ptr()->dim.rows, 24u);
	EXPECT_TRUE(cfig.c_ptr()->x_limits_set);
	EXPECT_EQ(cfig.c_ptr()->layer_count, 1u);

	/* C in, C++ out */
	GError *error = nullptr;
	ASSERT_TRUE(blot_figure_set_y_limits(fig.c_ptr(), 0, 10, &error));
	ASSERT_TRUE(blot_figure_line(fig.c_ptr(), BLOT_DATA_INT32, 4, C_XS, C_YS,
				     2, "from-c", &error));
	ASSERT_EQ(error, nullptr);
	EXPECT_EQ(cfig.c_ptr()->layer_count, 2u);

	blot_test::Rendered c = blot_test::render(fig.c_ptr(), F);
	ASSERT_TRUE(c.ok) << c.message;
	std::wstring cpp = cpp_text(fig);
	EXPECT_EQ(c.text, cpp);
	/* both layers are there: the legend names them */
	EXPECT_NE(cpp.find(L"from-cpp"), std::wstring::npos);
	EXPECT_NE(cpp.find(L"from-c\n"), std::wstring::npos);
}

/* the C API's cleanup on c_ptr() empties the Figure, which then cleans
 * up again in its destructor: nothing is freed twice (#28), and the
 * Figure can be filled again */
TEST(CppFigureCPtr, cleanup_through_c_ptr)
{
	RUN_ISOLATED(10, {
		Blot::Figure fig;
		fig.set_screen_size(40, 12);
		fig.scatter(XS, YS, 1, "first");
		blot_figure_cleanup(fig.c_ptr());
		EXPECT_EQ(fig.c_ptr()->layer_count, 0u);
		EXPECT_EQ(fig.c_ptr()->layers, nullptr);
		fig.scatter(XS, YS, 1, "second");
		EXPECT_EQ(fig.c_ptr()->layer_count, 1u);
		/* ~Figure() cleans up again */
	});
}

/* a move copies the struct into the target's own blot_figure: each
 * Figure keeps its c_ptr(), the target's has the layers, the source's is
 * empty */
TEST(CppFigureCPtr, move_keeps_each_c_ptr)
{
	Blot::Figure a;
	a.set_screen_size(40, 12);
	a.scatter(XS, YS, 1, "data");
	blot_figure *pa = a.c_ptr();

	Blot::Figure b(std::move(a));
	EXPECT_EQ(a.c_ptr(), pa);
	EXPECT_NE(b.c_ptr(), pa);
	EXPECT_EQ(a.c_ptr()->layer_count, 0u);
	EXPECT_EQ(b.c_ptr()->layer_count, 1u);

	Blot::Figure c;
	blot_figure *pc = c.c_ptr();
	c = std::move(b);
	EXPECT_EQ(c.c_ptr(), pc);
	EXPECT_EQ(b.c_ptr()->layer_count, 0u);
	EXPECT_EQ(c.c_ptr()->layer_count, 1u);

	blot_test::Rendered r = blot_test::render(c.c_ptr(), F);
	EXPECT_TRUE(r.ok) << r.message;
}
