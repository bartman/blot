#include <iostream>
#include <chrono>
#include <thread>
#include <ranges>
#include <cstdio>
#include <cstring>
#include <cerrno>

#include "config.hpp"
#include "reader.hpp"
#include "plotter.hpp"

#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "fmt/format.h"

static bool signaled = false;
static void sighandler(int sig)
{
	signaled = true;
}

static int run(int argc, char *argv[]);

/* nothing in blot is caught on the way here: a Blot::Exception (from
 * the library) or another std::exception (e.g. a bad --regex) is a
 * message and exit code 1, not std::terminate (#37) */
int main(int argc, char *argv[])
{
	try {
		/* stdout is the plot: messages go to stderr, not into it, and
		 * still show when stdout can't be written (#40) */
		spdlog::set_default_logger(std::make_shared<spdlog::logger>("",
			std::make_shared<spdlog::sinks::stderr_color_sink_mt>()));
		return run(argc, argv);
	} catch (const std::exception &e) {
		const char *what = e.what();
		spdlog::error("{}", what ? what : "unknown error");
	} catch (...) {
		spdlog::error("unknown error");
	}
	return 1;
}

static int run(int argc, char *argv[])
{
	Config config(argc, argv);

	/* a plot fills the terminal, so it needs its size: from the tty, or
	 * from COLUMNS and LINES when stdout isn't one.  Fail here, not after
	 * reading all the input */
	try {
		Blot::Dimensions term;
	} catch (const Blot::Exception &e) {
		const char *what = e.what();
		spdlog::error("cannot get the terminal size ({}); "
			"set COLUMNS and LINES when stdout is not a terminal",
			what ? what : "unknown error");
		return 1;
	}

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

	/* the first frame is one interval in: a file read to the end before
	 * then is one frame with all of it, not a first one with only its
	 * first value (#44, #47) */
	double next_display = blot_double_time() + config.display_interval();

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

			auto result = input.extract().parse<double,double>(line->number,
						 line->text.data());
			if (result.has_value()) {
				plotter.add(i, result->x, result->y);
			} else {
				spdlog::error("failed to parse value from source {} line {} '{}'",
					i, line->number, line->text);
				std::exit(1);
			}
		}

		if (signaled)
			return 1;

		bool do_show_plot = false;
		double sleep_after_seconds = 0;

		/* display periodically */

		double now = blot_double_time();
		if (now >= next_display) {
			next_display = now + config.display_interval();
			do_show_plot = true;
		}

		/* wait for the next time we have data */

		double max_idle = 0;
		for (size_t i=0; i<readers.size(); i++) {
			const auto &reader = readers[i];
			double reader_idle = reader->idle();
			max_idle = i ? std::max(max_idle, reader_idle) : reader_idle;
		}

		if (max_idle > 0) {
			do_show_plot = true;
			sleep_after_seconds = max_idle;
		}

		if (do_show_plot && plotter.have_data())
			plotter.plot();

		if (double useconds = sleep_after_seconds * 1000000) {

			// unfortunately std::this_thread::sleep_for() cannot be used,
			// as it will complete the sleep even if SIGINT is raised.

			usleep(useconds);
		}
	}

	plotter.plot();
	std::puts("");
	/* the output is the plot: a write error is a failure (#40) */
	if (std::fflush(stdout) != 0 || std::ferror(stdout)) {
		spdlog::error("cannot write the plot: {}", std::strerror(errno));
		return 1;
	}
	return 0;
}
