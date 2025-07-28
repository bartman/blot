#include <iostream>
#include <chrono>
#include <thread>
#include <ranges>

#include "config.hpp"
#include "reader.hpp"
#include "plotter.hpp"

#include "spdlog/spdlog.h"
#include "fmt/format.h"

static bool signaled = false;
static void sighandler(int sig)
{
	signaled = true;
}

int main(int argc, char *argv[])
{
	Config config(argc, argv);

	spdlog::debug("output_type = {}", config.output_type_name());

	std::vector<std::unique_ptr<Reader>> readers;

	signal(SIGINT, sighandler);

	auto keep_going = [&]{
		return !signaled
			&& std::any_of(readers.begin(), readers.end(),
				[](auto &reader)->bool {
					return *reader;
				});
	};

	for (size_t i=0; i<config.inputs(); i++) {
		const auto &input = config.input(i);
		spdlog::debug("source[{}] {} {} '{}' {} {}", i,
			   input.plot_name(),
			   input.source_name(),
			   input.details(),
			   input.plot_color(),
			   input.interval());

		readers.push_back(Reader::from(input));
	}

	Plotter<double,double> plotter(config);

	while(keep_going()) {

		for (size_t i=0; i<readers.size(); i++) {
			if (signaled)
				return 1;

			const auto &input = config.input(i);
			auto &reader = readers[i];
			if (reader->idle())
				continue;

			auto line = reader->line();
			if (!line.has_value())
				continue;

			spdlog::trace("{}:{}: {}", input.details(), line->number, line->text);

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

		if (signaled)
			return 1;

		/* wait for the next time we have data */

		auto idle_view = readers | std::views::transform([](const auto& reader) { return reader->idle(); });
		double idle = std::ranges::min(idle_view);

		if (idle > 0) {
			plotter.plot();

			// unfortunately std::this_thread::sleep_for() cannot be used,
			// as it will complete the sleep even if SIGINT is raised.

			double useconds = idle * 1000000;
			usleep(useconds);
		}
	}

	plotter.plot();
}
