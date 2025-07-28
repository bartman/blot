#include <format>
#include <iostream>
#include <chrono>
#include <thread>
#include <ranges>

#include "config.hpp"
#include "reader.hpp"
#include "blot.hpp"
#include "spdlog/spdlog.h"

int main(int argc, char *argv[])
{
	Config config(argc, argv);

	std::cout << std::format("output_type = {}\n", config.output_type_name());

	std::vector<std::unique_ptr<Reader>> readers;

	for (auto &input : config.m_inputs) {
		std::cout << std::format("-> {} {} {} {} {}\n",
			   input.plot_name(),
			   input.source_name(),
			   input.m_details,
			   input.m_plot_color,
			   input.m_interval);

		readers.push_back(Reader::from(input));
	}

	while(std::all_of(readers.begin(), readers.end(), [](auto &reader)->bool { return *reader; })) {

		for (auto &reader : readers) {
			if (reader->idle())
				continue;

			auto line = reader->line();
			if (line.has_value())
				std::cout << *line << std::endl;
		}

		auto idle_view = readers | std::views::transform([](const auto& reader) { return reader->idle(); });
		double idle = std::ranges::min(idle_view);

		if (idle > 0) {
			std::this_thread::sleep_for(std::chrono::duration<double>(idle));
		}
	}
}
