#pragma once

#include <cstdint>
#include <vector>
#include <string>

#include "blot.hpp"
#include "spdlog/spdlog.h"

struct Input final {
	enum Source {
		NONE,		// not yet initialized
		READ,		// read from a file, each line is an entry
		FOLLOW,		// like READ, but wait for more data
		POLL,		// read a file repetitively at interval, each read is an entry
		EXEC,		// run program, read from stdout, each line is an entry
		WATCH		// run program repetitively at interval, each run is one entry
	};

	blot_plot_type m_plot_type{BLOT_LINE};
	Source m_source{NONE};
	std::string m_details{};
	blot_color m_plot_color{9};
	double m_interval{0};

	explicit Input(blot_plot_type plot_type) : m_plot_type(plot_type) {}
	~Input() {}

	std::string plot_name() const {
		return blot_plot_type_to_string(m_plot_type);
	}

	std::string source_name() const {
		switch (m_source) {
			case READ: return "read";
			case FOLLOW: return "follow";
			case EXEC: return "exec";
			case WATCH: return "watch";
			default: return {};
		}
	}

	/* validate if the configuration looks sane */
	operator bool() const;

	void set_source (Input::Source source, const std::string &details);
	void set_color (const std::string &txt);
	void set_interval (const std::string &txt);

};

struct Config {
	const char *m_self{};
	enum output_type { ASCII, UNICODE, BRAILLE } m_output_type;
	std::vector<Input> m_inputs;
	bool m_using_interval{};

	Config(int argc, char *argv[]);
	~Config() {}

	std::string output_type_name() const {
		switch (m_output_type) {
			case ASCII:   return "ASCII";
			case UNICODE: return "Unicode";
			case BRAILLE: return "Braille";
			default: return {};
		}
	}
};


