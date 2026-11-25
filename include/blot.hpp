/* blot C++ wrapper */
/* vim: set noet sw=8 ts=8 tw=120: */
#pragma once
#include "blot.h"

#include <cassert>
#include <iostream>
#include <span>
#include <stdexcept>
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

/* owns its layers and its copy of the X labels: move-only, a moved-from
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

	Figure(Figure &&other) noexcept : blot_figure(other) {
		other.reset();
	}

	Figure& operator=(Figure &&other) noexcept {
		if (this != &other) {
			blot_figure_cleanup(this);
			static_cast<blot_figure&>(*this) = other;
			other.reset();
		}
		return *this;
	}

private:
	/* forget the (moved) layers and labels, back to what blot_figure_init()
	 * gives */
	void reset() noexcept {
		[[maybe_unused]] bool ok = blot_figure_init(this, nullptr);
		assert(ok);
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

	/* add layers: the figure keeps pointers to the data and the label,
	 * not copies (see blot_figure_plot()); they must outlive render().
	 * A temporary vector or string passed here dangles as soon as the
	 * statement ends (#24). */

	template <typename T, typename U>
	void plot(blot_plot_type plot_type, const std::span<T> &data_xs, const std::span<U> &data_ys, blot_color data_color, const char *data_label) {
		GError *error = nullptr;
		size_t data_count = data_ys.size();

		BLOT_EXPECT(!data_ys.empty());
		const void *ptr_ys = (const void*)data_ys.data();

		const void *ptr_xs = nullptr;
		if (data_xs.size()) {
			BLOT_EXPECT_EQ(data_xs.size(), data_ys.size());
			ptr_xs = (const void*)data_xs.data();
		}

		blot_data_type x_data_type = data_type<T>();
		blot_data_type y_data_type = data_type<U>();
		blot_data_type data_type = BLOT_DATA_TYPE(x_data_type, y_data_type);

		if (!blot_figure_plot(this, plot_type, data_type, data_count, ptr_xs, ptr_ys, data_color, data_label, &error)) {
			throw_error(error, "blot_figure_plot");
		}
	}

	template <typename T, typename U>
	void plot(blot_plot_type plot_type, const std::vector<T> &data_xs, const std::vector<U> &data_ys, blot_color data_color, const char *data_label) {
		plot(plot_type,
		       std::span<const T>(data_xs.data(), data_xs.size()),
		       std::span<const U>(data_ys.data(), data_ys.size()),
		       data_color, data_label);
	}

	template <typename T, typename U>
	void scatter(const std::vector<T> &data_xs, const std::vector<U>& data_ys, blot_color data_color, const char *data_label) {
		plot(BLOT_SCATTER, data_xs, data_ys, data_color, data_label);
	}

	template <typename T>
	void scatter(const std::vector<T> &data_ys, blot_color data_color, const char *data_label) {
		plot<T,T>(BLOT_SCATTER, {}, data_ys, data_color, data_label);
	}

	template <typename T, typename U>
	void line(const std::vector<T> &data_xs, const std::vector<U>& data_ys, blot_color data_color, const char *data_label) {
		plot(BLOT_LINE, data_xs, data_ys, data_color, data_label);
	}

	template <typename T>
	void line(const std::vector<T>& data_ys, blot_color data_color, const char *data_label) {
		plot<T,T>(BLOT_LINE, {}, data_ys, data_color, data_label);
	}

	template <typename T, typename U>
	void bar(const std::vector<T> &data_xs, const std::vector<U>& data_ys, blot_color data_color, const char *data_label) {
		plot(BLOT_BAR, data_xs, data_ys, data_color, data_label);
	}

	template <typename T>
	void bar(const std::vector<T>& data_ys, blot_color data_color, const char *data_label) {
		plot<T,T>(BLOT_BAR, {}, data_ys, data_color, data_label);
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
