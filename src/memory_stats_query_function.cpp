#include "memory_stats_query_function.hpp"

#include "duckdb/common/vector_size.hpp"
#include "duckdb/function/table_function.hpp"
#include "memory_stats.hpp"
#include "memory_unit_util.hpp"

namespace duckdb {

namespace {

struct SysMemoryInfoBindData : public FunctionData {
	MemoryUnit unit = MemoryUnit::BYTES;

	bool Equals(const FunctionData &other_p) const override {
		auto &other = other_p.Cast<SysMemoryInfoBindData>();
		return unit == other.unit;
	}

	unique_ptr<FunctionData> Copy() const override {
		auto result = make_uniq<SysMemoryInfoBindData>();
		result->unit = unit;
		return std::move(result);
	}
};

struct SysMemoryInfoData : public GlobalTableFunctionState {
	SysMemoryInfoData() : finished(false) {
	}
	bool finished;
};

unique_ptr<FunctionData> SysMemoryInfoBind(ClientContext &context, TableFunctionBindInput &input,
                                           vector<LogicalType> &return_types, vector<string> &names) {
	auto result = make_uniq<SysMemoryInfoBindData>();

	// Parse unit parameter if provided
	auto unit_it = input.named_parameters.find("unit");
	if (unit_it != input.named_parameters.end()) {
		result->unit = ParseUnit(unit_it->second.ToString());
	}

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

	return std::move(result);
}

unique_ptr<GlobalTableFunctionState> SysMemoryInfoInit(ClientContext &context, TableFunctionInitInput &input) {
	return make_uniq<SysMemoryInfoData>();
}

void SysMemoryInfoFunc(ClientContext &context, TableFunctionInput &data_p, DataChunk &output) {
	auto &data = data_p.global_state->Cast<SysMemoryInfoData>();
	auto &bind_data = data_p.bind_data->Cast<SysMemoryInfoBindData>();

	if (data.finished) {
		return;
	}

	MemoryInfo info = GetMemoryInfo();

	idx_t col_idx = 0;

	// Apply unit conversion
	// total_memory
	output.SetValue(col_idx++, 0, Value::UBIGINT(ConvertBytes(info.total_memory, bind_data.unit)));

	// used_memory
	output.SetValue(col_idx++, 0, Value::UBIGINT(ConvertBytes(info.used_memory, bind_data.unit)));

	// free_memory
	output.SetValue(col_idx++, 0, Value::UBIGINT(ConvertBytes(info.free_memory, bind_data.unit)));

	// cached_memory
	output.SetValue(col_idx++, 0, Value::UBIGINT(ConvertBytes(info.cached_memory, bind_data.unit)));

	// total_swap
	output.SetValue(col_idx++, 0, Value::UBIGINT(ConvertBytes(info.total_swap, bind_data.unit)));

	// used_swap
	output.SetValue(col_idx++, 0, Value::UBIGINT(ConvertBytes(info.used_swap, bind_data.unit)));

	// free_swap
	output.SetValue(col_idx++, 0, Value::UBIGINT(ConvertBytes(info.free_swap, bind_data.unit)));

	output.SetCardinality(1);
	data.finished = true;
}

} // namespace

void RegisterSysMemoryInfoFunction(ExtensionLoader &loader) {
	TableFunction sys_memory_info_func("sys_memory_info", {}, SysMemoryInfoFunc, SysMemoryInfoBind, SysMemoryInfoInit);
	sys_memory_info_func.named_parameters["unit"] = LogicalType::VARCHAR;
	loader.RegisterFunction(sys_memory_info_func);
}

} // namespace duckdb
