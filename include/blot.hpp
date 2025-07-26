/* blot C++ wrapper */
/* vim: set noet sw=8 ts=8 tw=120: */
#pragma once
#include "blot_canvas.h"
#include "blot_color.h"
#include "blot_error.h"
#include "blot_figure.h"
#include "blot_layer.h"
#include "blot_screen.h"
#include "blot_terminal.h"
#include "blot_time.h"
#include "blot_types.h"
#include "blot_utils.h"

#include <cassert>
#include <iostream>
#include <stdexcept>
#include <algorithm>
#include <utility>
#include <vector>

namespace Blot {

/* the C code already allocated a "GError", this class just
 * carries it throught the exception mechanism, then cleans up. */
class Exception final {
protected:
	GError *m_error;
public:
	explicit Exception(GError *error) : m_error(error) {}
	~Exception() { g_clear_error(&m_error); }

	Exception(const Exception &other) {
		if (other.m_error)
			m_error = g_error_copy(other.m_error);
	}
	Exception(Exception &&other) noexcept {
		m_error = std::exchange(other.m_error, nullptr);
	}

	Exception& operator=(const Exception &other) {
		if (this != &other) {
			g_clear_error(&m_error);
			m_error = g_error_copy(other.m_error);
		}
		return *this;
	}

	Exception& operator=(Exception &other) noexcept {
		if (this != &other) {
			g_clear_error(&m_error);
			m_error = std::exchange(other.m_error, nullptr);
		}
		return *this;
	}

	operator bool() const { return m_error != nullptr; }
	int code() const { return m_error ? m_error->code : 0; }
	std::string str() const { if (m_error) return m_error->message; return {}; }
	const char * c_str() const { return m_error ? m_error->message : nullptr; }
};


#if (defined(DEBUG) && DEBUG) || (defined(NDEBUG) && !NDEBUG)
#define BLOT_THROW(code,format,args...) ({ \
	GError *error = g_error_new(G_UNIX_ERROR, code, "%s:%u:%s: " format, \
			     basename(__FILE__), __LINE__, __func__, ##args); \
	std::cerr << error->message << std::endl; \
	throw Exception(error); \
})
#else
#define __BLOT_THROW(code,format,args...) \
	throw Exception(g_error_new(G_UNIX_ERROR, code, "%s:%u:%s: " format, \
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

struct Screen final {
	blot_screen *m_screen;
public:
	explicit Screen(blot_screen *screen) : m_screen(screen) { }
	~Screen() { blot_screen_delete(m_screen); }

	const wchar_t *get_text(size_t &size) {
		GError *error = nullptr;
		gsize tmp_size;
		auto txt = blot_screen_get_text(m_screen, &tmp_size, &error);
		if (!txt)
			throw Exception(error);
		size = tmp_size;
		return txt;
	}
};

// ------------------------------------------------------------------------

struct Figure final : public blot_figure {
public:
	explicit Figure() {
		GError *error = nullptr;

		if (!blot_figure_init(this, &error)) [[unlikely]] {
			throw Exception(error);
		}
	}
	~Figure() {
		blot_figure_cleanup(this);
	}

	/* configure */

	void set_axis_color(blot_color color) {
		GError *error = nullptr;
		if (!blot_figure_set_axis_color(this, color, &error))
			throw Exception(error);
	}

	void set_screen_size(unsigned cols, unsigned rows) {
		GError *error = nullptr;
		if (!blot_figure_set_screen_size(this, cols, rows, &error)) {
			throw Exception(error);
		}
	}

	void set_x_limits(double x_min, double x_max) {
		GError *error = nullptr;
		if (!blot_figure_set_x_limits(this, x_min, x_max, &error))
			throw Exception(error);
	}
	void set_y_limits(double y_min, double y_max) {
		GError *error = nullptr;
		if (!blot_figure_set_y_limits(this, y_min, y_max, &error))
			throw Exception(error);
	}

	void set_x_axis_labels(const std::vector<const char *> labels) {
		GError *error = nullptr;
		size_t label_count = labels.size();
		char **x_labels = (char**)labels.data(); // TOOD: fix this cast!
		if (!blot_figure_set_x_axis_labels(this, label_count, x_labels, &error))
			throw Exception(error);
	}

	/* add layers */

	template <typename T>
	void plot(blot_plot_type plot_type, const std::vector<T> &data_xs, const std::vector<T> data_ys, blot_color data_color, const char *data_label) {
		GError *error = nullptr;
		BLOT_EXPECT(data_ys.size());
		if (data_xs.size())
			BLOT_EXPECT_EQ(data_xs.size(), data_ys.size());
		size_t data_count = data_ys.size();

		blot_data_type data_type;
		const void *ptr_xs = (const void*)data_xs.data();
		const void *ptr_ys = (const void*)data_ys.data();

		if constexpr (std::is_same_v<T, int16_t>)
			data_type = BLOT_DATA_INT16;
		else if constexpr (std::is_same_v<T, int32_t>)
			data_type = BLOT_DATA_INT32;
		else if constexpr (std::is_same_v<T, int64_t>)
			data_type = BLOT_DATA_INT64;
		else if constexpr (std::is_same_v<T, float>)
			data_type = BLOT_DATA_FLOAT;
		else if constexpr (std::is_same_v<T, double>)
			data_type = BLOT_DATA_DOUBLE;
		else
			BLOT_THROW(EINVAL, "unsupported type");

		if (!blot_figure_plot(this, plot_type, data_type, data_count, ptr_xs, ptr_ys, data_color, data_label, &error)) {
			throw Exception(error);
		}
	}

	template <typename T>
	void scatter(const std::vector<T> &data_xs, const std::vector<T> data_ys, blot_color data_color, const char *data_label) {
		plot(BLOT_SCATTER, data_xs, data_ys, data_color, data_label);
	}

	template <typename T>
	void scatter(const std::vector<T> data_ys, blot_color data_color, const char *data_label) {
		plot(BLOT_SCATTER, {}, data_ys, data_color, data_label);
	}

	template <typename T>
	void line(const std::vector<T> &data_xs, const std::vector<T> data_ys, blot_color data_color, const char *data_label) {
		plot(BLOT_LINE, data_xs, data_ys, data_color, data_label);
	}

	template <typename T>
	void line(const std::vector<T> data_ys, blot_color data_color, const char *data_label) {
		plot(BLOT_LINE, {}, data_ys, data_color, data_label);
	}

	template <typename T>
	void bar(const std::vector<T> &data_xs, const std::vector<T> data_ys, blot_color data_color, const char *data_label) {
		plot(BLOT_BAR, data_xs, data_ys, data_color, data_label);
	}

	template <typename T>
	void bar(const std::vector<T> data_ys, blot_color data_color, const char *data_label) {
		plot(BLOT_BAR, {}, data_ys, data_color, data_label);
	}

	/* render */

	Screen render(blot_render_flags flags) {
		GError *error = nullptr;
		blot_screen * screen = blot_figure_render(this, flags, &error);
		if (!screen)
			throw Exception(error);
		return Screen(screen);
	}

};

};
