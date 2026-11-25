/* C++ wrapper ownership: the classes that own a C handle can't be
 * copied (a copy would free the handle twice), and moving hands the
 * handle over, leaving the source empty and safe to destroy (#20). */
#include <gtest/gtest.h>
#include <cerrno>
#include <cstdint>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "blot.hpp"

/* Screen owns a blot_screen, Figure owns its layers */
static_assert(!std::is_copy_constructible_v<Blot::Screen>, "Screen copy would double free");
static_assert(!std::is_copy_assignable_v<Blot::Screen>, "Screen copy would double free");
static_assert(std::is_nothrow_move_constructible_v<Blot::Screen>);
static_assert(std::is_nothrow_move_assignable_v<Blot::Screen>);

static_assert(!std::is_copy_constructible_v<Blot::Figure>, "Figure copy would double free");
static_assert(!std::is_copy_assignable_v<Blot::Figure>, "Figure copy would double free");
static_assert(std::is_nothrow_move_constructible_v<Blot::Figure>);
static_assert(std::is_nothrow_move_assignable_v<Blot::Figure>);

/* Exception owns a GError, but a thrown object must be copyable: its
 * copy is deep (g_error_copy), checked at run time below */
static_assert(std::is_copy_constructible_v<Blot::Exception>);
static_assert(std::is_copy_assignable_v<Blot::Exception>);
static_assert(std::is_nothrow_move_constructible_v<Blot::Exception>);
static_assert(std::is_nothrow_move_assignable_v<Blot::Exception>);

namespace {

const std::vector<int32_t> xs = {1, 2, 3, 4};
const std::vector<int32_t> ys = {4, 1, 3, 2};

Blot::Figure make_figure(unsigned layers)
{
	Blot::Figure fig;
	fig.set_screen_size(40, 12);
	for (unsigned i = 0; i < layers; i++)
		fig.scatter(xs, ys, 1, "data");
	return fig;
}

std::wstring text_of(Blot::Screen &scr)
{
	size_t size = 0;
	const wchar_t *txt = scr.get_text(size);
	return std::wstring(txt, size);
}

/* a self-move without -Wself-move noise */
template <typename T>
void self_move(T &obj)
{
	T &alias = obj;
	obj = std::move(alias);
}

Blot::Exception make_exception(int code)
{
	return Blot::Exception(g_error_new(G_UNIX_ERROR, code, "error %d", code));
}

} // namespace

// ------------------------------------------------------------------------

TEST(CppScreen, move_construct)
{
	Blot::Figure fig = make_figure(1);
	Blot::Screen a = fig.render(BLOT_RENDER_NO_COLOR);
	std::wstring want = text_of(a);
	ASSERT_FALSE(want.empty());

	Blot::Screen b(std::move(a));
	EXPECT_EQ(a.m_screen, nullptr);
	EXPECT_EQ(text_of(b), want);

	/* the moved-from screen has nothing to give */
	size_t size;
	EXPECT_THROW(a.get_text(size), Blot::Exception);
	/* both go out of scope here: one free, no double free */
}

TEST(CppScreen, move_assign)
{
	Blot::Figure fig = make_figure(1);
	Blot::Screen a = fig.render(BLOT_RENDER_NO_COLOR);
	Blot::Screen b = fig.render(BLOT_RENDER_NO_COLOR | BLOT_RENDER_BRAILLE);
	std::wstring want = text_of(a);

	/* b's own screen is freed (LeakSanitizer would see it otherwise) */
	b = std::move(a);
	EXPECT_EQ(a.m_screen, nullptr);
	EXPECT_EQ(text_of(b), want);

	/* assigning into the empty one works, and moving it back too */
	a = std::move(b);
	EXPECT_EQ(b.m_screen, nullptr);
	EXPECT_EQ(text_of(a), want);
}

TEST(CppScreen, move_assign_empty_to_empty)
{
	Blot::Screen a(nullptr);
	Blot::Screen b(nullptr);
	a = std::move(b);
	EXPECT_EQ(a.m_screen, nullptr);
	EXPECT_EQ(b.m_screen, nullptr);
}

TEST(CppScreen, self_move_assign)
{
	Blot::Figure fig = make_figure(1);
	Blot::Screen a = fig.render(BLOT_RENDER_NO_COLOR);
	std::wstring want = text_of(a);

	self_move(a);
	ASSERT_NE(a.m_screen, nullptr);
	EXPECT_EQ(text_of(a), want);
}

TEST(CppScreen, returned_by_value)
{
	auto render = [](Blot::Figure &fig) { return fig.render(BLOT_RENDER_NO_COLOR); };
	Blot::Figure fig = make_figure(1);
	std::vector<Blot::Screen> screens;
	for (int i = 0; i < 4; i++)
		screens.push_back(render(fig)); /* moves, and grows (moves again) */
	std::wstring want = text_of(screens[0]);
	for (auto &scr : screens)
		EXPECT_EQ(text_of(scr), want);
}

