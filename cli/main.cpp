#include <format>
#include <iostream>

#include "blot.hpp"
#include "spdlog/spdlog.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
#include "clipp.h"
#pragma GCC diagnostic pop

int main(int argc, char *argv[])
{
	const char *self = basename(argv[0]);

	bool show_help{}, be_verbose{}, show_version{};
	enum output_type { ASCII, UNICODE, BRAILLE } output_type;
	blot_plot_type plot_type = BLOT_LINE;

	auto head_cli = (
		clipp::option("-h", "--help").set(show_help).doc("This help"),
		clipp::option("-v", "--verbose").set(be_verbose).doc("Enable debug"),
		clipp::option("-V", "--version").set(show_version).doc("Version")
	);

	auto output_cli = (
		clipp::option("-A", "--ascii").set(output_type,ASCII).doc("ASCII output") |
		clipp::option("-U", "--unicode").set(output_type,UNICODE).doc("Unicode output") |
		clipp::option("-B", "--braille").set(output_type,BRAILLE).doc("Braille output")
	);

	blot_color plot_color = 9;
	auto modifier_cli = (
		clipp::option("-s", "--scatter").set(plot_type, BLOT_SCATTER).doc("Scatter plot").repeatable(true),
		clipp::option("-l", "--line").set(plot_type, BLOT_LINE).doc("Line plot").repeatable(true),
		clipp::option("-b", "--bar").set(plot_type, BLOT_BAR).doc("Bar plot").repeatable(true),

		clipp::repeatable(
			clipp::option("-c", "--color").doc("Color")
			& clipp::value("color").call([&](std::string c) {
				plot_color = std::strtol(c.c_str(), NULL, 10);
			})
		)
	);

	struct Input {
		enum {FILE,EXEC} type;
		std::string details;
		blot_plot_type plot_type;
		blot_color plot_color;
	};
	std::vector<Input> inputs;

	auto input_cli = (
		clipp::repeatable(
			clipp::option("-f", "--file")
			& clipp::value("file").call([&](std::string f) { inputs.push_back({Input::FILE, f, plot_type, plot_color}); })
		).doc("read from a file"),
		clipp::repeatable(
			clipp::option("-x", "--exec")
			& clipp::value("command").call([&](std::string x) { inputs.push_back({Input::EXEC, x, plot_type, plot_color}); })
		).doc("execute command")
	);

	std::vector<std::string> wrong;
	auto cli = (
		head_cli ,
		"Output" % output_cli,
		"Modifier" % modifier_cli,
		"Input" % input_cli,
		clipp::any_other(wrong)
	);

	if(!parse(argc, argv, cli) || wrong.size()) {
		spdlog::error("Failed to parse options.");
		for (const auto &w : wrong) {
			spdlog::error(" {}", w);
		}
		spdlog::error("Usage: {}", clipp::usage_lines(cli, self).str());
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
			(int)input.type,
			input.details,
			(int)input.plot_type,
			(int)input.plot_color)
		<< std::endl;
	}
}
