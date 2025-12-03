#pragma once

#include <string_view>

namespace duckdb {

// Trim whitespace from both ends of a string
std::string_view TrimString(std::string_view str);

// Remove quotes from a string (after trimming)
std::string_view RemoveQuotes(std::string_view str);

} // namespace duckdb
