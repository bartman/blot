#pragma once

#include <cstring>
#include <array>
#include <regex>
#include <variant>
#include <limits>
#include <charconv>

#include "spdlog/spdlog.h"
#include "fmt/format.h"

class Extract {
protected:
	using Var = std::variant<unsigned, std::regex>;
	Var m_var;

	template <typename T>
	static std::pair<T,const char*> __parse(const char *text, const char *end) {
		T value;
		auto [ptr,ec] = std::from_chars(text, end, value);
		if (ec != std::errc{}) {
			spdlog::error("failed to parse number({}) from '{}': {}",
				typeid(T).name(), text,
				std::make_error_code(ec).message());
			std::exit(1);
		}
		return {value, ptr};
	}

	static const char * __advance(const char *text, const char *end) {
		while (text < end && !(std::isdigit(*text) || *text == '.'))
			text ++;
		return text;
	}

public:
	void set(const auto &thing) {
		m_var = thing;
	}

	template <typename T, size_t N>
	struct ParseResult {
		std::array<T,N> array;
		size_t count{};
	};

	template <typename T, size_t N>
	ParseResult<T,N> parse(const char *text) const {
		ParseResult<T,N> result;

		if (std::holds_alternative<std::regex>(m_var)) {

			std::string str(text);
			auto &re = std::get<std::regex>(m_var);

			spdlog::trace("text={}", text);

			std::smatch matches;
			if (!std::regex_search(str, matches, re))
				return result;

			spdlog::trace("  matches={}", matches.size());

			for (size_t i = 1; i < matches.size(); ++i) {
				const char *start = text + matches.position(i);
				const char *end = start + matches.length(i);
				auto [value, _] = __parse<T>(start, end);
				result.array[i-1] = value;
				spdlog::trace("  array[{}] = {}", i-1, value);
			}
			result.count = matches.size() - 1;

			return result;
		}

		unsigned start_position = 0;
		if (std::holds_alternative<unsigned>(m_var))
			start_position = std::get<unsigned>(m_var);

		spdlog::trace("text={} position={}", text, start_position);

		const char *end = text+std::strlen(text);
		text = __advance(text, end);

		unsigned n=0;
		while (text<end && result.count<N) {
			auto [value, ptr] = __parse<T>(text, end);
			if (n >= start_position) {
				spdlog::trace("  array[{}] = {}", result.count, value);
				result.array[result.count++] = value;
			}
			++n;
			text = __advance(ptr, end);
		}

		return result;
	}

};

