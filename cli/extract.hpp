#pragma once

#include <cstring>
#include <regex>
#include <variant>
#include <optional>
#include <charconv>

#include "blot.hpp"
#include "spdlog/spdlog.h"
#include "fmt/format.h"

class Extract final {
public:
	template <typename X, typename Y>
	struct ParseResult {
		X x;
		Y y;
	};

protected:
	using Var = std::variant<unsigned, std::pair<unsigned,unsigned>, std::regex>;
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

	// advance to fist cluster of number digits
	static const char * __find_start(const char *text, const char *end) {
		while (text < end && !(std::isdigit(*text) || *text == '.'))
			text ++;
		return text;
	}

	// skip over the next cluster of number digits
	static const char * __skip_over(const char *text, const char *end) {
		while (text < end && (std::isdigit(*text) || *text == '.'))
			text ++;
		return text;
	}

	template <typename X, typename Y>
	static std::optional<ParseResult<X,Y>> parse_y_position(size_t line, const char *text, unsigned y_position) {
		spdlog::trace("line={} text='{}' x_position={} y_position",
				line, text, y_position);

		const char *end = text+std::strlen(text);
		text = __find_start(text, end);
		auto pos = 1u ;

		while (text < end && pos < y_position) {
			text = __skip_over(text, end);
			text = __find_start(text, end);
			pos ++;
		}

		if (!y_position || y_position == pos) {
			auto [yvalue, _] = __parse<Y>(text, end);

			return ParseResult<X,Y>{X(line), yvalue};
		}

		return {};
	}

	template <typename X, typename Y>
	static std::optional<ParseResult<X,Y>> parse_xy_position(const char *text, unsigned x_position, unsigned y_position) {

		spdlog::trace("text='{}' x_position={} y_position={}",
				text, x_position, y_position);

		const char *end = text+std::strlen(text);
		text = __find_start(text, end);
		auto pos = 1u;

		auto first_position = std::min(x_position, y_position);
		auto last_position = std::max(x_position, y_position);

		while (text < end && pos < first_position) {
			text = __skip_over(text, end);
			text = __find_start(text, end);
			pos ++;
		}

		X xvalue{};
		Y yvalue{};
		unsigned have = 0;

		while (text < end && pos <= last_position) {

			const char *next = nullptr;

			if (pos == x_position) {
				auto [val, ptr] = __parse<X>(text, end);
				have ++;
				xvalue = val;
				next = ptr;
			}

			if (pos == y_position) {
				auto [val, ptr] = __parse<Y>(text, end);
				have ++;
				yvalue = val;
				next = ptr;
			}

			if (have == 2)
				return ParseResult<X,Y>{xvalue, yvalue};

			if (!next)
				next = __skip_over(text, end);

			text = __find_start(next, end);
			pos ++;
		}

		return {};
	}

	template <typename X, typename Y>
	static std::optional<ParseResult<X,Y>> parse_regex(size_t line, const char *text, const std::regex &re) {

		std::string str(text);

		spdlog::trace("line={} text='{}'", line, text);

		std::smatch matches;
		if (!std::regex_search(str, matches, re))
			return {};

		spdlog::trace("  matches={}", matches.size());

		switch (matches.size()) {
			case 0:
				return {};
			case 1: {
				const char *start = text + matches.position(1);
				const char *end = start + matches.length(1);
				auto [value, _] = __parse<Y>(start, end);

				return ParseResult<X,Y>{X(line), value};
			}
			default: {
				const char *xstart = text + matches.position(1);
				const char *xend = xstart + matches.length(1);
				auto [xvalue, _1] = __parse<X>(xstart, xend);

				const char *ystart = text + matches.position(1);
				const char *yend = ystart + matches.length(1);
				auto [yvalue, _2] = __parse<Y>(ystart, yend);

				return ParseResult<X,Y>{xvalue, yvalue};
			}
		}
	}



public:
	void set(const auto &thing) {
		m_var = thing;
	}

	template <typename X, typename Y>
	std::optional<ParseResult<X,Y>> parse(size_t line, const char *text) const {

		if (std::holds_alternative<std::regex>(m_var)) {
			auto &re = std::get<std::regex>(m_var);
			return parse_regex<X,Y>(line, text, re);
		}

		if (std::holds_alternative<std::pair<unsigned,unsigned>>(m_var)) {
			auto [x,y] = std::get<std::pair<unsigned,unsigned>>(m_var);
			return parse_xy_position<X,Y>(text, x, y);
		}

		if (std::holds_alternative<unsigned>(m_var)) {
			auto y = std::get<unsigned>(m_var);
			return parse_y_position<X,Y>(line, text, y);
		}

		BLOT_THROW(EINVAL,"internal error");
	}
};

