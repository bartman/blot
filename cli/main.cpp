#include <format>
#include <iostream>
#include <chrono>
#include <thread>
#include <ranges>

#include "config.hpp"
#include "reader.hpp"
#include "plotter.hpp"

#include "spdlog/spdlog.h"

int main(int argc, char *argv[])
{
	Config config(argc, argv);

	spdlog::debug("output_type = {}", config.output_type_name());

	std::vector<std::unique_ptr<Reader>> readers;

	auto keep_going = [&]{
		return std::any_of(readers.begin(), readers.end(),
		     [](auto &reader)->bool {
			return *reader;
		     });
	};

	for (size_t i=0; i<config.m_inputs.size(); i++) {
		const auto &input = config.m_inputs[i];
		spdlog::debug("source[{}] {} {} '{}' {} {}", i,
			   input.plot_name(),
			   input.source_name(),
			   input.m_details,
			   input.m_plot_color,
			   input.m_interval);

		readers.push_back(Reader::from(input));
	}

	if (config.m_using_interval) {
		spdlog::error("hang on");
		std::exit(1);
	}

	Plotter<double,double> plotter(config);

	while(keep_going()) {

		for (size_t i=0; i<readers.size(); i++) {
			const auto &input = config.m_inputs[i];
			auto &reader = readers[i];
			if (reader->idle())
				continue;

			auto line = reader->line();
			if (!line.has_value())
				continue;

			spdlog::trace("{}:{}: {}", input.m_details, line->number, line->text);

			double value;
			auto [_,ec] = std::from_chars(line->text.data(),
				 line->text.data()+line->text.size(), value);
			if (ec != std::errc{}) {
				spdlog::error("failed to parse value from source {} line {} '{}': {}",
					i, line->number, line->text, std::make_error_code(ec).message());
				std::exit(1);
			}

			plotter.add(i, line->number, value);
		}

		/* wait for the next time we have data */

		auto idle_view = readers | std::views::transform([](const auto& reader) { return reader->idle(); });
		double idle = std::ranges::min(idle_view);

		if (idle > 0) {
			std::this_thread::sleep_for(std::chrono::duration<double>(idle));
		}
	}

	plotter.plot();
}
