#pragma once

#include "config.hpp"

#include <string>
#include <optional>
#include <memory>

struct Reader {
	virtual ~Reader() {}

	virtual bool fail() const = 0;
	virtual bool eof() const = 0;
	virtual operator bool() const = 0;
	virtual std::optional<std::string> line() = 0;

	static std::unique_ptr<Reader> from(const Input &input);
};

