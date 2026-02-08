#include "disk_stats_query_function.hpp"

#include "disk_stats.hpp"
#include "duckdb/common/assert.hpp"
#include "duckdb/common/vector_size.hpp"
#include "duckdb/function/table_function.hpp"
#include "memory_unit_util.hpp"

namespace duckdb {

namespace {

struct SysDiskInfoBindData : public FunctionData {
	MemoryUnit unit = MemoryUnit::BYTES;

	bool Equals(const FunctionData &other_p) const override {
		auto &other = other_p.Cast<SysDiskInfoBindData>();
		return unit == other.unit;
	}

	unique_ptr<FunctionData> Copy() const override {
		auto result = make_uniq<SysDiskInfoBindData>();
		result->unit = unit;
		return std::move(result);
	}
};

struct SysDiskInfoData : public GlobalTableFunctionState {
	explicit SysDiskInfoData(ClientContext &context) : finished(false), current_index(0) {
		disks = GetDiskInfo(context);
	}
	bool finished;
	size_t current_index;
	vector<DiskInfo> disks;
};

unique_ptr<FunctionData> SysDiskInfoBind(ClientContext &context, TableFunctionBindInput &input,
                                         vector<LogicalType> &return_types, vector<string> &names) {
	D_ASSERT(return_types.empty());
	D_ASSERT(names.empty());
	return_types.reserve(6);
	names.reserve(6);

	auto result = make_uniq<SysDiskInfoBindData>();

	// Parse unit parameter if provided
	auto unit_it = input.named_parameters.find("unit");
	if (unit_it != input.named_parameters.end()) {
		result->unit = ParseUnit(unit_it->second.ToString());
	}

	names.emplace_back("mount_point");
	return_types.emplace_back(LogicalType {LogicalTypeId::VARCHAR});

	names.emplace_back("file_system");
	return_types.emplace_back(LogicalType {LogicalTypeId::VARCHAR});

	names.emplace_back("file_system_type");
	return_types.emplace_back(LogicalType {LogicalTypeId::VARCHAR});

	names.emplace_back("total_space");
	return_types.emplace_back(LogicalType {LogicalTypeId::UBIGINT});

	names.emplace_back("used_space");
	return_types.emplace_back(LogicalType {LogicalTypeId::UBIGINT});

	names.emplace_back("free_space");
	return_types.emplace_back(LogicalType {LogicalTypeId::UBIGINT});

	return std::move(result);
}

unique_ptr<GlobalTableFunctionState> SysDiskInfoInit(ClientContext &context, TableFunctionInitInput &input) {
	return make_uniq<SysDiskInfoData>(context);
}

void SysDiskInfoFunc(ClientContext &context, TableFunctionInput &data_p, DataChunk &output) {
	auto &data = data_p.global_state->Cast<SysDiskInfoData>();
	auto &bind_data = data_p.bind_data->Cast<SysDiskInfoBindData>();

	if (data.finished) {
		return;
	}

	idx_t output_count = 0;
	idx_t col_idx = 0;

	// Output rows in batches
	while (data.current_index < data.disks.size() && output_count < STANDARD_VECTOR_SIZE) {
		const auto &info = data.disks[data.current_index];
		col_idx = 0;

		// mount_point
		output.SetValue(col_idx++, output_count, Value(info.mount_point));

		// file_system
		output.SetValue(col_idx++, output_count, Value(info.file_system));

		// file_system_type
		output.SetValue(col_idx++, output_count, Value(info.file_system_type));

		// Apply unit conversion
		// total_space
		output.SetValue(col_idx++, output_count, Value::UBIGINT(ConvertBytes(info.total_space, bind_data.unit)));

		// used_space
		output.SetValue(col_idx++, output_count, Value::UBIGINT(ConvertBytes(info.used_space, bind_data.unit)));

		// free_space
		output.SetValue(col_idx++, output_count, Value::UBIGINT(ConvertBytes(info.free_space, bind_data.unit)));

		data.current_index++;
		output_count++;
	}

	if (data.current_index >= data.disks.size()) {
		data.finished = true;
	}

	output.SetCardinality(output_count);
}

} // namespace

void RegisterSysDiskInfoFunction(ExtensionLoader &loader) {
	TableFunction sys_disk_info_func("sys_disk_info", {}, SysDiskInfoFunc, SysDiskInfoBind, SysDiskInfoInit);
	sys_disk_info_func.named_parameters["unit"] = LogicalType::VARCHAR;
	loader.RegisterFunction(sys_disk_info_func);
}

} // namespace duckdb
