#include "config.hpp"

#include <format>
#include <iostream>
#include <charconv>
#include <system_error>

#include "blot.hpp"
#include "spdlog/spdlog.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
#include "clipp.h"
#pragma GCC diagnostic pop


Config::Config(int argc, char *argv[])
: m_self(basename(argv[0]))
{
	bool show_help{}, show_version{};
	auto cli_head = (
		clipp::option("-h", "--help").set(show_help).doc("This help"),
		clipp::option("-V", "--version").set(show_version).doc("Version"),
		clipp::option("-v", "--verbose").call([]{
			spdlog::set_level(spdlog::level::debug);
		}).doc("Enable verbose output"),
		clipp::option("--debug").call([]{
			spdlog::set_level(spdlog::level::trace);
		}).doc("Enable debug output"),
		clipp::option("-i", "--interval").doc("Interval in seconds")
		& clipp::value("seconds").call([&](const char *txt){
			auto len = strlen(txt);
			auto [_,ec] = std::from_chars(txt, txt+len, m_interval);
			if (ec != std::errc{}) {
				spdlog::error("failed to parse '{}': {}",
					txt, std::make_error_code(ec).message());
				std::exit(1);
			}
		})
	);

	auto cli_output = (
		clipp::option("-A", "--ascii").set(m_output_type,ASCII).doc("ASCII output") |
		clipp::option("-U", "--unicode").set(m_output_type,UNICODE).doc("Unicode output") |
		clipp::option("-B", "--braille").set(m_output_type,BRAILLE).doc("Braille output")
	);

	std::vector<std::string> wrong;
	auto cli_wrong = clipp::any_other(wrong);

	auto start_input = [&](blot_plot_type plot_type) {
		m_inputs.push_back(Input{plot_type});
	};

	auto cli = (
		cli_head |
		cli_output,
		clipp::repeatable(
			/* select a plot type */

			clipp::command("scatter")
				.call([&]() { start_input(BLOT_SCATTER); })
				.doc("Add a scatter plot") |
			clipp::command("line")
				.call([&]() { start_input(BLOT_LINE); })
				.doc("Add a line/curve plot") |
			clipp::command("bar")
				.call([&]() { start_input(BLOT_BAR); })
				.doc("Add a bar plot"),

			/* augment this plots characteristics */

			clipp::option("-c", "--color").doc("Set plot color")
			& clipp::value("color")
				.call([&](const char *c) { m_inputs.back().set_color(c); }),

			/* source data from file or command */

			clipp::option("-r", "--read").doc("Read file to the end")
			& clipp::value("file")
				.call([&](const char *f) { m_inputs.back().set_source(Input::READ, f); })
			|
			clipp::option("-f", "--follow").doc("Read file waiting for more")
			& clipp::value("file")
				.call([&](const char *f) { m_inputs.back().set_source(Input::FOLLOW, f); })
			|
			clipp::option("-x", "--exec").doc("Run command, one value per line")
			& clipp::value("command")
				.call([&](const char *x) { m_inputs.back().set_source(Input::EXEC, x); })
			|
			clipp::option("-w", "--watch").doc("Run command, one value per call")
			& clipp::value("command")
				.call([&](const char *x) { m_inputs.back().set_source(Input::WATCH, x); })

		),
		cli_wrong
	);

	if(!parse(argc, argv, cli) || wrong.size()) {
		spdlog::error("Failed to parse options.");
		for (const auto &w : wrong) {
			spdlog::error("Unexpected: {}", w);
		}
		spdlog::error("Usage:\n{}", clipp::usage_lines(cli, m_self).str());
		std::exit(1);
	}

	if (show_version) {
		std::cout << "refspec: " << BLOT_GIT_REFSPEC << '\n'
			<< "revision: " << BLOT_GIT_REVISION << '\n'
			<< "sha1: " << BLOT_GIT_SHA1 << std::endl;
		std::exit(0);
	}

	if (show_help) {
		Blot::Dimensions term;
		unsigned doc_start = std::min(30u, term.cols/2);
		auto fmt = clipp::doc_formatting{}
			.indent_size(4)
			.first_column(4)
			.doc_column(doc_start)
			.last_column(term.cols);
		std::cout << clipp::make_man_page(cli, m_self, fmt)
			.append_section("EXAMPE",
		   "\n"
		   "    blot --braille \\\n"
		   "        line    --color 10 --file x_y1_values \\\n"
		   "        scatter --color 11 --file x_y2_values\n"
		   "\n"
		   "    blot --braille \\\n"
		   "        line    --color 10 --exec 'seq 1 100' \\\n"
		   "        scatter --color 11 --file x_y_values\n"
		   )
			<< std::endl;
		std::exit(0);
	}

	if (m_inputs.empty()) {
		spdlog::error("no plots defined");
		std::exit(1);
	}
	for (auto &input : m_inputs) {
		if (!input) {
			spdlog::error("incomplete plot definition");
			std::exit(1);
		}
	}
}
