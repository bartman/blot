#include "config.hpp"

#include <iostream>
#include <charconv>
#include <cmath>
#include <cstdlib>
#include <unistd.h>
#include <ranges>
#include <system_error>

#include "blot.hpp"
#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "fmt/format.h"

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
		clipp::option("-V", "--version").set(show_version).doc("Version")
	);
	auto cli_mod = "Run modifiers:" % (
		clipp::option("-v", "--verbose").call([]{
			spdlog::set_level(spdlog::level::debug);
		}).doc("Enable verbose output"),
		clipp::option("--debug").call([]{
			spdlog::set_level(spdlog::level::trace);
		}).doc("Enable debug output"),
		clipp::option("--timing").set(m_show_timing).doc("Show timing statitiscs"),
		(clipp::option("-i", "--interval") & clipp::value("sec")
			.call([&](const char *txt) {
				m_display_interval = parse_seconds("display interval", txt);
			}))
			.doc("Display interval in seconds while the input lasts (default 1)"),
		/* repeatable: the last --color= or --no-color given wins */
		clipp::joinable(clipp::option("--color=") & clipp::value("when")
			.call([&](const char *txt) { set_color_mode(txt); }))
			.repeatable(true)
			.doc("Colour: auto (on a terminal, the default), always (also "
			     "clears the screen for each frame) or never"),
		clipp::option("--no-color").repeatable(true)
			.call([&]{ m_color_mode = ColorMode::NEVER; })
			.doc("No colour, same as --color=never")
	);

	auto cli_output = "Output:" % one_of (
		clipp::option("-A", "--ascii").set(m_output_type,ASCII).doc("ASCII output"),
		clipp::option("-U", "--unicode").set(m_output_type,UNICODE).doc("Unicode output"),
		clipp::option("-B", "--braille").set(m_output_type,BRAILLE).doc("Braille output")
	);

	std::vector<std::string> wrong;
	auto cli_wrong = clipp::any_other(wrong);

	auto start_input = [&](blot_plot_type plot_type) {
		blot_color color = m_inputs.empty()
			? m_first_color
			: m_inputs.back().plot_color() + 1;
		m_inputs.push_back(Input{plot_type, color});
	};

	auto cli = (
		cli_head | (
			cli_mod,
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

				"Plot data source:" % one_of(
					/* source data from file or command */

					(clipp::option("-R", "--read") & clipp::value("file")
						.call([&](const char *f) { m_inputs.back().set_source(Input::READ, f); }))
						.doc("Read file to the end, each line is a record"),
					(clipp::option("-F", "--follow") & clipp::value("file")
						.call([&](const char *f) { m_inputs.back().set_source(Input::FOLLOW, f); }))
						.doc("Read file waiting for more, each line is a record"),
					(clipp::option("-P", "--poll") & clipp::value("file")
						.call([&](const char *f) { m_inputs.back().set_source(Input::POLL, f); }))
						.doc("Read file at interval, each read is one record"),
					(clipp::option("-X", "--exec") & clipp::value("cmd")
						.call([&](const char *x) { m_inputs.back().set_source(Input::EXEC, x); }))
						.doc("Run command, each line is a record"),
					(clipp::option("-W", "--watch") & clipp::value("cmd")
						.call([&](const char *x) { m_inputs.back().set_source(Input::WATCH, x); }))
						.doc("Run command at interval, each read is one record")
				),

				"Data source parsing:" % (
					/* how to extract values from lines */

					(clipp::option("-p", "--position") & clipp::value("y-pos|x-pos,y-pos")
						.call([&](const char *txt) { m_inputs.back().set_position(txt); }))
						.doc("Find numbers in input line, pick 1 or 2 positions for X and Y values"),
					(clipp::option("-r", "--regex") & clipp::value("regex")
						.call([&](const char *txt) { m_inputs.back().set_regex(txt); }))
						.doc("Regex to match numbers from input line"),
					(clipp::option("-l", "--limit") & clipp::value("count")
						.call([&](const char *txt) { m_inputs.back().set_data_limit(txt); }))
						.doc("How many historical values to retain for plotting")
				),

				"Plot modifiers:" % (
					/* augment this plots characteristics */

					(clipp::option("-c", "--color") & clipp::value("color")
						.call([&](const char *txt) { m_inputs.back().set_color(txt); }))
						.doc("Set plot color (1..255)"),
					(clipp::option("-i", "--interval") & clipp::value("sec")
						.call([&](const char *txt) { m_inputs.back().set_interval(txt); }))
						.doc("Set sampling interval in seconds")
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

	/* colour (#44): --color=always or never as given; auto (the
	 * default) is none when NO_COLOR is set and not empty
	 * (https://no-color.org), otherwise only on a terminal.  The screen
	 * clear before each frame is for a terminal too: through a pipe or
	 * into a file, the output is the plain text of the plot, unless
	 * --color=always asks for the terminal's output */
	bool tty = isatty(STDOUT_FILENO);
	const char *no_color = std::getenv("NO_COLOR");
	switch (m_color_mode) {
		case ColorMode::ALWAYS:
			m_color = true;
			break;
		case ColorMode::NEVER:
			m_color = false;
			break;
		case ColorMode::AUTO:
			m_color = tty && !(no_color && *no_color);
			break;
	}
	m_clear = tty || m_color_mode == ColorMode::ALWAYS;

	/* and the messages on stderr: spdlog colours them on a terminal
	 * whatever --no-color or NO_COLOR say.  Here, before the checks
	 * below, so their messages follow it too */
	if (!m_color) {
		for (auto &sink : spdlog::default_logger()->sinks()) {
			auto c = std::dynamic_pointer_cast<spdlog::sinks::stderr_color_sink_mt>(sink);
			if (c)
				c->set_color_mode(spdlog::color_mode::never);
		}
	}

	if (show_version) {
		std::cout << "refspec: " << BLOT_GIT_REFSPEC << '\n'
			<< "revision: " << BLOT_GIT_REVISION << '\n'
			<< "sha1: " << BLOT_GIT_SHA1 << std::endl;
		std::exit(0);
	}

	if (show_help) {
		/* the terminal's width, or 80 columns when there is none
		 * (e.g. blot --help | less, without COLUMNS and LINES) */
		unsigned cols = 80;
		try {
			Blot::Dimensions term;
			if (term.cols)
				cols = term.cols;
		} catch (const Blot::Exception &) {
		}
		unsigned doc_start = std::min(32u, cols/2);
		auto fmt = clipp::doc_formatting{}
			.indent_size(4)
			.first_column(4)
			.doc_column(doc_start)
			.last_column(cols);
		std::cout << clipp::make_man_page(cli, m_self, fmt)
			.append_section("EXAMPLES",
		   "\n"
		   "    blot --braille \\\n"
		   "        line    --color 10 --read x_y1_values -p 1,2 \\\n"
		   "        scatter --color 11 --read x_y2_values -p 1,2\n"
		   "\n"
		   "    blot --braille \\\n"
		   "        scatter --color 11 --read y_values \\\n"
		   "        line    --color 10 --exec 'seq 1 100'\n"
		   "\n"
		   "    blot --braille \\\n"
		   "        line --poll /proc/loadavg --position 1 \\\n"
		   "        line --poll /proc/loadavg --position 2 \\\n"
		   "        line --poll /proc/loadavg --position 3\n"
		   )
			<< std::endl;
		std::exit(0);
	}

	if (m_inputs.empty()) {
		spdlog::error("no plots defined");
		std::exit(1);
	}

	bool with_interval{}, no_interval{};
	double prev_interval = 1;
	size_t prev_data_limit = Input::g_default_data_limit;
	for (auto &input : m_inputs) {
		if (input.needs_interval()) {
			if (!input.interval()) {
				input.set_interval(prev_interval);
			} else {
				prev_interval = input.interval();
			}
		}
		if (!input.data_limit()) {
			input.set_data_limit(prev_data_limit);
		} else {
			prev_data_limit = input.data_limit();
		}
		if (!input) {
			spdlog::error("incomplete plot definition");
			std::exit(1);
		}
		with_interval |= !!(input.interval());
		no_interval |= !(input.interval());
	}
	if (with_interval && no_interval) {
		spdlog::error("cannot mix interval and non-interval sources");
		std::exit(1);
	}
	m_using_input_interval = with_interval;
}

void Config::set_color_mode(const std::string &txt)
{
	if (txt == "auto")
		m_color_mode = ColorMode::AUTO;
	else if (txt == "always")
		m_color_mode = ColorMode::ALWAYS;
	else if (txt == "never")
		m_color_mode = ColorMode::NEVER;
	else {
		spdlog::error("invalid --color '{}': expected auto, always or never", txt);
		std::exit(1);
	}
}

/* a number of seconds: positive and finite, all of txt (#44) */
double Config::parse_seconds(const char *what, const std::string &txt)
{
	double value{};
	const char *end = txt.data() + txt.size();
	auto [ptr, ec] = std::from_chars(txt.data(), end, value);
	if (ec != std::errc{} || ptr != end || !std::isfinite(value) || value <= 0) {
		spdlog::error("invalid {} '{}': expected a positive number of seconds",
			what, txt);
		std::exit(1);
	}
	return value;
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

void Input::set_position (const std::string &txt)
{
	/* ranges and views are fun, but not all standard libraries support it */
#if defined(__cpp_lib_ranges) && __cpp_lib_ranges >= 201911L && defined(__GLIBCXX__) && __GLIBCXX__ >= 20230601
	auto result = std::views::split(txt, ',')
		| std::views::transform([](auto&& sr) {
			std::string_view sv{sr.begin(), sr.end()};

			unsigned number;
			auto [_,ec] = std::from_chars(sv.begin(), sv.end(), number);
			if (ec != std::errc{}) {
				spdlog::error("failed to parse position from '{}': {}",
						sv, std::make_error_code(ec).message());
				std::exit(1);
			}
			return number;
		});

	std::vector<unsigned> positions(result.begin(), result.end());

	switch (positions.size()) {
		case 1:
			m_extract.set(positions[0]);
			break;
		case 2:
			m_extract.set(std::pair<unsigned,unsigned>{positions[0], positions[1]});
			break;
		default:
			spdlog::error("unexpected count ({}) of positions found in '{}'",
		 positions.size(), txt);
			std::exit(1);
	}
#else
	std::vector<unsigned> positions;
	std::string_view sv = txt;
	size_t start = 0;

	while (true) {
		size_t comma_pos = sv.find(',', start);
		std::string_view part = (comma_pos == std::string_view::npos)
			? sv.substr(start)
			: sv.substr(start, comma_pos - start);

		unsigned number;
		auto [ptr, ec] = std::from_chars(part.begin(), part.end(), number);
		if (ec != std::errc{}) {
			spdlog::error("failed to parse position from '{}': {}",
				part, std::make_error_code(ec).message());
			std::exit(1);
		}
		positions.push_back(number);

		if (comma_pos == std::string_view::npos) {
			break;
		}
		start = comma_pos + 1;
	}

	switch (positions.size()) {
		case 1:
			m_extract.set(positions[0]);
			break;
		case 2:
			m_extract.set(std::pair<unsigned, unsigned>{positions[0], positions[1]});
			break;
		default:
			spdlog::error("unexpected count ({}) of positions found in '{}'",
				positions.size(), txt);
			std::exit(1);
	}

#endif
}

void Input::set_regex (const std::string &txt)
{
	m_extract.set(std::regex(txt));
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

void Input::set_interval (double interval)
{
	if (needs_interval() && !interval) {
		spdlog::warn("{} {} plot cannot have zero interval",
			blot_plot_type_to_string(m_plot_type), source_name());
		std::exit(1);
	}
	m_interval = interval;
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
void Input::set_data_limit (size_t value)
{
	m_data_limit = value;
}
void Input::set_data_limit (const std::string &txt)
{
	auto [_,ec] = std::from_chars(txt.data(), txt.data()+txt.size(), m_data_limit);
	if (ec != std::errc{}) {
		spdlog::error("failed to parse data limit from '{}': {}",
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
	switch (m_source) {
		case READ:
		case FOLLOW:
		case EXEC:
		case POLL:
		case WATCH:
			break;
		case NONE:
			spdlog::warn("{} plot does not defined an input (file or command)",
				blot_plot_type_to_string(m_plot_type));
			return false;
		default:
			spdlog::warn("{} plot has invalid type: {}",
				blot_plot_type_to_string(m_plot_type), (int)m_source);
			return false;
	}
	if (needs_interval() && !m_interval) {
		spdlog::warn("{} plot has zero {} interval",
			blot_plot_type_to_string(m_plot_type), source_name());
		return false;
	} else if (!needs_interval() && m_interval) {
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

