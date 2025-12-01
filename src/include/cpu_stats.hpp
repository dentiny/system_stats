#pragma once

#include "duckdb/common/string.hpp"
#include <cstdint>

namespace duckdb {

struct CPUInfo {
	// Basic CPU info
	string model_name;
	string architecture;
	string vendor_id;
	string cpu_description;
	string processor_type;

	// Core counts
	int32_t logical_cpus = 0;
	int32_t physical_cpus = 0;
	int32_t num_cores = 0;

	// Clock speed
	uint64_t cpu_frequency_hz = 0;

	// Cache sizes (in KB)
	int32_t l1d_cache_kb = 0;
	int32_t l1i_cache_kb = 0;
	int32_t l2_cache_kb = 0;
	int32_t l3_cache_kb = 0;

	// Platform-specific
	string cpu_family;
	string cpu_type;
	string byte_order;
};

// Get CPU information for the current platform
CPUInfo GetCPUInfo();

} // namespace duckdb
