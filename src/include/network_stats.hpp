#pragma once

#include "duckdb/common/string.hpp"
#include "duckdb/common/vector.hpp"
#include "duckdb/common/types.hpp"

namespace duckdb {

// Forward declaration.
class ClientContext;

struct NetworkInfo {
	string interface_name;
	string ipv4_address;
	uint64_t tx_bytes = 0;
	uint64_t tx_packets = 0;
	uint64_t tx_errors = 0;
	uint64_t tx_dropped = 0;
	uint64_t rx_bytes = 0;
	uint64_t rx_packets = 0;
	uint64_t rx_errors = 0;
	uint64_t rx_dropped = 0;
	uint64_t speed_mbps = 0;
};

// Get network information for the current platform
vector<NetworkInfo> GetNetworkInfo(ClientContext &context);

} // namespace duckdb
