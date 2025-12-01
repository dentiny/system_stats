#define DUCKDB_EXTENSION_MAIN

#include "system_stats_extension.hpp"

#include "duckdb.hpp"
#include "duckdb/common/exception.hpp"
#include "duckdb/function/table_function.hpp"
#include "duckdb/parser/parsed_data/create_table_function_info.hpp"
#include "memory_stats.hpp"

namespace duckdb {

namespace {

struct SysMemoryInfoData : public GlobalTableFunctionState {
	SysMemoryInfoData() : finished(false) {
	}
	bool finished;
};

unique_ptr<FunctionData> SysMemoryInfoBind(ClientContext &context, TableFunctionBindInput &input,
                                           vector<LogicalType> &return_types, vector<string> &names) {
	names.emplace_back("total_memory");
	return_types.emplace_back(LogicalType {LogicalTypeId::UBIGINT});

	names.emplace_back("used_memory");
	return_types.emplace_back(LogicalType {LogicalTypeId::UBIGINT});

	names.emplace_back("free_memory");
	return_types.emplace_back(LogicalType {LogicalTypeId::UBIGINT});

	names.emplace_back("cached_memory");
	return_types.emplace_back(LogicalType {LogicalTypeId::UBIGINT});

	names.emplace_back("total_swap");
	return_types.emplace_back(LogicalType {LogicalTypeId::UBIGINT});

	names.emplace_back("used_swap");
	return_types.emplace_back(LogicalType {LogicalTypeId::UBIGINT});

	names.emplace_back("free_swap");
	return_types.emplace_back(LogicalType {LogicalTypeId::UBIGINT});

	return nullptr;
}

unique_ptr<GlobalTableFunctionState> SysMemoryInfoInit(ClientContext &context, TableFunctionInitInput &input) {
	return make_uniq<SysMemoryInfoData>();
}

void SysMemoryInfoFunc(ClientContext &context, TableFunctionInput &data_p, DataChunk &output) {
	auto &data = data_p.global_state->Cast<SysMemoryInfoData>();

	if (data.finished) {
		return;
	}

	MemoryInfo info = GetMemoryInfo();

	idx_t col_idx = 0;

	// total_memory
	output.SetValue(col_idx++, 0, Value::UBIGINT(info.total_memory));

	// used_memory
	output.SetValue(col_idx++, 0, Value::UBIGINT(info.used_memory));

	// free_memory
	output.SetValue(col_idx++, 0, Value::UBIGINT(info.free_memory));

	// cached_memory
	output.SetValue(col_idx++, 0, Value::UBIGINT(info.cached_memory));

	// total_swap
	output.SetValue(col_idx++, 0, Value::UBIGINT(info.total_swap));

	// used_swap
	output.SetValue(col_idx++, 0, Value::UBIGINT(info.used_swap));

	// free_swap
	output.SetValue(col_idx++, 0, Value::UBIGINT(info.free_swap));

	output.SetCardinality(1);
	data.finished = true;
}

void LoadInternal(ExtensionLoader &loader) {
	TableFunction sys_memory_info_func("sys_memory_info", {}, SysMemoryInfoFunc, SysMemoryInfoBind, SysMemoryInfoInit);
	loader.RegisterFunction(sys_memory_info_func);
}

} // namespace

void SystemStatsExtension::Load(ExtensionLoader &loader) {
	LoadInternal(loader);
}

string SystemStatsExtension::Name() {
	return "system_stats";
}

string SystemStatsExtension::Version() const {
#ifdef EXT_VERSION_SYSTEM_STATS
	return EXT_VERSION_SYSTEM_STATS;
#else
	return "0.1.0";
#endif
}

} // namespace duckdb

extern "C" {

DUCKDB_CPP_EXTENSION_ENTRY(system_stats, loader) {
	duckdb::LoadInternal(loader);
}
}
