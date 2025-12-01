#include "cpu_stats.hpp"

#include "duckdb/common/exception.hpp"
#include "duckdb/common/string.hpp"

#ifdef __linux__
#include <cstring>
#include <fstream>
#include <sstream>
#include <sys/utsname.h>
#elif __APPLE__
#include <array>
#include <sys/sysctl.h>
#include <sys/types.h>
#endif

namespace duckdb {

namespace {

#ifdef __linux__
// Read CPU cache size from sysfs (format: "32K", "256K", etc.)
int32_t ReadCPUCacheSize(const char *path) {
	std::ifstream file(path);
	if (!file) {
		return 0;
	}

	string line;
	if (std::getline(file, line)) {
		// Extract numeric part only
		int32_t value = 0;
		for (size_t i = 0; i < line.length(); i++) {
			if (isdigit(line[i])) {
				value = value * 10 + (line[i] - '0');
			} else {
				break;
			}
		}
		return value;
	}
	return 0;
}

CPUInfo GetCPUInfoLinux() {
	CPUInfo info;

	// Get architecture from uname
	struct utsname uts;
	if (uname(&uts) == 0) {
		info.architecture = uts.machine;
	}

	// Read cache sizes
	info.l1d_cache_kb = ReadCPUCacheSize("/sys/devices/system/cpu/cpu0/cache/index0/size");
	info.l1i_cache_kb = ReadCPUCacheSize("/sys/devices/system/cpu/cpu0/cache/index1/size");
	info.l2_cache_kb = ReadCPUCacheSize("/sys/devices/system/cpu/cpu0/cache/index2/size");
	info.l3_cache_kb = ReadCPUCacheSize("/sys/devices/system/cpu/cpu0/cache/index3/size");

	// Parse /proc/cpuinfo
	std::ifstream cpuinfo("/proc/cpuinfo");
	if (!cpuinfo) {
		return info;
	}

	string line;
	bool model_found = false;

	while (std::getline(cpuinfo, line)) {
		// Look for colon separator
		const char *colon = strchr(line.c_str(), ':');
		if (!colon) {
			continue;
		}

		// Get value part (after colon)
		string value = colon + 1;
		// Trim leading whitespace
		value.erase(0, value.find_first_not_of(" \t"));
		value.erase(value.find_last_not_of(" \t\n\r") + 1);

		// Match keys using strstr (matching PostgreSQL approach)
		if (strstr(line.c_str(), "vendor_id") == line.c_str()) {
			info.vendor_id = value;
		} else if (strstr(line.c_str(), "cpu family") == line.c_str()) {
			info.cpu_family = value;
		} else if (strstr(line.c_str(), "model name") == line.c_str()) {
			info.model_name = value;
		} else if (strstr(line.c_str(), "model") == line.c_str() && !model_found) {
			// Store model number (not model_name)
			model_found = true;
		} else if (strstr(line.c_str(), "cpu MHz") == line.c_str()) {
			// Each "cpu MHz" line represents a physical processor
			info.physical_cpus++;

			// Store CPU frequency from first entry (convert MHz to Hz)
			if (info.cpu_frequency_hz == 0) {
				try {
					float mhz = std::stof(value);
					info.cpu_frequency_hz = static_cast<uint64_t>(mhz * 1000000);
				} catch (...) {
					// Ignore parse errors
				}
			}
		} else if (strstr(line.c_str(), "cpu cores") == line.c_str()) {
			try {
				info.num_cores = std::stoi(value);
			} catch (...) {
				// Ignore parse errors
			}
		}
	}

	return info;
}
#endif

#ifdef __APPLE__
CPUInfo GetCPUInfoMacOS() {
	CPUInfo info;

	// Get number of available CPUs
	std::array<int, 2> mib = {CTL_HW, HW_AVAILCPU};
	int count = 0;
	size_t len = sizeof(count);

	if (sysctl(mib.data(), 2, &count, &len, NULL, 0) != 0 || count < 1) {
		mib[1] = HW_NCPU;
		sysctl(mib.data(), 2, &count, &len, NULL, 0);
		if (count < 1) {
			count = 1;
		}
	}
	info.num_cores = count;

	// Get various CPU properties using sysctlbyname
	int int_val;
	uint64_t uint64_val;
	char str_val[256];
	size_t int_size = sizeof(int_val);
	size_t uint64_size = sizeof(uint64_val);
	size_t str_size;

	// Byte order
	if (sysctlbyname("hw.byteorder", &int_val, &int_size, 0, 0) == 0) {
		info.byte_order = std::to_string(int_val);
	}

	// CPU family
	if (sysctlbyname("hw.cpufamily", &int_val, &int_size, 0, 0) == 0) {
		info.cpu_family = std::to_string(int_val);
	}

	// CPU type
	if (sysctlbyname("hw.cputype", &int_val, &int_size, 0, 0) == 0) {
		info.cpu_type = std::to_string(int_val);
	}

	// Logical CPUs
	if (sysctlbyname("hw.logicalcpu", &int_val, &int_size, 0, 0) == 0) {
		info.logical_cpus = int_val;
	}

	// Physical CPUs
	if (sysctlbyname("hw.physicalcpu", &int_val, &int_size, 0, 0) == 0) {
		info.physical_cpus = int_val;
	}

	// CPU frequency
	if (sysctlbyname("hw.cpufrequency", &uint64_val, &uint64_size, 0, 0) == 0) {
		info.cpu_frequency_hz = uint64_val;
	}

	// Cache sizes (convert from bytes to KB, matching PostgreSQL)
	if (sysctlbyname("hw.l1dcachesize", &uint64_val, &uint64_size, 0, 0) == 0) {
		info.l1d_cache_kb = static_cast<int32_t>(uint64_val / 1024);
	}

	if (sysctlbyname("hw.l1icachesize", &uint64_val, &uint64_size, 0, 0) == 0) {
		info.l1i_cache_kb = static_cast<int32_t>(uint64_val / 1024);
	}

	if (sysctlbyname("hw.l2cachesize", &uint64_val, &uint64_size, 0, 0) == 0) {
		info.l2_cache_kb = static_cast<int32_t>(uint64_val / 1024);
	}

	if (sysctlbyname("hw.l3cachesize", &uint64_val, &uint64_size, 0, 0) == 0) {
		info.l3_cache_kb = static_cast<int32_t>(uint64_val / 1024);
	}

	// Model name
	str_size = sizeof(str_val);
	if (sysctlbyname("hw.model", str_val, &str_size, NULL, 0) == 0) {
		info.model_name = str_val;
	}

	// Machine architecture
	str_size = sizeof(str_val);
	if (sysctlbyname("hw.machine", str_val, &str_size, 0, 0) == 0) {
		info.architecture = str_val;
	}

	return info;
}
#endif

} // namespace

CPUInfo GetCPUInfo() {
#ifdef __linux__
	return GetCPUInfoLinux();
#elif __APPLE__
	return GetCPUInfoMacOS();
#else
	throw NotImplementedException("CPU statistics are not supported on this platform");
#endif
}

} // namespace duckdb
