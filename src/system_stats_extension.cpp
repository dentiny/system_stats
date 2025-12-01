#define DUCKDB_EXTENSION_MAIN

#include "system_stats_extension.hpp"

#include "duckdb.hpp"
#include "duckdb/common/exception.hpp"
#include "duckdb/function/table_function.hpp"
#include "duckdb/parser/parsed_data/create_table_function_info.hpp"
#include "memory_stats.hpp"
#include "cpu_stats.hpp"

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

// CPU Info Function
struct SysCPUInfoData : public GlobalTableFunctionState {
	SysCPUInfoData() : finished(false) {
	}
	bool finished;
};

unique_ptr<FunctionData> SysCPUInfoBind(ClientContext &context, TableFunctionBindInput &input,
                                        vector<LogicalType> &return_types, vector<string> &names) {
	// Match PostgreSQL system_stats extension column names
	names.emplace_back("model_name");
	return_types.emplace_back(LogicalType {LogicalTypeId::VARCHAR});

	names.emplace_back("cpu_vendor");
	return_types.emplace_back(LogicalType {LogicalTypeId::VARCHAR});

	names.emplace_back("cpu_description");
	return_types.emplace_back(LogicalType {LogicalTypeId::VARCHAR});

	names.emplace_back("processor_type");
	return_types.emplace_back(LogicalType {LogicalTypeId::VARCHAR});

	names.emplace_back("architecture");
	return_types.emplace_back(LogicalType {LogicalTypeId::VARCHAR});

	names.emplace_back("logical_processor");
	return_types.emplace_back(LogicalType {LogicalTypeId::INTEGER});

	names.emplace_back("physical_processor");
	return_types.emplace_back(LogicalType {LogicalTypeId::INTEGER});

	names.emplace_back("no_of_cores");
	return_types.emplace_back(LogicalType {LogicalTypeId::INTEGER});

	names.emplace_back("cpu_clock_speed");
	return_types.emplace_back(LogicalType {LogicalTypeId::UBIGINT});

	names.emplace_back("l1dcache_size");
	return_types.emplace_back(LogicalType {LogicalTypeId::INTEGER});

	names.emplace_back("l1icache_size");
	return_types.emplace_back(LogicalType {LogicalTypeId::INTEGER});

	names.emplace_back("l2cache_size");
	return_types.emplace_back(LogicalType {LogicalTypeId::INTEGER});

	names.emplace_back("l3cache_size");
	return_types.emplace_back(LogicalType {LogicalTypeId::INTEGER});

	names.emplace_back("cpu_family");
	return_types.emplace_back(LogicalType {LogicalTypeId::VARCHAR});

	names.emplace_back("cpu_type");
	return_types.emplace_back(LogicalType {LogicalTypeId::VARCHAR});

	names.emplace_back("cpu_byte_order");
	return_types.emplace_back(LogicalType {LogicalTypeId::VARCHAR});

	return nullptr;
}

unique_ptr<GlobalTableFunctionState> SysCPUInfoInit(ClientContext &context, TableFunctionInitInput &input) {
	return make_uniq<SysCPUInfoData>();
}

void SysCPUInfoFunc(ClientContext &context, TableFunctionInput &data_p, DataChunk &output) {
	auto &data = data_p.global_state->Cast<SysCPUInfoData>();

	if (data.finished) {
		return;
	}

	CPUInfo info = GetCPUInfo();

	idx_t col_idx = 0;

	// model_name
	output.SetValue(col_idx++, 0, Value(info.model_name));

	// cpu_vendor (was vendor_id)
	output.SetValue(col_idx++, 0, Value(info.vendor_id));

	// cpu_description (empty for now - PostgreSQL uses this for additional CPU description)
	output.SetValue(col_idx++, 0, Value(""));

	// processor_type (empty for now - PostgreSQL uses this for processor type details)
	output.SetValue(col_idx++, 0, Value(""));

	// architecture
	output.SetValue(col_idx++, 0, Value(info.architecture));

	// logical_processor (was logical_cpus)
	output.SetValue(col_idx++, 0, Value::INTEGER(info.logical_cpus));

	// physical_processor (was physical_cpus)
	output.SetValue(col_idx++, 0, Value::INTEGER(info.physical_cpus));

	// no_of_cores (was num_cores)
	output.SetValue(col_idx++, 0, Value::INTEGER(info.num_cores));

	// cpu_clock_speed (was cpu_frequency_hz)
	output.SetValue(col_idx++, 0, Value::UBIGINT(info.cpu_frequency_hz));

	// l1dcache_size (was l1d_cache_kb)
	output.SetValue(col_idx++, 0, Value::INTEGER(info.l1d_cache_kb));

	// l1icache_size (was l1i_cache_kb)
	output.SetValue(col_idx++, 0, Value::INTEGER(info.l1i_cache_kb));

	// l2cache_size (was l2_cache_kb)
	output.SetValue(col_idx++, 0, Value::INTEGER(info.l2_cache_kb));

	// l3cache_size (was l3_cache_kb)
	output.SetValue(col_idx++, 0, Value::INTEGER(info.l3_cache_kb));

	// cpu_family
	output.SetValue(col_idx++, 0, Value(info.cpu_family));

	// cpu_type
	output.SetValue(col_idx++, 0, Value(info.cpu_type));

	// cpu_byte_order (was byte_order)
	output.SetValue(col_idx++, 0, Value(info.byte_order));

	output.SetCardinality(1);
	data.finished = true;
}

void LoadInternal(ExtensionLoader &loader) {
	// Register sys_memory_info table function
	TableFunction sys_memory_info_func("sys_memory_info", {}, SysMemoryInfoFunc, SysMemoryInfoBind, SysMemoryInfoInit);
	loader.RegisterFunction(sys_memory_info_func);

	// Register sys_cpu_info table function
	TableFunction sys_cpu_info_func("sys_cpu_info", {}, SysCPUInfoFunc, SysCPUInfoBind, SysCPUInfoInit);
	loader.RegisterFunction(sys_cpu_info_func);
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
