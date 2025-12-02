#pragma once

#include "duckdb.hpp"

namespace duckdb {

// Register sys_memory_info table function
void RegisterSysMemoryInfoFunction(ExtensionLoader &loader);

} // namespace duckdb