// ------------------------------------------------------------------------

TEST(CppFigure, move_construct)
{
	Blot::Figure a = make_figure(2);
	a.set_x_limits(0, 5);
	ASSERT_EQ(a.layer_count, 2u);
	blot_layer **layers = a.layers;

	Blot::Figure b(std::move(a));
	EXPECT_EQ(b.layer_count, 2u);
	EXPECT_EQ(b.layers, layers);
	EXPECT_TRUE(b.x_limits_set);
	EXPECT_EQ(b.lim.x_max, 5);

	/* the source is a fresh, empty figure */
	EXPECT_EQ(a.layer_count, 0u);
	EXPECT_EQ(a.layers, nullptr);
	EXPECT_FALSE(a.x_limits_set);
	EXPECT_FALSE(a.screen_dimensions_set);

	Blot::Screen scr = b.render(BLOT_RENDER_NO_COLOR);
	EXPECT_FALSE(text_of(scr).empty());

	/* and it stays usable */
	a.set_screen_size(40, 12);
	a.line(xs, ys, 2, "again");
	Blot::Screen scr2 = a.render(BLOT_RENDER_NO_COLOR);
	EXPECT_FALSE(text_of(scr2).empty());
}

TEST(CppFigure, move_assign)
{
	Blot::Figure a = make_figure(2);
	Blot::Figure b = make_figure(1);
	blot_layer **layers = a.layers;

	/* b's one layer is freed (LeakSanitizer would see it otherwise) */
	b = std::move(a);
	EXPECT_EQ(b.layer_count, 2u);
	EXPECT_EQ(b.layers, layers);
	EXPECT_EQ(a.layer_count, 0u);
	EXPECT_EQ(a.layers, nullptr);

	Blot::Screen scr = b.render(BLOT_RENDER_NO_COLOR);
	EXPECT_FALSE(text_of(scr).empty());
}

TEST(CppFigure, self_move_assign)
{
	Blot::Figure a = make_figure(2);
	blot_layer **layers = a.layers;

	self_move(a);
	EXPECT_EQ(a.layer_count, 2u);
	EXPECT_EQ(a.layers, layers);

	Blot::Screen scr = a.render(BLOT_RENDER_NO_COLOR);
	EXPECT_FALSE(text_of(scr).empty());
}

TEST(CppFigure, moved_from_destroyed_twice_over)
{
	/* a chain of moves: every moved-from figure is destroyed safely */
	std::vector<Blot::Figure> figs;
	for (int i = 0; i < 5; i++)
		figs.push_back(make_figure(1));
	for (auto &fig : figs)
		EXPECT_EQ(fig.layer_count, 1u);
}

// ------------------------------------------------------------------------

TEST(CppException, copy_is_deep)
{
	Blot::Exception a = make_exception(EINVAL);
	{
		Blot::Exception b(a);
		EXPECT_EQ(b.code(), EINVAL);
		EXPECT_EQ(b.str(), a.str());
		EXPECT_NE(b.what(), a.what()); /* its own GError */
	}
	EXPECT_EQ(a.code(), EINVAL);
	EXPECT_EQ(a.str(), "error 22");
}

TEST(CppException, copy_of_empty)
{
	Blot::Exception a(nullptr);
	Blot::Exception b(a);
	EXPECT_FALSE(b);
	EXPECT_EQ(b.code(), 0);
}

TEST(CppException, copy_assign_leaves_source)
{
	Blot::Exception a = make_exception(EINVAL);
	Blot::Exception b = make_exception(ERANGE);
	b = a; /* a copy: a keeps its error */
	EXPECT_EQ(a.code(), EINVAL);
	EXPECT_EQ(b.code(), EINVAL);
	EXPECT_NE(a.what(), b.what());

	Blot::Exception empty(nullptr);
	b = empty;
	EXPECT_FALSE(b);
	EXPECT_TRUE(a);
}

TEST(CppException, move)
{
	Blot::Exception a = make_exception(EINVAL);
	Blot::Exception b(std::move(a));
	EXPECT_FALSE(a);
	EXPECT_EQ(b.code(), EINVAL);

	Blot::Exception c = make_exception(ERANGE);
	c = std::move(b);
	EXPECT_FALSE(b);
	EXPECT_EQ(c.code(), EINVAL);

	self_move(c);
	EXPECT_EQ(c.code(), EINVAL);
}

TEST(CppException, throw_catch_and_copy)
{
	try {
		Blot::Figure fig;
		fig.set_screen_size(0, 0);
		FAIL() << "no throw";
	} catch (const Blot::Exception &thrown) {
		Blot::Exception e(thrown); /* a copy outlives the thrown one */
		EXPECT_EQ(e.code(), EINVAL);
		EXPECT_NE(e.what(), nullptr);
		EXPECT_NE(e.what(), thrown.what());
	}
}
