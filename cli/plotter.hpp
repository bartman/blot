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

	const Config &m_config;
	size_t m_max_layers{};
	size_t m_data_history{};

public:
	explicit Plotter(const Config &config)
	: m_config(config), m_max_layers(m_config.m_inputs.size())
	{
		m_data.resize(m_max_layers);
	}

	void add(size_t layer, X x, Y y) {
		m_data[layer].m_xs.push_back(x);
		m_data[layer].m_ys.push_back(y);
	}

	void plot() const {

		setlocale(LC_CTYPE, "");

		Blot::Figure fig;
		fig.set_axis_color(8);

		Blot::Dimensions term;
		fig.set_screen_size(term.cols, term.rows/2);

		for (size_t i=0; i<m_data.size(); i++) {
			const auto &data = m_data[i];
			const auto &input = m_config.m_inputs[i];

			int color = input.m_plot_color;
			const char *label = input.m_details.c_str();

			fig.plot(input.m_plot_type, data.m_xs, data.m_ys, color, label);
		}

		blot_render_flags flags
			= BLOT_RENDER_BRAILLE
			| BLOT_RENDER_LEGEND_BELOW
			| BLOT_RENDER_CLEAR;

		Blot::Screen scr = fig.render(flags);

		gsize txt_size = 0;
		const wchar_t *txt = scr.get_text(txt_size);

		spdlog::debug("rendered size = %zu", txt_size);

		printf("%ls\n", txt);
		fflush(stdout);
	}
};
