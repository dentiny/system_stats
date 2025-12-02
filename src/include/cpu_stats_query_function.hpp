#pragma once

#include "duckdb.hpp"
#include "duckdb/function/table_function.hpp"

namespace duckdb {

// Register sys_cpu_info table function
void RegisterSysCPUInfoFunction(ExtensionLoader &loader);

} // namespace duckdb
