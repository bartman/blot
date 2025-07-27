#include "reader.hpp"
#include "config.hpp"

#include <fstream>
#include <filesystem>

#include <cstdio>    // for popen, pclose
#include <unistd.h>  // for read, fileno
#include <fcntl.h>   // for fcntl, O_NONBLOCK
#include <cerrno>    // for errno
#include <cstring>   // for strerror

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
		if (fs::is_character_file(m_path) || fs::is_fifo(m_path)) {
			if (follow) {
				spdlog::error("{}: fifo/chardev cannot be used with follow", m_path.string());
				std::exit(1);
			}
		} else if (!fs::is_regular_file(m_path)) {
			spdlog::error("{}: is not a file/fifo/chardev", m_path.string());
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

struct ExecStreamReader : public Reader {
	std::string m_command;
	FILE* m_pipe = nullptr;
	int m_fd = -1;
	std::string m_buffer;
	bool m_eof = false;
	bool m_fail = false;

	ExecStreamReader(const std::string& command) : m_command(command) {
		m_pipe = popen(m_command.c_str(), "r");
		if (!m_pipe) {
			spdlog::error("failed to execute command '{}': {}", m_command, std::strerror(errno));
			std::exit(1);
		}

		m_fd = fileno(m_pipe);
		int flags = fcntl(m_fd, F_GETFL, 0);
		if (flags == -1 || fcntl(m_fd, F_SETFL, flags | O_NONBLOCK) == -1) {
			spdlog::error("failed to set non-blocking mode for command '{}': {}", m_command, std::strerror(errno));
			std::exit(1);
		}
	}

	~ExecStreamReader() override {
		if (m_pipe) {
			pclose(m_pipe);
		}
	}

	bool fail() const override {
		return m_fail;
	}

	bool eof() const override {
		return m_eof;
	}

	operator bool() const override {
		return !m_fail && !m_eof;
	}

	std::optional<std::string> line() override {
		if (m_fail || m_eof) {
			return {};
		}

		// Extract a complete line from the buffer if available
		while (true) {
			size_t pos = m_buffer.find('\n');
			if (pos != std::string::npos) {
				std::string l = m_buffer.substr(0, pos);
				m_buffer.erase(0, pos + 1);
				spdlog::debug("{}: {}", m_command, l);
				return l;
			}

			// Read more data non-blockingly
			char buf[4096];
			ssize_t bytes_read = read(m_fd, buf, sizeof(buf));
			if (bytes_read > 0) {
				m_buffer.append(buf, bytes_read);
				continue;  // Check for a line again
			} else if (bytes_read == 0) {
				// EOF: subprocess exited
				m_eof = true;
				spdlog::trace("command '{}' reached EOF", m_command);
				if (!m_buffer.empty()) {
					// Return any remaining data as the last (unterminated) line
					std::string l = std::move(m_buffer);
					m_buffer.clear();
					spdlog::debug("{}: {}", m_command, l);
					return l;
				}
				return {};
			} else {
				// Read error
				if (errno == EAGAIN || errno == EWOULDBLOCK) {
					// No data available yet (async/non-blocking behavior)
					return {};
				} else {
					spdlog::warn("read error for command '{}': {}", m_command, std::strerror(errno));
					m_fail = true;
					return {};
				}
			}
		}
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
			return std::make_unique<ExecStreamReader>(input.m_details);
		case Input::WATCH:
			spdlog::error("input source value {} not yet supported",
					(int)input.m_source);
			std::terminate();

		default:
			spdlog::error("invalid input source value {}", (int)input.m_source);
			std::terminate();
	}
}
