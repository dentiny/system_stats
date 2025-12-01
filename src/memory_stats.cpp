#include "memory_stats.hpp"
#include "duckdb/common/exception.hpp"

#ifdef __linux__
#include <sys/sysinfo.h>
#include <fstream>
#include <sstream>
#include <string>
#elif __APPLE__
#include <sys/types.h>
#include <sys/sysctl.h>
#include <mach/mach.h>
#include <mach/vm_statistics.h>
#include <mach/mach_types.h>
#include <mach/mach_init.h>
#include <mach/mach_host.h>
#endif

namespace duckdb {

#ifdef __linux__
static MemoryInfo GetMemoryInfoLinux() {
	MemoryInfo info;

	// Use sysinfo for basic stats
	struct sysinfo si;
	if (sysinfo(&si) == 0) {
		info.total_memory = static_cast<uint64_t>(si.totalram) * si.mem_unit;
		info.free_memory = static_cast<uint64_t>(si.freeram) * si.mem_unit;
		info.total_swap = static_cast<uint64_t>(si.totalswap) * si.mem_unit;
		info.free_swap = static_cast<uint64_t>(si.freeswap) * si.mem_unit;
		info.buffers_memory = static_cast<uint64_t>(si.bufferram) * si.mem_unit;
		info.used_swap = info.total_swap - info.free_swap;
	}

	// Parse /proc/meminfo for more detailed stats
	std::ifstream meminfo("/proc/meminfo");
	std::string line;

	while (std::getline(meminfo, line)) {
		std::istringstream iss(line);
		std::string key;
		uint64_t value;
		std::string unit;

		if (iss >> key >> value >> unit) {
			// Convert from kB to bytes
			value *= 1024;

			if (key == "Cached:") {
				info.cached_memory = value;
			} else if (key == "MemAvailable:") {
				info.available_memory = value;
			}
		}
	}

	// Calculate used memory
	info.used_memory = info.total_memory - info.free_memory;

	// If available memory wasn't found, estimate it
	if (info.available_memory == 0) {
		info.available_memory = info.free_memory + info.buffers_memory + info.cached_memory;
	}

	return info;
}
#endif

#ifdef __APPLE__
static MemoryInfo GetMemoryInfoMacOS() {
	MemoryInfo info;

	// Get total physical memory
	int mib[2] = {CTL_HW, HW_MEMSIZE};
	uint64_t total_mem;
	size_t len = sizeof(total_mem);
	if (sysctl(mib, 2, &total_mem, &len, NULL, 0) == 0) {
		info.total_memory = total_mem;
	}

	// Get VM statistics
	mach_msg_type_number_t count = HOST_VM_INFO64_COUNT;
	vm_statistics64_data_t vm_stats;
	kern_return_t kr = host_statistics64(mach_host_self(), HOST_VM_INFO64, (host_info64_t)&vm_stats, &count);

	if (kr == KERN_SUCCESS) {
		natural_t page_size;
		host_page_size(mach_host_self(), &page_size);

		info.free_memory = static_cast<uint64_t>(vm_stats.free_count) * page_size;
		info.cached_memory = static_cast<uint64_t>(vm_stats.external_page_count) * page_size;
		info.used_memory = static_cast<uint64_t>(vm_stats.active_count + vm_stats.wire_count) * page_size;
		info.available_memory = info.free_memory + static_cast<uint64_t>(vm_stats.inactive_count) * page_size;
	}

	// Get swap usage
	int swap_mib[2] = {CTL_VM, VM_SWAPUSAGE};
	struct xsw_usage swap_info;
	size_t swap_len = sizeof(swap_info);
	if (sysctl(swap_mib, 2, &swap_info, &swap_len, NULL, 0) == 0) {
		info.total_swap = swap_info.xsu_total;
		info.used_swap = swap_info.xsu_used;
		info.free_swap = swap_info.xsu_avail;
	}

	return info;
}
#endif

MemoryInfo GetMemoryInfo() {
#ifdef __linux__
	return GetMemoryInfoLinux();
#elif __APPLE__
	return GetMemoryInfoMacOS();
#else
	throw NotImplementedException("Memory statistics are not supported on this platform");
#endif
}

} // namespace duckdb
