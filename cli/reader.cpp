#include "reader.hpp"
#include "config.hpp"

#include <fstream>
#include <filesystem>

#include "spdlog/spdlog.h"

namespace fs = std::filesystem;

struct FileReader : public Reader {

	fs::path m_path;
	bool m_follow{};
	std::ifstream m_stream;
	size_t m_line_number{};

	FileReader(const std::string &details, bool follow = false)
	: m_path(details), m_follow(follow)
	{
		if (!fs::exists(m_path)) {
			spdlog::error("{}: does not exist", m_path.string());
			std::exit(1);
		}
		if (!fs::is_regular_file(m_path)) {
			spdlog::error("{}: is not a file", m_path.string());
			std::exit(1);
		}
		m_stream = std::ifstream(m_path);
		if (!m_stream.is_open()) {
			spdlog::error("{}: failed to open", m_path.string());
			std::exit(1);
		}
	}
	~FileReader() override {}

	bool fail() const override { return m_stream.fail(); }
	bool eof() const override { return m_stream.eof(); }
	operator bool() const override {
		return !fail() && (!eof() || m_follow);
	}

	std::optional<std::string> line() override
	{
		if (m_stream.fail())
			// reached failed state
			return {};

		// TODO: may need to implement --line-buffered behaviour
		std::string line;
		if (std::getline(m_stream, line)) {
			m_line_number ++;
			// successfully read next line
			spdlog::debug("{}:{}: {}", m_path.string(), m_line_number, line);
			return line;
		}

		spdlog::trace("{}:{}: follow={} fail={} eof={}", m_path.string(), m_line_number,
				m_follow, m_stream.fail(), m_stream.eof());

		if (m_stream.eof()) {
			// reached the end of file
			if (m_follow)
				m_stream.clear();
			spdlog::trace("{}:{}: reached end of file", m_path.string(), m_line_number);
			return {};
		}

		if (!m_stream.fail()) {
			// this will confuse the eof()/fail() logic -- should not happen
			spdlog::error("{}: no output, fail={} eof={}",
				m_path.string(), m_stream.fail(), m_stream.eof());
			std::terminate();
		}

		// some reading failure
		spdlog::warn("{}: failed reading", m_path.string());
		return {};
	}
};

std::unique_ptr<Reader> Reader::from(const Input &input)
{
	switch (input.m_source) {
		case Input::READ:
			return std::make_unique<FileReader>(input.m_details);
		case Input::FOLLOW:
			return std::make_unique<FileReader>(input.m_details, true);
		case Input::EXEC:
		case Input::WATCH:
			spdlog::error("input source value {} not yet supported",
					(int)input.m_source);
			std::terminate();

		default:
			spdlog::error("invalid input source value {}", (int)input.m_source);
			std::terminate();
	}
}
