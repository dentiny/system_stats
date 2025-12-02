#include "network_stats_query_function.hpp"

#include "duckdb/common/types/value.hpp"
#include "duckdb/common/vector.hpp"
#include "duckdb/common/vector_size.hpp"
#include "duckdb/function/table_function.hpp"
#include "network_stats.hpp"

namespace duckdb {

namespace {

struct SysNetworkInfoData : public GlobalTableFunctionState {
	SysNetworkInfoData() : finished(false), current_index(0) {
		networks = GetNetworkInfo();
	}
	bool finished;
	size_t current_index;
	vector<NetworkInfo> networks;
};

unique_ptr<FunctionData> SysNetworkInfoBind(ClientContext &context, TableFunctionBindInput &input,
                                            vector<LogicalType> &return_types, vector<string> &names) {
	// Match PostgreSQL system_stats extension column names
	names.emplace_back("interface_name");
	return_types.emplace_back(LogicalType {LogicalTypeId::VARCHAR});

	names.emplace_back("ip_address");
	return_types.emplace_back(LogicalType {LogicalTypeId::VARCHAR});

	names.emplace_back("tx_bytes");
	return_types.emplace_back(LogicalType {LogicalTypeId::UBIGINT});

	names.emplace_back("tx_packets");
	return_types.emplace_back(LogicalType {LogicalTypeId::UBIGINT});

	names.emplace_back("tx_errors");
	return_types.emplace_back(LogicalType {LogicalTypeId::UBIGINT});

	names.emplace_back("tx_dropped");
	return_types.emplace_back(LogicalType {LogicalTypeId::UBIGINT});

	names.emplace_back("rx_bytes");
	return_types.emplace_back(LogicalType {LogicalTypeId::UBIGINT});

	names.emplace_back("rx_packets");
	return_types.emplace_back(LogicalType {LogicalTypeId::UBIGINT});

	names.emplace_back("rx_errors");
	return_types.emplace_back(LogicalType {LogicalTypeId::UBIGINT});

	names.emplace_back("rx_dropped");
	return_types.emplace_back(LogicalType {LogicalTypeId::UBIGINT});

	names.emplace_back("link_speed_mbps");
	return_types.emplace_back(LogicalType {LogicalTypeId::INTEGER});

	return nullptr;
}

unique_ptr<GlobalTableFunctionState> SysNetworkInfoInit(ClientContext &context, TableFunctionInitInput &input) {
	return make_uniq<SysNetworkInfoData>();
}

void SysNetworkInfoFunc(ClientContext &context, TableFunctionInput &data_p, DataChunk &output) {
	auto &data = data_p.global_state->Cast<SysNetworkInfoData>();

	if (data.finished) {
		return;
	}

	idx_t output_count = 0;
	idx_t col_idx = 0;

	// Output rows in batches
	while (data.current_index < data.networks.size() && output_count < STANDARD_VECTOR_SIZE) {
		const auto &info = data.networks[data.current_index];
		col_idx = 0;

		// interface_name
		output.SetValue(col_idx++, output_count, Value(info.interface_name));

		// ip_address
		output.SetValue(col_idx++, output_count, Value(info.ipv4_address));

		// tx_bytes
		output.SetValue(col_idx++, output_count, Value::UBIGINT(info.tx_bytes));

		// tx_packets
		output.SetValue(col_idx++, output_count, Value::UBIGINT(info.tx_packets));

		// tx_errors
		output.SetValue(col_idx++, output_count, Value::UBIGINT(info.tx_errors));

		// tx_dropped
		output.SetValue(col_idx++, output_count, Value::UBIGINT(info.tx_dropped));

		// rx_bytes
		output.SetValue(col_idx++, output_count, Value::UBIGINT(info.rx_bytes));

		// rx_packets
		output.SetValue(col_idx++, output_count, Value::UBIGINT(info.rx_packets));

		// rx_errors
		output.SetValue(col_idx++, output_count, Value::UBIGINT(info.rx_errors));

		// rx_dropped
		output.SetValue(col_idx++, output_count, Value::UBIGINT(info.rx_dropped));

		// link_speed_mbps
		output.SetValue(col_idx++, output_count, Value::INTEGER(static_cast<int32_t>(info.speed_mbps)));

		data.current_index++;
		output_count++;
	}

	if (data.current_index >= data.networks.size()) {
		data.finished = true;
	}

	output.SetCardinality(output_count);
}

} // namespace

// Register function (must be after anonymous namespace to reference anonymous functions)
void RegisterSysNetworkInfoFunction(ExtensionLoader &loader) {
	TableFunction sys_network_info_func("sys_network_info", {}, SysNetworkInfoFunc, SysNetworkInfoBind,
	                                    SysNetworkInfoInit);
	loader.RegisterFunction(sys_network_info_func);
}

} // namespace duckdb
