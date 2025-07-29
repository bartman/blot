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

	double next_display = 0;

	while(keep_going()) {

		bool have_data = false;

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

			auto result = input.extract().parse<double,2>(line->text.data());
			switch (result.count) {
				case 0:
					spdlog::error("failed to parse value from source {} line {} '{}'", i, line->number, line->text);
					std::exit(1);
				case 1:
					have_data = true;
					plotter.add(i, line->number, result.array[0]);
					break;
				case 2:
				default:
					have_data = true;
					plotter.add(i, result.array[0], result.array[1]);
					break;
			}

		}

		if (signaled)
			return 1;

		bool do_show_plot = false;
		double sleep_after_seconds = 0;

		/* display periodically */

		if (double interval = config.display_interval()) {
			double now = blot_double_time();
			if (now > next_display) {
				next_display = now + interval;
				do_show_plot = true;
			}
		}

		/* wait for the next time we have data */

		auto idle_view = readers | std::views::transform([](const auto& reader) { return reader->idle(); });
		double idle = std::ranges::min(idle_view);

		if (idle > 0) {
			do_show_plot = true;
			sleep_after_seconds = idle;
		}

		if (do_show_plot && have_data)
			plotter.plot();

		if (double useconds = sleep_after_seconds * 1000000) {

			// unfortunately std::this_thread::sleep_for() cannot be used,
			// as it will complete the sleep even if SIGINT is raised.

			usleep(useconds);
		}
	}

	plotter.plot();
}
