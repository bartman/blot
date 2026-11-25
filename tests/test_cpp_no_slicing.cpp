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
#include <type_traits>

#include "blot.hpp"

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

/* Dimensions stays a blot_dimensions, on purpose: it owns nothing, so a
 * sliced copy of it is a whole, independent copy of the size.  If
 * blot_dimensions ever gets more than its two unsigned (a pointer, say),
 * look again at #41 before changing this. */
static_assert(std::is_trivially_copyable_v<blot_dimensions>);
static_assert(sizeof(blot_dimensions) == 2 * sizeof(unsigned));
static_assert(std::is_convertible_v<Blot::Dimensions &, blot_dimensions &>);
