#pragma once

#include "duckdb/common/string.hpp"
#include "duckdb/common/vector.hpp"
#include "duckdb/common/types.hpp"

namespace duckdb {
class ClientContext;

struct OSInfo {
	std::string name;
	std::string version;
	std::string host_name;
	int32_t handle_count = 0;
	int32_t process_count = 0;
	int32_t thread_count = 0;
	std::string architecture;
	uint64_t os_up_since_seconds = 0;
};

// Get OS information for the current platform
OSInfo GetOSInfo(ClientContext &context);

} // namespace duckdb
