#include "os_info_query_function.hpp"

#include "duckdb/common/assert.hpp"
#include "duckdb/common/types/value.hpp"
#include "duckdb/common/vector_size.hpp"
#include "duckdb/function/table_function.hpp"
#include "os_info.hpp"

namespace duckdb {

namespace {

struct SysOSInfoData : public GlobalTableFunctionState {
	SysOSInfoData() : finished(false) {
		os_info = GetOSInfo();
	}
	bool finished;
	OSInfo os_info;
};

unique_ptr<FunctionData> SysOSInfoBind(ClientContext &context, TableFunctionBindInput &input,
                                       vector<LogicalType> &return_types, vector<string> &names) {
	D_ASSERT(return_types.empty());
	D_ASSERT(names.empty());
	return_types.reserve(9);
	names.reserve(9);

	names.emplace_back("name");
	return_types.emplace_back(LogicalType {LogicalTypeId::VARCHAR});

	names.emplace_back("version");
	return_types.emplace_back(LogicalType {LogicalTypeId::VARCHAR});

	names.emplace_back("host_name");
	return_types.emplace_back(LogicalType {LogicalTypeId::VARCHAR});

	names.emplace_back("domain_name");
	return_types.emplace_back(LogicalType {LogicalTypeId::VARCHAR});

	names.emplace_back("handle_count");
	return_types.emplace_back(LogicalType {LogicalTypeId::INTEGER});

	names.emplace_back("process_count");
	return_types.emplace_back(LogicalType {LogicalTypeId::INTEGER});

	names.emplace_back("thread_count");
	return_types.emplace_back(LogicalType {LogicalTypeId::INTEGER});

	names.emplace_back("architecture");
	return_types.emplace_back(LogicalType {LogicalTypeId::VARCHAR});

	names.emplace_back("os_up_since_seconds");
	return_types.emplace_back(LogicalType {LogicalTypeId::INTEGER});

	return nullptr;
}

unique_ptr<GlobalTableFunctionState> SysOSInfoInit(ClientContext &context, TableFunctionInitInput &input) {
	return make_uniq<SysOSInfoData>();
}

void SysOSInfoFunc(ClientContext &context, TableFunctionInput &data_p, DataChunk &output) {
	auto &data = data_p.global_state->Cast<SysOSInfoData>();

	if (data.finished) {
		return;
	}

	const auto &info = data.os_info;
	idx_t col_idx = 0;

	// name
	output.SetValue(col_idx++, 0, Value(info.name));

	// version
	output.SetValue(col_idx++, 0, Value(info.version));

	// host_name
	output.SetValue(col_idx++, 0, Value(info.host_name));

	// domain_name
	output.SetValue(col_idx++, 0, Value(info.domain_name));

	// handle_count
	output.SetValue(col_idx++, 0, Value::INTEGER(info.handle_count));

	// process_count
	output.SetValue(col_idx++, 0, Value::INTEGER(info.process_count));

	// thread_count
	output.SetValue(col_idx++, 0, Value::INTEGER(info.thread_count));

	// architecture
	output.SetValue(col_idx++, 0, Value(info.architecture));

	// os_up_since_seconds
	output.SetValue(col_idx++, 0, Value::INTEGER(info.os_up_since_seconds));

	output.SetCardinality(1);
	data.finished = true;
}

} // namespace

void RegisterSysOSInfoFunction(ExtensionLoader &loader) {
	TableFunction sys_os_info_func("sys_os_info", {}, SysOSInfoFunc, SysOSInfoBind, SysOSInfoInit);
	loader.RegisterFunction(sys_os_info_func);
}

} // namespace duckdb
