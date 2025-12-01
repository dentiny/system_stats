#define DUCKDB_EXTENSION_MAIN

#include "system_stats_extension.hpp"
#include "memory_stats.hpp"
#include "duckdb.hpp"
#include "duckdb/common/exception.hpp"
#include "duckdb/function/table_function.hpp"
#include "duckdb/parser/parsed_data/create_table_function_info.hpp"

namespace duckdb {

struct SysMemoryInfoData : public GlobalTableFunctionState {
	SysMemoryInfoData() : finished(false) {
	}
	bool finished;
};

static unique_ptr<FunctionData> SysMemoryInfoBind(ClientContext &context, TableFunctionBindInput &input,
                                                  vector<LogicalType> &return_types, vector<string> &names) {
	names.emplace_back("total_memory");
	return_types.emplace_back(LogicalType {LogicalTypeId::UBIGINT});

	names.emplace_back("used_memory");
	return_types.emplace_back(LogicalType {LogicalTypeId::UBIGINT});

	names.emplace_back("free_memory");
	return_types.emplace_back(LogicalType {LogicalTypeId::UBIGINT});

	names.emplace_back("available_memory");
	return_types.emplace_back(LogicalType {LogicalTypeId::UBIGINT});

	names.emplace_back("cached_memory");
	return_types.emplace_back(LogicalType {LogicalTypeId::UBIGINT});

	names.emplace_back("buffers_memory");
	return_types.emplace_back(LogicalType {LogicalTypeId::UBIGINT});

	names.emplace_back("total_swap");
	return_types.emplace_back(LogicalType {LogicalTypeId::UBIGINT});

	names.emplace_back("used_swap");
	return_types.emplace_back(LogicalType {LogicalTypeId::UBIGINT});

	names.emplace_back("free_swap");
	return_types.emplace_back(LogicalType {LogicalTypeId::UBIGINT});

	return nullptr;
}

static unique_ptr<GlobalTableFunctionState> SysMemoryInfoInit(ClientContext &context, TableFunctionInitInput &input) {
	return make_uniq<SysMemoryInfoData>();
}

static void SysMemoryInfoFunc(ClientContext &context, TableFunctionInput &data_p, DataChunk &output) {
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

	// available_memory
	output.SetValue(col_idx++, 0, Value::UBIGINT(info.available_memory));

	// cached_memory
	output.SetValue(col_idx++, 0, Value::UBIGINT(info.cached_memory));

	// buffers_memory
	output.SetValue(col_idx++, 0, Value::UBIGINT(info.buffers_memory));

	// total_swap
	output.SetValue(col_idx++, 0, Value::UBIGINT(info.total_swap));

	// used_swap
	output.SetValue(col_idx++, 0, Value::UBIGINT(info.used_swap));

	// free_swap
	output.SetValue(col_idx++, 0, Value::UBIGINT(info.free_swap));

	output.SetCardinality(1);
	data.finished = true;
}

static void LoadInternal(ExtensionLoader &loader) {
	TableFunction sys_memory_info_func("sys_memory_info", {}, SysMemoryInfoFunc, SysMemoryInfoBind, SysMemoryInfoInit);
	loader.RegisterFunction(sys_memory_info_func);
}

void SystemStatsExtension::Load(ExtensionLoader &loader) {
	LoadInternal(loader);
}

std::string SystemStatsExtension::Name() {
	return "system_stats";
}

std::string SystemStatsExtension::Version() const {
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
