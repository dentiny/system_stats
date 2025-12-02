#pragma once

#include "duckdb.hpp"
#include "duckdb/function/table_function.hpp"

namespace duckdb {

// Register sys_network_info table function
void RegisterSysNetworkInfoFunction(ExtensionLoader &loader);

} // namespace duckdb
