#include <format>
#include <iostream>

#include "blot.hpp"
#include "spdlog/spdlog.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
#include "clipp.h"
#pragma GCC diagnostic pop

struct Input {
	enum Type {NONE,FILE,EXEC} m_type;
	std::string m_details;
	blot_plot_type m_plot_type;
	blot_color m_plot_color;
};

int main(int argc, char *argv[])
{
	const char *self = basename(argv[0]);

	bool show_help{}, be_verbose{}, show_version{};
	auto head_cli = (
		clipp::option("-h", "--help").set(show_help).doc("This help"),
		clipp::option("-v", "--verbose").set(be_verbose).doc("Enable debug"),
		clipp::option("-V", "--version").set(show_version).doc("Version")
	);

	enum output_type { ASCII, UNICODE, BRAILLE } output_type;

	auto output_cli = (
		clipp::option("-A", "--ascii").set(output_type,ASCII).doc("ASCII output") |
		clipp::option("-U", "--unicode").set(output_type,UNICODE).doc("Unicode output") |
		clipp::option("-B", "--braille").set(output_type,BRAILLE).doc("Braille output")
	);

	std::vector<std::string> wrong;
	auto wrong_cli = clipp::any_other(wrong);

	std::vector<Input> inputs;

	auto start_input = [&](blot_plot_type new_plot_type) {
		inputs.push_back({});
	};
	auto set_source = [&](Input::Type type, const std::string &details) {
		inputs.back().m_type = type;
		inputs.back().m_details = details;
	};
	auto set_color = [&](const std::string &c) {
		inputs.back().m_plot_color = std::strtol(c.c_str(), NULL, 10);
	};

	auto cli = (
		head_cli |
		clipp::repeatable(
			/* select a plot type */

			clipp::command("scatter").call([&]() { start_input(BLOT_SCATTER); }).doc("Add a scatter plot") |
			clipp::command("line")   .call([&]() { start_input(BLOT_LINE);    }).doc("Add a line/curve plot") |
			clipp::command("bar")    .call([&]() { start_input(BLOT_BAR);     }).doc("Add a bar plot"),

			/* augment this plots chracteristics */

			clipp::option("-c", "--color").doc("Set plot color")
			& clipp::value("color").call([&](const char *c) { set_color(c); }),

			/* source data from file or command */

			clipp::option("-f", "--file").doc("Read from a file")
			& clipp::value("file")   .call([&](const char *f) { set_source(Input::FILE, f); })
			|
			clipp::option("-x", "--exec").doc("Run command")
			& clipp::value("command").call([&](const char *x) { set_source(Input::EXEC, x); })

		),
		wrong_cli
	);

	if(!parse(argc, argv, cli) || wrong.size()) {
		spdlog::error("Failed to parse options.");
		for (const auto &w : wrong) {
			spdlog::error("Unexpected: {}", w);
		}
		spdlog::error("Usage:\n{}", clipp::usage_lines(cli, self).str());
		return 1;
	}

	if (show_version) {
		std::cout << "refspec: " << BLOT_GIT_REFSPEC << '\n'
			<< "revision: " << BLOT_GIT_REVISION << '\n'
			<< "sha1: " << BLOT_GIT_SHA1 << std::endl;
		return 0;
	}

	if (show_help) {
		Blot::Dimensions term;
		unsigned doc_start = std::min(30u, term.cols/2);
		auto fmt = clipp::doc_formatting{}
			.indent_size(4)
			.first_column(4)
			.doc_column(doc_start)
			.last_column(term.cols);
		std::cout << clipp::make_man_page(cli, self, fmt)
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
		return 0;
	}

	std::cout << std::format("output_type = {}", (int)output_type) << std::endl;

	for (auto &input : inputs) {
		std::cout << std::format("-> {} {} {} {}",
			(int)input.m_type,
			input.m_details,
			(int)input.m_plot_type,
			(int)input.m_plot_color)
		<< std::endl;
	}
}
