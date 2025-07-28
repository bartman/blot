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

#pragma GCC diagnostic ignored "-Wunused-variable"

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
		}).doc("Enable debug output")
	);

	auto cli_output = "Output:" % (
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
		cli_head | (
			cli_output,
			clipp::repeatable(
				/* select a plot type */

				"Plot type:" % (

				clipp::command("scatter")
					.call([&]() { start_input(BLOT_SCATTER); })
					.doc("Add a scatter plot") |
				clipp::command("line")
					.call([&]() { start_input(BLOT_LINE); })
					.doc("Add a line/curve plot") |
				clipp::command("bar")
					.call([&]() { start_input(BLOT_BAR); })
					.doc("Add a bar plot")
				),

				/* modifiers - no heading because there is a --help bug in clipp */

				(

				/* source data from file or command */

				clipp::option("-r", "--read").doc("Read file to the end, each line is a record")
				& clipp::value("file")
					.call([&](const char *f) { m_inputs.back().set_source(Input::READ, f); })
				|
				clipp::option("-f", "--follow").doc("Read file waiting for more, each line is a record")
				& clipp::value("file")
					.call([&](const char *f) { m_inputs.back().set_source(Input::FOLLOW, f); })
				|
				clipp::option("-p", "--poll").doc("Read file at interval, each read is one record")
				& clipp::value("file")
					.call([&](const char *f) { m_inputs.back().set_source(Input::POLL, f); })
				|
				clipp::option("-x", "--exec").doc("Run command, each line is a record")
				& clipp::value("command")
					.call([&](const char *x) { m_inputs.back().set_source(Input::EXEC, x); })
				|
				clipp::option("-w", "--watch").doc("Run command at interval, each read is one record")
				& clipp::value("command")
					.call([&](const char *x) { m_inputs.back().set_source(Input::WATCH, x); }),

				/* augment this plots characteristics */

				clipp::option("-c", "--color").doc("Set plot color (1..255)")
				& clipp::value("color")
					.call([&](const char *txt) { m_inputs.back().set_color(txt); }),
				clipp::option("-i", "--interval").doc("Set interval in seconds")
				& clipp::value("seconds")
					.call([&](const char *txt) { m_inputs.back().set_interval(txt); })

				)

			),
			cli_wrong
		)
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

void Input::set_source (Input::Source source, const std::string &details)
{
	if (m_source != NONE || m_details.size()) {
		spdlog::error("Input source being set twice, m_source={}/{}, m_details='{}'",
			int(m_source), source_name(), m_details);
		std::exit(1);
	}
	m_source = source;
	m_details = details;
}

void Input::set_color (const std::string &txt)
{
	unsigned number;
	auto [_,ec] = std::from_chars(txt.data(), txt.data()+txt.size(), number);
	if (ec != std::errc{}) {
		spdlog::error("failed to parse color from '{}': {}",
			txt, std::make_error_code(ec).message());
		std::exit(1);
	}
	if (number < 1 || number > 255) {
		spdlog::error("color {} is not valid", txt);
		std::exit(1);
	}
	m_plot_color = number;
}

void Input::set_interval (const std::string &txt)
{
	auto [_,ec] = std::from_chars(txt.data(), txt.data()+txt.size(), m_interval);
	if (ec != std::errc{}) {
		spdlog::error("failed to parse interval from '{}': {}",
			txt, std::make_error_code(ec).message());
		std::exit(1);
	}
}

Input::operator bool() const
{
	switch (m_plot_type) {
		case BLOT_SCATTER:
		case BLOT_LINE:
		case BLOT_BAR:
			break;
		default:
			spdlog::warn("invalid plot type: {}", (int)m_plot_type);
			return false;
	}
	bool need_interval = false;
	switch (m_source) {
		case NONE:
			spdlog::warn("{} plot does not defined an input (file or command)",
				blot_plot_type_to_string(m_plot_type));
			return false;
		case READ:
		case FOLLOW:
		case EXEC:
			break;
		case POLL:
		case WATCH:
			need_interval = true;
			break;
		default:
			spdlog::warn("{} plot has invalid type: {}",
				blot_plot_type_to_string(m_plot_type), (int)m_source);
			return false;
	}
	if (need_interval && !m_interval) {
		spdlog::warn("{} plot has zero {} interval",
			blot_plot_type_to_string(m_plot_type), source_name());
		return false;
	} else if (!need_interval && m_interval) {
		spdlog::warn("{} plot has non-zero {} interval (%f)",
			blot_plot_type_to_string(m_plot_type), source_name(), m_interval);
		return false;
	}
	if (!std::any_of(m_details.begin(), m_details.end(), [](auto c){ return !std::isspace(c); })) {
		spdlog::warn("{} plot has empty {} argument '{}'",
			blot_plot_type_to_string(m_plot_type), source_name(), m_details);
		return false;
	}

	return true;
}

