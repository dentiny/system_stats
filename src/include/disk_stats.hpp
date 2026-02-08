#pragma once

#include "duckdb/common/string.hpp"
#include "duckdb/common/types.hpp"
#include "duckdb/common/vector.hpp"

namespace duckdb {
class ClientContext;

struct DiskInfo {
	string mount_point;
	string file_system;
	string file_system_type;
	uint64_t total_space = 0;
	uint64_t used_space = 0;
	uint64_t free_space = 0;
};

// Get disk information for the current platform
std::vector<DiskInfo> GetDiskInfo(ClientContext &context);

} // namespace duckdb
