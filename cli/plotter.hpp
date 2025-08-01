#pragma once

#include "blot.hpp"
#include "config.hpp"

#include "blot.hpp"
#include "spdlog/spdlog.h"

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

		setlocale(LC_CTYPE, "");

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

		printf("%ls", txt);
		fflush(stdout);

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
