#pragma once

#include "duckdb.hpp"

namespace duckdb {

// Register sys_disk_info table function
void RegisterSysDiskInfoFunction(ExtensionLoader &loader);

} // namespace duckdb
