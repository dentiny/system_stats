#pragma once

#include "duckdb.hpp"
#include "duckdb/function/table_function.hpp"
#include "memory_unit_util.hpp"

namespace duckdb {

// FunctionData to store unit preference for memory info
struct SysMemoryInfoBindData : public FunctionData {
	MemoryUnit unit = MemoryUnit::BYTES;

	bool Equals(const FunctionData &other_p) const override;
	unique_ptr<FunctionData> Copy() const override;
};

// Register sys_memory_info table function
void RegisterSysMemoryInfoFunction(ExtensionLoader &loader);

} // namespace duckdb
