#pragma once

#include "duckdb/common/string.hpp"

namespace duckdb {

// Trim whitespace from both ends of a string
string TrimString(const string &str);

// Remove quotes from a string (after trimming)
string RemoveQuotes(const string &str);

} // namespace duckdb
