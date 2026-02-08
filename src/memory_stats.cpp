#include "memory_stats.hpp"

#include "database_instance_cache.hpp"
#include "duckdb/common/exception.hpp"
#include "duckdb/common/numeric_utils.hpp"
#include "duckdb/common/string.hpp"
#include "duckdb/logging/logger.hpp"

#ifdef __linux__
#include <fstream>
#include <sstream>
#elif __APPLE__
#include <array>
#include <cerrno>
#include <mach/mach.h>
#include <sys/sysctl.h>
#include <sys/types.h>
#include <unistd.h>
#endif

namespace duckdb {

namespace {

#ifdef __linux__
// Convert memory value from /proc/meminfo (in kB) to bytes
uint64_t ParseBytesValue(const string &line) {
	std::istringstream iss(line);
	string key;
	uint64_t value;
	string unit;

	if (iss >> key >> value >> unit) {
		// Convert from kB to bytes
		return value * 1024;
	}
	return 0;
}

MemoryInfo GetMemoryInfoLinux(ClientContext &context) {
	MemoryInfo info;
	std::ifstream meminfo("/proc/meminfo");
	if (!meminfo.is_open()) {
		if (auto *db = GetDbInstance(context)) {
			DUCKDB_LOG_DEBUG(*db, "Failed to open /proc/meminfo: %s", strerror(errno));
		}
		return info;
	}
	string line;
	int parsed_count = 0;

	while (std::getline(meminfo, line) && parsed_count < 5) {
		// Read total memory
		if (line.find("MemTotal:") == 0) {
			info.total_memory = ParseBytesValue(line);
			++parsed_count;
			continue;
		}
		// Read free memory
		if (line.find("MemFree:") == 0) {
			info.free_memory = ParseBytesValue(line);
			++parsed_count;
			continue;
		}
		// Read cached memory
		if (line.find("Cached:") == 0) {
			info.cached_memory = ParseBytesValue(line);
			++parsed_count;
			continue;
		}
		// Read total swap memory
		if (line.find("SwapTotal:") == 0) {
			info.total_swap = ParseBytesValue(line);
			++parsed_count;
		}
		// Read free swap memory
		if (line.find("SwapFree:") == 0) {
			info.free_swap = ParseBytesValue(line);
			++parsed_count;
			continue;
		}
	}

	info.used_memory = info.total_memory - info.free_memory;
	info.used_swap = info.total_swap - info.free_swap;

	return info;
}
#endif

#ifdef __APPLE__
MemoryInfo GetMemoryInfoMacOS(ClientContext &context) {
	MemoryInfo info;

	// Get total physical memory
	std::array<int, 2> mib = {CTL_HW, HW_MEMSIZE};
	size_t len = sizeof(info.total_memory);
	if (sysctl(mib.data(), 2, &info.total_memory, &len, NULL, 0) != 0) {
		if (auto *db = GetDbInstance(context)) {
			DUCKDB_LOG_DEBUG(*db, "sysctl() failed to get total memory: %s", strerror(errno));
		}
		return info;
	}

	// Get VM statistics
	vm_statistics_data_t vm_stats;
	mach_msg_type_number_t count = sizeof(vm_stats) / sizeof(integer_t);
	mach_port_t mport = mach_host_self();

	kern_return_t ret = host_statistics(mport, HOST_VM_INFO, (host_info_t)&vm_stats, &count);
	if (ret != KERN_SUCCESS) {
		if (auto *db = GetDbInstance(context)) {
			DUCKDB_LOG_DEBUG(*db, "host_statistics() failed with error code: %d", ret);
		}
		return info;
	}

	int pagesize = getpagesize();

	// Free memory includes inactive pages
	info.free_memory = NumericCast<uint64_t>(vm_stats.inactive_count + vm_stats.free_count) * pagesize;
	info.used_memory = info.total_memory - info.free_memory;

	// Get swap usage
	std::array<int, 2> swap_mib = {CTL_VM, VM_SWAPUSAGE};
	struct xsw_usage swap_info;
	size_t swap_len = sizeof(swap_info);
	if (sysctl(swap_mib.data(), 2, &swap_info, &swap_len, NULL, 0) != 0) {
		if (auto *db = GetDbInstance(context)) {
			DUCKDB_LOG_DEBUG(*db, "sysctl() failed to get swap usage: %s", strerror(errno));
		}
	} else {
		info.total_swap = swap_info.xsu_total;
		info.used_swap = swap_info.xsu_used;
		info.free_swap = swap_info.xsu_avail;
	}

	return info;
}
#endif

} // namespace

MemoryInfo GetMemoryInfo(ClientContext &context) {
#ifdef __linux__
	return GetMemoryInfoLinux(context);
#elif __APPLE__
	return GetMemoryInfoMacOS(context);
#else
	throw NotImplementedException("Memory statistics are not supported on this platform");
#endif
}

} // namespace duckdb
