/* blot C++ wrapper */
/* vim: set noet sw=8 ts=8 tw=120: */
#pragma once
#include "blot.h"

#include <cassert>
#include <iostream>
#include <memory>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <algorithm>
#include <utility>
#include <vector>

namespace Blot {

/* the C code already allocated a "GError", this class just
 * carries it through the exception mechanism, then cleans up.
 *
 * An Exception can be empty, without a GError: after a move, or when made
 * from nullptr.  It still has a message, unknown_error, so what() and
 * str() never give nullptr or an empty text (#27); its code() is 0. */
class Exception final : public std::exception {
protected:
	GError *m_error;
public:
	explicit Exception(GError *error) : m_error(error) {}
	~Exception() { g_clear_error(&m_error); }

	/* a thrown object must be copyable: the copy gets its own GError */
	Exception(const Exception &other)
		: std::exception(other),
		  m_error(other.m_error ? g_error_copy(other.m_error) : nullptr) {}
	Exception(Exception &&other) noexcept
		: std::exception(other),
		  m_error(std::exchange(other.m_error, nullptr)) {}

	Exception& operator=(const Exception &other) {
		if (this != &other) {
			g_clear_error(&m_error);
			m_error = other.m_error ? g_error_copy(other.m_error) : nullptr;
		}
		return *this;
	}

	Exception& operator=(Exception &&other) noexcept {
		if (this != &other) {
			g_clear_error(&m_error);
			m_error = std::exchange(other.m_error, nullptr);
		}
		return *this;
	}

	static constexpr const char *unknown_error = "blot: unknown error";

	/* true when it carries a GError */
	explicit operator bool() const noexcept { return m_error != nullptr; }
	int code() const noexcept { return m_error ? m_error->code : 0; }
	std::string str() const { return what(); }

	const char * what() const noexcept override {
		if (m_error && m_error->message)
			return m_error->message;
		return unknown_error;
	}
};

/* throw the error a C call gave; one that failed without setting an error
 * still throws an Exception that names it, with code 0 */
[[noreturn]] inline void throw_error(GError *error, const char *call)
{
	if (!error)
		error = g_error_new(G_UNIX_ERROR, 0, "blot: %s() failed without an error", call);
	throw Exception(error);
}


#if (defined(DEBUG) && DEBUG) || (defined(NDEBUG) && !NDEBUG)
#define BLOT_THROW(code,format,args...) ({ \
	GError *error = g_error_new(G_UNIX_ERROR, code, "%s:%u:%s: " format, \
			     basename(__FILE__), __LINE__, __func__, ##args); \
	std::cerr << error->message << std::endl; \
	throw Blot::Exception(error); \
})
#else
#define BLOT_THROW(code,format,args...) \
	throw Blot::Exception(g_error_new(G_UNIX_ERROR, code, "%s:%u:%s: " format, \
			     basename(__FILE__), __LINE__, __func__, ##args))
#endif

#define BLOT_EXPECT(condition) ({ \
	if (!(condition)) { \
		BLOT_THROW(EINVAL, "assertion '%s' failed", __stringify(condition)); \
	} \
})

#define BLOT_EXPECT_EQ(a,b) ({ \
	if ((a) != (b)) { \
		BLOT_THROW(EINVAL, "expected %s == %s, but %s != %s", \
		     __stringify(a), __stringify(b), \
		     std::to_string(a).c_str(), std::to_string(b).c_str()); \
	} \
})

// ------------------------------------------------------------------------

template <typename T>
constexpr blot_data_type data_type() {
	if constexpr (std::is_same_v<std::remove_const_t<T>, int16_t>)
		return BLOT_DATA_INT16;
	else if constexpr (std::is_same_v<std::remove_const_t<T>, int32_t>)
		return BLOT_DATA_INT32;
	else if constexpr (std::is_same_v<std::remove_const_t<T>, int64_t>)
		return BLOT_DATA_INT64;
	else if constexpr (std::is_same_v<std::remove_const_t<T>, float>)
		return BLOT_DATA_FLOAT;
	else if constexpr (std::is_same_v<std::remove_const_t<T>, double>)
		return BLOT_DATA_DOUBLE;
	else
		BLOT_THROW(EINVAL, "unsupported type");
}

// ------------------------------------------------------------------------

struct Dimensions final : public blot_dimensions {
public:
	explicit Dimensions() {
		GError *error = nullptr;
		if (!blot_terminal_get_size(this, &error))
			throw_error(error, "blot_terminal_get_size");
	}
};

// ------------------------------------------------------------------------

/* owns the blot_screen: move-only, a moved-from Screen is empty */
struct Screen final {
	blot_screen *m_screen;
public:
	explicit Screen(blot_screen *screen) : m_screen(screen) { }
	~Screen() {
		if (m_screen)
			blot_screen_delete(m_screen);
	}

