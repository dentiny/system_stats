#include "cpu_stats_query_function.hpp"

#include "cpu_stats.hpp"
#include "duckdb/common/vector_size.hpp"
#include "duckdb/function/table_function.hpp"

namespace duckdb {

namespace {

struct SysCPUInfoData : public GlobalTableFunctionState {
	SysCPUInfoData() : finished(false) {
	}
	bool finished;
};

unique_ptr<FunctionData> SysCPUInfoBind(ClientContext &context, TableFunctionBindInput &input,
                                        vector<LogicalType> &return_types, vector<string> &names) {
	names.emplace_back("model_name");
	return_types.emplace_back(LogicalType {LogicalTypeId::VARCHAR});

	names.emplace_back("architecture");
	return_types.emplace_back(LogicalType {LogicalTypeId::VARCHAR});

	names.emplace_back("logical_processor");
	return_types.emplace_back(LogicalType {LogicalTypeId::INTEGER});

	names.emplace_back("physical_processor");
	return_types.emplace_back(LogicalType {LogicalTypeId::INTEGER});

	names.emplace_back("cpu_clock_speed_Hz");
	return_types.emplace_back(LogicalType {LogicalTypeId::UBIGINT});

	names.emplace_back("l1dcache_size_KiB");
	return_types.emplace_back(LogicalType {LogicalTypeId::INTEGER});

	names.emplace_back("l1icache_size_KiB");
	return_types.emplace_back(LogicalType {LogicalTypeId::INTEGER});

	names.emplace_back("l2cache_size_KiB");
	return_types.emplace_back(LogicalType {LogicalTypeId::INTEGER});

	names.emplace_back("l3cache_size_KiB");
	return_types.emplace_back(LogicalType {LogicalTypeId::INTEGER});

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

	// architecture
	output.SetValue(col_idx++, 0, Value(info.architecture));

	// logical_processor
	output.SetValue(col_idx++, 0, Value::INTEGER(info.logical_cpus));

	// physical_processor
	output.SetValue(col_idx++, 0, Value::INTEGER(info.physical_cpus));

	// cpu_clock_speed_Hz
	output.SetValue(col_idx++, 0, Value::UBIGINT(info.cpu_frequency_hz));

	// l1dcache_size_KiB
	output.SetValue(col_idx++, 0, Value::INTEGER(info.l1d_cache_kb));

	// l1icache_size_KiB
	output.SetValue(col_idx++, 0, Value::INTEGER(info.l1i_cache_kb));

	// l2cache_size_KiB
	output.SetValue(col_idx++, 0, Value::INTEGER(info.l2_cache_kb));

	// l3cache_size_KiB
	output.SetValue(col_idx++, 0, Value::INTEGER(info.l3_cache_kb));

	// cpu_byte_order
	output.SetValue(col_idx++, 0, Value(info.byte_order));

	output.SetCardinality(1);
	data.finished = true;
}

} // namespace

void RegisterSysCPUInfoFunction(ExtensionLoader &loader) {
	TableFunction sys_cpu_info_func("sys_cpu_info", {}, SysCPUInfoFunc, SysCPUInfoBind, SysCPUInfoInit);
	loader.RegisterFunction(sys_cpu_info_func);
}

} // namespace duckdb
