#pragma once

#include "duckdb.hpp"
#include "duckdb/function/table_function.hpp"
#include "memory_unit_util.hpp"

namespace duckdb {

// FunctionData to store unit preference for disk info
struct SysDiskInfoBindData : public FunctionData {
	MemoryUnit unit = MemoryUnit::BYTES;

	bool Equals(const FunctionData &other_p) const override;
	unique_ptr<FunctionData> Copy() const override;
};

// Register sys_disk_info table function
void RegisterSysDiskInfoFunction(ExtensionLoader &loader);

} // namespace duckdb
