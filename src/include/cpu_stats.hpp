#pragma once

#include "duckdb/common/string.hpp"
#include "duckdb/common/types.hpp"

namespace duckdb {
class ClientContext;

struct CPUInfo {
	// Basic CPU info
	string model_name;
	string architecture;

	// Core counts
	int32_t logical_cpus = 0;
	int32_t physical_cpus = 0;

	// Cache sizes (in KB)
	int32_t l1d_cache_kb = 0;
	int32_t l1i_cache_kb = 0;
	int32_t l2_cache_kb = 0;
	int32_t l3_cache_kb = 0;

	// Platform-specific
	string byte_order;
};

// Get CPU information for the current platform
CPUInfo GetCPUInfo(ClientContext &context);

} // namespace duckdb
