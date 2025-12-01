#pragma once

#include "duckdb/common/string.hpp"
#include <cstdint>
#include <vector>

namespace duckdb {

struct DiskInfo {
	string mount_point;
	string file_system;
	string drive_letter; // NULL on Linux
	string drive_type;   // NULL on Linux
	string file_system_type;
	uint64_t total_space = 0;
	uint64_t used_space = 0;
	uint64_t free_space = 0;
	uint64_t total_inodes = 0;
	uint64_t used_inodes = 0;
	uint64_t free_inodes = 0;
};

// Get disk information for the current platform
std::vector<DiskInfo> GetDiskInfo();

} // namespace duckdb
