// Instantiates spdlog::logger::log_ the way the cli does.  Ubuntu 26.04's
// fmt 10.1 + spdlog 1.15.3 fail here with clang 21 ("call to consteval
// function 'fmt::basic_format_string...' is not a constant expression").
#include <spdlog/spdlog.h>

void blot_spdlog_probe(const char *what, const char *where, int line)
{
	spdlog::info("{} {} {}", what, where, line);
}
