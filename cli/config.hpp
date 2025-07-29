#pragma once

#include <cstdint>
#include <vector>
#include <string>

#include "extract.hpp"

#include "blot.hpp"
#include "spdlog/spdlog.h"

class  Input final {
public:
	enum Source {
		NONE,		// not yet initialized
		READ,		// read from a file, each line is an entry
		FOLLOW,		// like READ, but wait for more data
		POLL,		// read a file repetitively at interval, each read is an entry
		EXEC,		// run program, read from stdout, each line is an entry
		WATCH		// run program repetitively at interval, each run is one entry
	};

protected:
	blot_plot_type m_plot_type{BLOT_LINE};
	Source m_source{NONE};
	std::string m_details{};
	Extract m_extract;
	blot_color m_plot_color;
	double m_interval{0};

public:
	explicit Input(blot_plot_type plot_type, blot_color color)
	: m_plot_type(plot_type), m_plot_color(color) {}
	~Input() {}

	blot_plot_type plot_type() const { return m_plot_type; }
	std::string plot_name() const {
		return blot_plot_type_to_string(m_plot_type);
	}

	Source source() const { return m_source; }
	std::string source_name() const {
		switch (m_source) {
			case READ: return "read";
			case FOLLOW: return "follow";
			case EXEC: return "exec";
			case WATCH: return "watch";
			default: return {};
		}
	}

	bool needs_interval() const {
		switch (m_source) {
			case POLL:
			case WATCH:
				return true;
			default:
				return false;
		}
	}

	const char * details() const { return m_details.c_str(); }
	const Extract& extract() const { return m_extract; }
	blot_color plot_color() const { return m_plot_color; }
	double interval() const { return m_interval; }

	/* validate if the configuration looks sane */
	operator bool() const;

	void set_source (Input::Source source, const std::string &details);
	void set_position (const std::string &txt);
	void set_regex (const std::string &txt);
	void set_color (const std::string &txt);
	void set_interval (double interval);
	void set_interval (const std::string &txt);


};

class  Config final {
protected:
	const char *m_self{};
	enum output_type { ASCII, UNICODE, BRAILLE } m_output_type;
	const static blot_color m_first_color{9};
	std::vector<Input> m_inputs;
	bool m_display_interval{1};
	bool m_using_input_interval{};
	bool m_show_timing{};

public:
	explicit Config(int argc, char *argv[]);
	~Config() {}

	std::string output_type_name() const {
		switch (m_output_type) {
			case ASCII:   return "ASCII";
			case UNICODE: return "Unicode";
			case BRAILLE: return "Braille";
			default: return {};
		}
	}

	size_t inputs() const { return m_inputs.size(); }

	const Input& input(size_t n) const { return m_inputs.at(n); }
	Input& input(size_t n) { return m_inputs.at(n); }

	bool using_input_interval() const { return m_using_input_interval; }
	double display_interval() const { return m_display_interval; }
	bool show_timing() const { return m_show_timing; }
};


