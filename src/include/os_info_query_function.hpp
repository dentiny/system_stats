#pragma once

#include "duckdb.hpp"

namespace duckdb {

// Register sys_os_info table function
void RegisterSysOSInfoFunction(ExtensionLoader &loader);

} // namespace duckdb
