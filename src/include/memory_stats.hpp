#pragma once

#include "duckdb/common/types.hpp"

namespace duckdb {
class ClientContext;

struct MemoryInfo {
	uint64_t total_memory = 0;
	uint64_t used_memory = 0;
	uint64_t free_memory = 0;
	uint64_t total_swap = 0;
	uint64_t used_swap = 0;
	uint64_t free_swap = 0;
	uint64_t cached_memory = 0;
};

// Get memory information for the current platform
MemoryInfo GetMemoryInfo(ClientContext &context);

} // namespace duckdb
