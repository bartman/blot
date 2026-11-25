#pragma once

#include "blot.hpp"
#include "config.hpp"
#include "plot_encode.hpp"

#include "blot.hpp"
#include "spdlog/spdlog.h"

#include <cerrno>
#include <climits>
#include <clocale>
#include <cstdio>
#include <cstring>
#include <cwchar>
#include <cstdlib>
#include <initializer_list>
#include <stdexcept>
#include <string>

/* one warning about characters written as '?' is enough */
inline bool plot_warned_once()
{
	static bool warned;
	bool was = warned;
	warned = true;
	return was;
}

/* the plot is wide text, written in the encoding of the locale's
 * LC_CTYPE.  The C and POSIX locales (no LANG, as in a docker container
 * or CI) encode only ASCII, and a plot isn't ASCII even with --ascii (the
 * legend's symbol), so use a UTF-8 locale there (#40).  Unlike Python's
 * PEP 538 this also applies when LC_ALL=C is set explicitly */
inline void plot_locale()
{
	static bool done;
	if (done)
		return;
	done = true;

	const char *loc = setlocale(LC_CTYPE, "");
	if (loc && strcmp(loc, "C") && strcmp(loc, "POSIX"))
		return;

	/* C.UTF-8 is in glibc >= 2.35 and Debian; UTF-8 and en_US.UTF-8
	 * elsewhere (macOS, older glibc) */
	for (const char *utf8 : { "C.UTF-8", "C.utf8", "UTF-8", "en_US.UTF-8" }) {
		if (setlocale(LC_CTYPE, utf8)) {
			spdlog::debug("locale is {}, using {} for the plot",
				loc ? loc : "unknown", utf8);
			return;
		}
	}
	setlocale(LC_CTYPE, "C");
	if (!plot_warned_once())
		spdlog::warn("no UTF-8 locale available; the plot's "
			"non-ASCII characters are written as '?'");
}

/* write the rendered text to f in the locale's encoding, all of it.
 * printf("%ls") gave up at the first character the locale can't encode
 * and returned -1, unchecked: the plot was cut off mid-line, at a spot
 * that depends on glibc's version and stdout's buffering (#40).  See
 * plot_encode() for a character the locale can't encode.  Throws on a
 * write error */
inline void plot_write(FILE *f, const wchar_t *txt, size_t len)
{
	bool replaced;
	std::string out = plot_encode(txt, len, &replaced);

	if (replaced && !plot_warned_once())
		spdlog::warn("the locale ({}) can't encode some of "
			"the plot's characters; they are written as '?'",
			setlocale(LC_CTYPE, nullptr));

	if (fwrite(out.data(), 1, out.size(), f) != out.size()
	    || fflush(f) != 0)
		throw std::runtime_error(std::string("cannot write the plot: ")
					 + strerror(errno));
}

template <typename X, typename Y>
class Plotter final {
protected:
	template <typename T>
	class Storage {
	protected:
		std::vector<T> m_data;
		size_t m_offset{};

		void relocation_check() {
			bool full = m_data.size() == m_data.capacity();
			if (full && m_offset && m_offset > (m_data.capacity()/2)) {
				// full but most is empty, so we should relocate the data
				m_data.erase(m_data.begin(), m_data.begin() + m_offset);
				m_offset = 0;
			}
		}

	public:
		void push_back(const T &v) {
			relocation_check();
			m_data.push_back(v);
		}

		void erase_front(size_t count = 1) {
			if (count > size())
				BLOT_THROW(ERANGE, "count=%zu exceeds size=%zu", count, size());
			m_offset += count;
		}

		T* data() noexcept { return m_data.data() + m_offset; }
		const T* data() const noexcept { return m_data.data() + m_offset; }
		size_t size() const { return m_data.size() - m_offset; }

		std::span<T> span() {
			return std::span<const T>(data(), size());
		}
		std::span<const T> span() const {
			return std::span<const T>(data(), size());
		}

	};

	struct Data {
		Storage<X> m_xs;
		Storage<Y> m_ys;
	};
	std::vector<Data> m_data;
	size_t m_count{};

	const Config &m_config;
	size_t m_max_layers{};
	size_t m_data_history{};

	struct {
		size_t count{};
		double init{};
		double add{};
		double render{};
		double print{};
		double total{};
	} m_stats;


public:
	explicit Plotter(const Config &config)
	: m_config(config), m_max_layers(m_config.inputs())
	{
		m_data.resize(m_max_layers);
	}

	void add(size_t layer, X x, Y y) {
		m_data[layer].m_xs.push_back(x);
		m_data[layer].m_ys.push_back(y);
		m_count ++;

		if (m_data[layer].m_xs.size() > m_config.input(layer).data_limit()) {
			m_data[layer].m_xs.erase_front();
			m_data[layer].m_ys.erase_front();
		}
	}

	bool have_data() const { return m_count > 0; }

	void plot() {

		plot_locale();

		bool timing = m_config.show_timing();

		double t_start = timing ? blot_double_time() : 0;

		Blot::Figure fig;
		fig.set_axis_color(8);

		#if 0
		Blot::Dimensions term;
		fig.set_screen_size(term.cols, term.rows/2);
		#endif

		double t_init = timing ? blot_double_time() : 0;

		for (size_t i=0; i<m_data.size(); i++) {
			const auto &data = m_data[i];
			const auto &input = m_config.input(i);

			fig.plot(input.plot_type(), data.m_xs.span(), data.m_ys.span(),
				input.plot_color(), input.details());
		}

		double t_add = timing ? blot_double_time() : 0;

		blot_render_flags flags
			= BLOT_RENDER_LEGEND_BELOW
			| BLOT_RENDER_CLEAR;

		switch (m_config.output_type()) {
			case Config::ASCII:
				flags = flags | BLOT_RENDER_NO_UNICODE;
				break;
			case Config::BRAILLE:
				flags = flags | BLOT_RENDER_BRAILLE;
				break;
			default: // UNICODE
				break;
		}

		if (timing)
			flags = flags | BLOT_RENDER_LEGEND_DETAILS;

		Blot::Screen scr = fig.render(flags);

		double t_render = timing ? blot_double_time() : 0;

		gsize txt_size = 0;
		const wchar_t *txt = scr.get_text(txt_size);

		spdlog::debug("rendered size = %zu", txt_size);

		plot_write(stdout, txt, txt_size);

		double t_print = timing ? blot_double_time() : 0;

		if (timing) {
			m_stats.count ++;
			m_stats.init   += t_init   - t_start;
			m_stats.add    += t_add    - t_init;
			m_stats.render += t_render - t_add;
			m_stats.print  += t_print  - t_render;
			m_stats.total  += t_print  - t_start;

			fmt::print("time: count={} init={:.6f} add={:.6f} render={:.6f} print={:.6f} [{:.6f}]",
				m_stats.count,
				m_stats.init / m_stats.count,
				m_stats.add / m_stats.count,
				m_stats.render / m_stats.count,
				m_stats.print / m_stats.count,
				m_stats.total / m_stats.count);
			std::flush(std::cout);
		}
	}
};
