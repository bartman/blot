#pragma once

#include "config.hpp"

#include <string>
#include <optional>
#include <memory>

struct Line final {
	size_t number;
	std::string text;

	explicit Line(size_t n, std::string t) : number(n), text(t) {}
};

class Reader {
public:
	virtual ~Reader() {}

	virtual bool fail() const = 0;
	virtual bool eof() const = 0;
	virtual operator bool() const = 0;
	virtual double idle() const = 0;
	virtual size_t lines() const = 0;
	virtual std::optional<Line> line() = 0;

	static std::unique_ptr<Reader> from(const Input &input);
};

