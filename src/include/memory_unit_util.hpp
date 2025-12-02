#pragma once

#include <cstdint>

#include "duckdb/common/string.hpp"
#include "duckdb/common/string_util.hpp"

namespace duckdb {

enum class MemoryUnit {
	BYTES, // Default
	KB,    // 1000 bytes
	KiB,   // 1024 bytes
	MB,    // 1000^2 bytes
	MiB,   // 1024^2 bytes
	GB,    // 1000^3 bytes
	GiB,   // 1024^3 bytes
	TB,    // 1000^4 bytes
	TiB    // 1024^4 bytes
};

// Util function to convert bytes to specified unit
uint64_t ConvertBytes(uint64_t bytes, MemoryUnit unit);

// Util function to parse unit string
MemoryUnit ParseUnit(const string &unit_str);

} // namespace duckdb
