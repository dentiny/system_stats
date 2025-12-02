#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace duckdb {

struct NetworkInfo {
	std::string interface_name;
	std::string ipv4_address;
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
std::vector<NetworkInfo> GetNetworkInfo();

} // namespace duckdb
