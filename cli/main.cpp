#include <format>
#include <iostream>

#include "config.hpp"
#include "reader.hpp"
#include "blot.hpp"
#include "spdlog/spdlog.h"

int main(int argc, char *argv[])
{
	Config config(argc, argv);

	std::cout << std::format("output_type = {}\n", config.output_type_name());

	for (auto &input : config.m_inputs) {
		std::cout << "---------------------------------------------\n";
		std::cout << std::format("-> {} {} {} {} {}\n",
			   input.plot_name(),
			   input.source_name(),
			   input.m_details,
			   input.m_plot_color,
			   input.m_interval);

		auto reader = Reader::from(input);
		while (*reader) {
			auto line = reader->line();
			if (line.has_value())
				std::cout << *line << std::endl;
		}
	}
}