	Screen(const Screen &) = delete;
	Screen& operator=(const Screen &) = delete;

	Screen(Screen &&other) noexcept
		: m_screen(std::exchange(other.m_screen, nullptr)) { }

	Screen& operator=(Screen &&other) noexcept {
		if (this != &other) {
			if (m_screen)
				blot_screen_delete(m_screen);
			m_screen = std::exchange(other.m_screen, nullptr);
		}
		return *this;
	}

	const wchar_t *get_text(size_t &size) {
		GError *error = nullptr;
		gsize tmp_size;
		auto txt = blot_screen_get_text(m_screen, &tmp_size, &error);
		if (!txt)
			throw_error(error, "blot_screen_get_text");
		size = tmp_size;
		return txt;
	}
};

// ------------------------------------------------------------------------

/* a layer's label, as the Figure keeps it: its own copy of the text.  It is
 * made from a C string (NULL is no label, as in blot_figure_plot()), a
 * std::string or a std::string_view, temporary or not. */
class Label final {
	std::string m_text;
public:
	Label(const char *text) : m_text(text ? text : "") {}
	Label(std::string text) : m_text(std::move(text)) {}
	Label(std::string_view text) : m_text(text) {}

	std::string release() && { return std::move(m_text); }
};

/* owns its layers, its copy of the X labels, and the data and label of each
 * layer added from a std::vector (see plot()): move-only, a moved-from
 * Figure is a fresh, empty one */
struct Figure final : public blot_figure {
public:
	explicit Figure() {
		GError *error = nullptr;

		if (!blot_figure_init(this, &error)) [[unlikely]] {
			throw_error(error, "blot_figure_init");
		}
	}
	~Figure() {
		blot_figure_cleanup(this);
	}

	Figure(const Figure &) = delete;
	Figure& operator=(const Figure &) = delete;

	/* the layers point into m_owned's buffers, which a move hands over
	 * without moving them */
	Figure(Figure &&other) noexcept
		: blot_figure(other), m_owned(std::move(other.m_owned)) {
		other.reset();
	}

	Figure& operator=(Figure &&other) noexcept {
		if (this != &other) {
			/* the layers go before the data they point to */
			blot_figure_cleanup(this);
			static_cast<blot_figure&>(*this) = other;
			m_owned = std::move(other.m_owned);
			other.reset();
		}
		return *this;
	}

private:
	/* what the layers point to: a vector<T> per data array and a string
	 * per label, each in its own heap block, so that its address doesn't
	 * change when m_owned grows or the Figure moves (a short std::string
	 * keeps its text inside the object, which a move would change) */
	std::vector<std::shared_ptr<const void>> m_owned;

	/* forget the (moved) layers, labels and data, back to what
	 * blot_figure_init() gives */
	void reset() noexcept {
		[[maybe_unused]] bool ok = blot_figure_init(this, nullptr);
		assert(ok);
		m_owned.clear();
	}

	/* the copy of a vector the Figure keeps; an empty one is no data */
	template <typename T>
	static std::shared_ptr<const std::vector<T>> own(std::vector<T> &&data) {
		if (data.empty())
			return nullptr;
		return std::make_shared<const std::vector<T>>(std::move(data));
	}

	/* add a layer: count, types and pointers are checked here; keep_*
	 * hold what the pointers point into (null for borrowed data), and are
	 * kept only when the layer is added */
	template <typename T, typename U>
	void add_layer(blot_plot_type plot_type,
		       const T *xs, size_t xs_count, const U *ys, size_t ys_count,
		       blot_color data_color, Label &&data_label,
		       std::shared_ptr<const void> keep_xs, std::shared_ptr<const void> keep_ys) {
		GError *error = nullptr;

		BLOT_EXPECT(ys_count != 0);
		if (xs_count)
			BLOT_EXPECT_EQ(xs_count, ys_count);
		else
			xs = nullptr;

		blot_data_type x_data_type = data_type<T>();
		blot_data_type y_data_type = data_type<U>();
		blot_data_type data_type = BLOT_DATA_TYPE(x_data_type, y_data_type);

		auto label = std::make_shared<const std::string>(std::move(data_label).release());

		/* room for all three first: nothing can throw once the layer
		 * points at them */
		m_owned.reserve(m_owned.size() + 3);

		if (!blot_figure_plot(this, plot_type, data_type, ys_count, xs, ys,
				      data_color, label->c_str(), &error)) {
			throw_error(error, "blot_figure_plot");
		}

		m_owned.push_back(std::move(label));
		if (keep_xs)
			m_owned.push_back(std::move(keep_xs));
		if (keep_ys)
			m_owned.push_back(std::move(keep_ys));
	}

public:

	/* configure */

