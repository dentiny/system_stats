#pragma once

#include "duckdb.hpp"
#include "duckdb/function/table_function.hpp"

namespace duckdb {

TableFunction GetSysNetworkInfoFunction();

} // namespace duckdb
