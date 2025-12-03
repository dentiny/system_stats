#include "string_utils.hpp"

namespace duckdb {

std::string_view TrimString(std::string_view str) {
	size_t start = str.find_first_not_of(" \t\n\r");
	if (start == std::string_view::npos) {
		return "";
	}
	size_t end = str.find_last_not_of(" \t\n\r");
	return str.substr(start, end - start + 1);
}

std::string_view RemoveQuotes(std::string_view str) {
	std::string_view result = TrimString(str);
	if (result.length() >= 2 && result.front() == '"' && result.back() == '"') {
		return result.substr(1, result.length() - 2);
	}
	return result;
}

} // namespace duckdb
