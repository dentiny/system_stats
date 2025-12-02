#include "network_stats.hpp"

#include "duckdb/common/exception.hpp"
#include "duckdb/common/string.hpp"
#include "duckdb/common/string_util.hpp"
#include "duckdb/common/vector.hpp"
#include "scope_guard.hpp"

#ifdef __linux__
#include <arpa/inet.h>
#include <array>
#include <fstream>
#include <ifaddrs.h>
#include <netdb.h>
#include <sstream>
#include <sys/socket.h>
#elif __APPLE__
#include <arpa/inet.h>
#include <array>
#include <cstdlib>
#include <cstring>
#include <ifaddrs.h>
#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_var.h>
#include <net/route.h>
#include <netdb.h>
#include <netinet/tcp_fsm.h>
#include <sys/socket.h>
#include <sys/sysctl.h>
#include <sys/types.h>
#endif

namespace duckdb {

namespace {

#ifdef __linux__
// Read a value from a file in /sys/class/net
uint64_t ReadSysNetValue(const string &interface, const string &stat_name) {
	string file_path = StringUtil::Format("/sys/class/net/%s/statistics/%s", interface, stat_name);
	std::ifstream file(file_path);
	if (!file.is_open()) {
		return 0;
	}
	uint64_t value = 0;
	file >> value;
	return value;
}

// Read speed from /sys/class/net/{interface}/speed
uint64_t ReadSpeedMbps(const string &interface) {
	string file_path = StringUtil::Format("/sys/class/net/%s/speed", interface);
	std::ifstream file(file_path);
	if (!file.is_open()) {
		return 0;
	}
	uint64_t speed = 0;
	file >> speed;
	return speed;
}

vector<NetworkInfo> GetNetworkInfoLinux() {
	vector<NetworkInfo> networks;
	struct ifaddrs *ifaddr;
	struct ifaddrs *ifa;

	if (getifaddrs(&ifaddr) == -1) {
		return networks;
	}

	SCOPE_EXIT {
		freeifaddrs(ifaddr);
	};

	// Iterate through all network interfaces
	for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
		if (ifa->ifa_addr == NULL) {
			continue;
		}

		// Only process IPv4 addresses
		if (ifa->ifa_addr->sa_family != AF_INET) {
			continue;
		}

		NetworkInfo info;
		info.interface_name = ifa->ifa_name;

		// Get IPv4 address
		std::array<char, NI_MAXHOST> host;
		int ret =
		    getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in), host.data(), NI_MAXHOST, NULL, 0, NI_NUMERICHOST);
		if (ret == 0) {
			info.ipv4_address = host.data();
		}

		// Read statistics from /sys/class/net
		info.speed_mbps = ReadSpeedMbps(info.interface_name);
		info.rx_bytes = ReadSysNetValue(info.interface_name, "rx_bytes");
		info.tx_bytes = ReadSysNetValue(info.interface_name, "tx_bytes");
		info.rx_packets = ReadSysNetValue(info.interface_name, "rx_packets");
		info.tx_packets = ReadSysNetValue(info.interface_name, "tx_packets");
		info.rx_errors = ReadSysNetValue(info.interface_name, "rx_errors");
		info.tx_errors = ReadSysNetValue(info.interface_name, "tx_errors");
		info.rx_dropped = ReadSysNetValue(info.interface_name, "rx_dropped");
		info.tx_dropped = ReadSysNetValue(info.interface_name, "tx_dropped");

		networks.emplace_back(info);
	}

	return networks;
}
#endif

#ifdef __APPLE__
vector<NetworkInfo> GetNetworkInfoMacOS() {
	vector<NetworkInfo> networks;

	// Get network interface list using sysctl
	std::array<int, 6> desc = {CTL_NET, PF_ROUTE, 0, 0, NET_RT_IFLIST2, 0};
	size_t len = 0;

	if (sysctl(desc.data(), 6, NULL, &len, NULL, 0) < 0) {
		return networks;
	}

	char *buf = static_cast<char *>(malloc(len));
	if (buf == NULL) {
		return networks;
	}

	SCOPE_EXIT {
		free(buf);
	};

	if (sysctl(desc.data(), 6, buf, &len, NULL, 0) < 0) {
		return networks;
	}

	// Get interface addresses using getifaddrs
	struct ifaddrs *ifaddr;
	if (getifaddrs(&ifaddr) == -1) {
		return networks;
	}

	SCOPE_EXIT {
		freeifaddrs(ifaddr);
	};

	char *limit = buf + len;
	char *next_ptr = buf;

	// Process sysctl buffer
	while (next_ptr < limit) {
		struct if_msghdr *ifm = reinterpret_cast<struct if_msghdr *>(next_ptr);
		next_ptr += ifm->ifm_msglen;

		if (ifm->ifm_type != RTM_IFINFO2) {
			continue;
		}

		struct if_msghdr2 *if2m = reinterpret_cast<struct if_msghdr2 *>(ifm);
		struct sockaddr_dl *sdl = reinterpret_cast<struct sockaddr_dl *>(if2m + 1);

		string interface_name(sdl->sdl_data, sdl->sdl_nlen);

		// Find matching IPv4 address from getifaddrs
		for (struct ifaddrs *ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
			if (ifa->ifa_addr == NULL) {
				continue;
			}

			if (strcmp(ifa->ifa_name, interface_name.c_str()) != 0) {
				continue;
			}
			if (ifa->ifa_addr->sa_family != AF_INET) {
				continue;
			}

			NetworkInfo info;
			info.interface_name = interface_name;

			// Get IPv4 address
			std::array<char, NI_MAXHOST> host;
			int ret = getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in), host.data(), NI_MAXHOST, NULL, 0,
			                      NI_NUMERICHOST);
			if (ret == 0) {
				info.ipv4_address = host.data();
			}

			// Get statistics from if_msghdr2
			info.tx_bytes = static_cast<uint64_t>(if2m->ifm_data.ifi_obytes);
			info.tx_packets = static_cast<uint64_t>(if2m->ifm_data.ifi_opackets);
			info.tx_errors = static_cast<uint64_t>(if2m->ifm_data.ifi_oerrors);
			info.tx_dropped = 0; // Not available on macOS
			info.speed_mbps = 0; // Not available on macOS
			info.rx_bytes = static_cast<uint64_t>(if2m->ifm_data.ifi_ibytes);
			info.rx_packets = static_cast<uint64_t>(if2m->ifm_data.ifi_ipackets);
			info.rx_errors = static_cast<uint64_t>(if2m->ifm_data.ifi_ierrors);
			info.rx_dropped = static_cast<uint64_t>(if2m->ifm_data.ifi_iqdrops);

			networks.emplace_back(info);
			break;
		}
	}

	return networks;
}
#endif

} // namespace

vector<NetworkInfo> GetNetworkInfo() {
#ifdef __linux__
	return GetNetworkInfoLinux();
#elif __APPLE__
	return GetNetworkInfoMacOS();
#else
	throw NotImplementedException("Network statistics are not supported on this platform");
#endif
}

} // namespace duckdb
