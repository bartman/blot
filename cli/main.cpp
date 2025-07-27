#include <format>
#include <iostream>

#include "config.hpp"
#include "blot.hpp"
#include "spdlog/spdlog.h"

int main(int argc, char *argv[])
{
	Config config(argc, argv);

	std::cout << std::format("output_type = {}", (int)config.m_output_type) << std::endl;

	for (auto &input : config.m_inputs) {
		std::cout << std::format("-> {} {} {} {}",
			input.plot_name(),
			input.source_name(),
			input.m_details,
			input.m_plot_color)
		<< std::endl;
	}
}
