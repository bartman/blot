#pragma once

#include "blot.hpp"
#include "config.hpp"

#include "blot.hpp"
#include "spdlog/spdlog.h"

template <typename X, typename Y>
class Plotter final {
protected:
	struct Data {
		std::vector<X> m_xs;
		std::vector<Y> m_ys;
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

			fig.plot(input.plot_type(), data.m_xs, data.m_ys,
				input.plot_color(), input.details());
		}

		double t_add = timing ? blot_double_time() : 0;

		blot_render_flags flags
			= BLOT_RENDER_BRAILLE
			| BLOT_RENDER_LEGEND_BELOW
			| BLOT_RENDER_CLEAR;

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
