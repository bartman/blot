#pragma once

#include <cstdint>
#include <vector>
#include <string>

#include "blot.hpp"
#include "spdlog/spdlog.h"

struct Input final {
	enum Source {NONE,READ,FOLLOW,EXEC,WATCH};

	blot_plot_type m_plot_type;
	Source m_source;
	std::string m_details;
	blot_color m_plot_color{9};

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

	operator bool() const {
		switch (m_plot_type) {
			case BLOT_SCATTER:
			case BLOT_LINE:
			case BLOT_BAR:
				break;
			default:
				spdlog::warn("invalid plot type: {}", (int)m_plot_type);
				return false;
		}
		if (m_source == NONE) {
			spdlog::warn("{} plot does not defined an input (file or command)",
				blot_plot_type_to_string(m_plot_type));
			return false;
		}
		if (m_details.empty()) {
			spdlog::warn("{} plot has empty {} argument",
				blot_plot_type_to_string(m_plot_type), source_name());
			return false;
		}

		return true;
	}

	void set_source (Input::Source source, const std::string &details) {
		m_source = source;
		m_details = details;
	};
	void set_color (const std::string &c) {
		m_plot_color = std::strtol(c.c_str(), NULL, 10);
	};
};

struct Config {
	const char *m_self{};
	enum output_type { ASCII, UNICODE, BRAILLE } m_output_type;
	std::vector<Input> m_inputs;
	double m_interval{0};

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