	void set_axis_color(blot_color color) {
		GError *error = nullptr;
		if (!blot_figure_set_axis_color(this, color, &error))
			throw_error(error, "blot_figure_set_axis_color");
	}

	void set_screen_size(unsigned cols, unsigned rows) {
		GError *error = nullptr;
		if (!blot_figure_set_screen_size(this, cols, rows, &error)) {
			throw_error(error, "blot_figure_set_screen_size");
		}
	}

	void set_x_limits(double x_min, double x_max) {
		GError *error = nullptr;
		if (!blot_figure_set_x_limits(this, x_min, x_max, &error))
			throw_error(error, "blot_figure_set_x_limits");
	}
	void set_y_limits(double y_min, double y_max) {
		GError *error = nullptr;
		if (!blot_figure_set_y_limits(this, y_min, y_max, &error))
			throw_error(error, "blot_figure_set_y_limits");
	}

	void set_x_axis_labels(const std::vector<const char *> &labels) {
		GError *error = nullptr;
		BLOT_EXPECT(!labels.empty());
		size_t label_count = labels.size();
		/* blot copies the array and the strings, and writes to
		 * neither: the vector may be a temporary (#23) */
		char **x_labels = const_cast<char **>(labels.data());
		if (!blot_figure_set_x_axis_labels(this, label_count, x_labels, &error))
			throw_error(error, "blot_figure_set_x_axis_labels");
	}

	/* add layers.  blot_figure_plot() keeps pointers, not copies (see
	 * blot_figure.h); the Figure gives it pointers into copies of its own
	 * (#24), moved along with the Figure:
	 *
	 * - the label is always copied;
	 * - a std::vector is taken by value: an lvalue is copied, an rvalue
	 *   (a temporary, or std::move(v)) is moved in without a copy.  Either
	 *   way the vector may die or change right after the call;
	 * - a std::span is a view, and is borrowed: its data must stay valid,
	 *   and unchanged, until the last render() (the cli plots its buffers
	 *   this way, without copying them).
	 *
	 * An empty xs means no X data: X is the index of each Y. */

	template <typename T, typename U>
	void plot(blot_plot_type plot_type, const std::span<T> &data_xs, const std::span<U> &data_ys, blot_color data_color, Label data_label) {
		add_layer(plot_type, data_xs.data(), data_xs.size(), data_ys.data(), data_ys.size(),
			  data_color, std::move(data_label), nullptr, nullptr);
	}

	template <typename T, typename U>
	void plot(blot_plot_type plot_type, std::vector<T> data_xs, std::vector<U> data_ys, blot_color data_color, Label data_label) {
		auto xs = own(std::move(data_xs));
		auto ys = own(std::move(data_ys));
		/* before the shared_ptrs are moved into the arguments */
		const T *xs_data = xs ? xs->data() : nullptr;
		const U *ys_data = ys ? ys->data() : nullptr;
		size_t xs_count = xs ? xs->size() : 0;
		size_t ys_count = ys ? ys->size() : 0;
		add_layer(plot_type, xs_data, xs_count, ys_data, ys_count,
			  data_color, std::move(data_label), std::move(xs), std::move(ys));
	}

	template <typename T, typename U>
	void scatter(std::vector<T> data_xs, std::vector<U> data_ys, blot_color data_color, Label data_label) {
		plot(BLOT_SCATTER, std::move(data_xs), std::move(data_ys), data_color, std::move(data_label));
	}

	template <typename T>
	void scatter(std::vector<T> data_ys, blot_color data_color, Label data_label) {
		plot(BLOT_SCATTER, std::vector<T>(), std::move(data_ys), data_color, std::move(data_label));
	}

	template <typename T, typename U>
	void line(std::vector<T> data_xs, std::vector<U> data_ys, blot_color data_color, Label data_label) {
		plot(BLOT_LINE, std::move(data_xs), std::move(data_ys), data_color, std::move(data_label));
	}

	template <typename T>
	void line(std::vector<T> data_ys, blot_color data_color, Label data_label) {
		plot(BLOT_LINE, std::vector<T>(), std::move(data_ys), data_color, std::move(data_label));
	}

	template <typename T, typename U>
	void bar(std::vector<T> data_xs, std::vector<U> data_ys, blot_color data_color, Label data_label) {
		plot(BLOT_BAR, std::move(data_xs), std::move(data_ys), data_color, std::move(data_label));
	}

	template <typename T>
	void bar(std::vector<T> data_ys, blot_color data_color, Label data_label) {
		plot(BLOT_BAR, std::vector<T>(), std::move(data_ys), data_color, std::move(data_label));
	}

	/* render */

	Screen render(blot_render_flags flags) {
		GError *error = nullptr;
		blot_screen * screen = blot_figure_render(this, flags, &error);
		if (!screen)
			throw_error(error, "blot_figure_render");
		return Screen(screen);
	}

};

};
