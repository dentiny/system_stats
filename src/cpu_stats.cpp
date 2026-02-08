// TODO(hjiang): Add system stats for containerized environments.

#include "cpu_stats.hpp"

#include "duckdb/common/exception.hpp"
#include "duckdb/common/numeric_utils.hpp"
#include "duckdb/common/operator/integer_cast_operator.hpp"
#include "duckdb/common/string.hpp"
#include "duckdb/common/string_util.hpp"
#include "string_utils.hpp"

#ifdef __linux__
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
// Get system byte order.
string GetByteOrder() {
	union {
		uint32_t i;
		char c[4];
	} test = {0x01020304};
	return test.c[0] == 1 ? "Big Endian" : "Little Endian";
}

// Read CPU cache size from sysfs in KiB.
// Example content: "32K", "256K", etc.
int32_t ReadCPUCacheSize(const char *path) {
	std::ifstream file(path);
	if (!file) {
		return 0;
	}

	string line;
	if (std::getline(file, line)) {
		// Extract numeric part only
		int32_t value = 0;
		for (size_t idx = 0; idx < line.length(); idx++) {
			if (isdigit(line[idx])) {
				value = value * 10 + (line[idx] - '0');
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

	struct utsname uts;
	if (uname(&uts) == 0) {
		info.architecture = uts.machine;
	}

	// Determine byte order
	info.byte_order = GetByteOrder();

	// Read cache sizes from sysfs.
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
	int processor_count = 0;

	// ARM-specific fields
	string cpu_implementer;
	string cpu_architecture;
	string cpu_variant;
	string cpu_part;

	while (std::getline(cpuinfo, line)) {
		// Look for colon separator
		size_t colon_pos = line.find(':');
		if (colon_pos == string::npos) {
			continue;
		}

		std::string_view line_sv {line};
		std::string_view key = TrimString(line_sv.substr(0, colon_pos));
		std::string_view value = TrimString(line_sv.substr(colon_pos + 1));

		if (key == "model name") {
			info.model_name = value;
			continue;
		}
		if (key == "model" && !model_found) {
			model_found = true;
			continue;
		}
		if (key == "cpu MHz") {
			// Each "cpu MHz" line represents a logical processor
			info.logical_cpus++;
			continue;
		}
		if (key == "physical id") {
			// Count unique physical processors
			int32_t phys_id = 0;
			if (TrySimpleIntegerCast<int32_t>(value.data(), value.length(), phys_id, false)) {
				if (phys_id + 1 > info.physical_cpus) {
					info.physical_cpus = phys_id + 1;
				}
			}
			// Ignore parse errors
			continue;
		}

		if (key == "processor") {
			processor_count++;
			continue;
		}
		if (key == "CPU implementer") {
			if (cpu_implementer.empty()) {
				cpu_implementer = value;
			}
			continue;
		}
		if (key == "CPU architecture") {
			if (cpu_architecture.empty()) {
				cpu_architecture = value;
			}
			continue;
		}
		if (key == "CPU variant") {
			if (cpu_variant.empty()) {
				cpu_variant = value;
			}
			continue;
		}
		if (key == "CPU part") {
			if (cpu_part.empty()) {
				cpu_part = value;
			}
			continue;
		}
	}

	if (processor_count > 0 && info.logical_cpus == 0) {
		info.logical_cpus = processor_count;
		info.physical_cpus = processor_count;

		// Build a descriptive model name from ARM CPU info
		if (info.model_name.empty() && !cpu_implementer.empty()) {
			info.model_name = StringUtil::Format("ARM v%s (impl: %s, part: %s, variant: %s)", cpu_architecture,
			                                     cpu_implementer, cpu_part, cpu_variant);
		}
	}

	return info;
}
#endif

#ifdef __APPLE__
// Get system byte order for macOS.
string GetByteOrderMacOS() {
	int int_val = 0;
	size_t int_size = sizeof(int_val);

	if (sysctlbyname("hw.byteorder", &int_val, &int_size, 0, 0) == 0) {
		// macOS byte order: 1234 = little-endian, 4321 = big-endian
		if (int_val == 1234) {
			return "Little Endian";
		}
		D_ASSERT(int_val == 4321);
		return "Big Endian";
	}
	return "(Unknown)";
}

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

	// Get various CPU properties using sysctlbyname
	int int_val = 0;
	uint64_t uint64_val = 0;
	char str_val[256] = {0};
	size_t int_size = sizeof(int_val);
	size_t uint64_size = sizeof(uint64_val);
	size_t str_size = sizeof(str_val);

	// Byte order
	info.byte_order = GetByteOrderMacOS();

	// Logical CPUs
	if (sysctlbyname("hw.logicalcpu", &int_val, &int_size, 0, 0) == 0) {
		info.logical_cpus = int_val;
	}

	// Physical CPUs
	if (sysctlbyname("hw.physicalcpu", &int_val, &int_size, 0, 0) == 0) {
		info.physical_cpus = int_val;
	}

	// Cache sizes (convert from bytes to KB, matching PostgreSQL)
	if (sysctlbyname("hw.l1dcachesize", &uint64_val, &uint64_size, 0, 0) == 0) {
		info.l1d_cache_kb = NumericCast<int32_t>(uint64_val / 1024);
	}

	if (sysctlbyname("hw.l1icachesize", &uint64_val, &uint64_size, 0, 0) == 0) {
		info.l1i_cache_kb = NumericCast<int32_t>(uint64_val / 1024);
	}

	if (sysctlbyname("hw.l2cachesize", &uint64_val, &uint64_size, 0, 0) == 0) {
		info.l2_cache_kb = NumericCast<int32_t>(uint64_val / 1024);
	}

	if (sysctlbyname("hw.l3cachesize", &uint64_val, &uint64_size, 0, 0) == 0) {
		info.l3_cache_kb = NumericCast<int32_t>(uint64_val / 1024);
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
