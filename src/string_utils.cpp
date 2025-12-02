#include "string_utils.hpp"

namespace duckdb {

string TrimString(const string &str) {
	size_t start = str.find_first_not_of(" \t\n\r");
	if (start == string::npos) {
		return "";
	}
	size_t end = str.find_last_not_of(" \t\n\r");
	return str.substr(start, end - start + 1);
}

string RemoveQuotes(const string &str) {
	string result = TrimString(str);
	if (result.length() >= 2 && result.front() == '"' && result.back() == '"') {
		return result.substr(1, result.length() - 2);
	}
	return result;
}

} // namespace duckdb
