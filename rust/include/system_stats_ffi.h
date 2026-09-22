#pragma once

#include <cstddef>
#include <cstdint>

extern "C" {

struct RustCPUInfo {
	const char *model_name = nullptr;
	const char *architecture = nullptr;
	int32_t logical_cpus = 0;
	int32_t physical_cpus = 0;
	int32_t l1d_cache_kb = 0;
	int32_t l1i_cache_kb = 0;
	int32_t l2_cache_kb = 0;
	int32_t l3_cache_kb = 0;
	const char *byte_order = nullptr;
};

struct RustMemoryInfo {
	uint64_t total_memory = 0;
	uint64_t used_memory = 0;
	uint64_t free_memory = 0;
	uint64_t total_swap = 0;
	uint64_t used_swap = 0;
	uint64_t free_swap = 0;
	uint64_t cached_memory = 0;
};

struct RustDiskInfo {
	const char *mount_point = nullptr;
	const char *file_system = nullptr;
	const char *file_system_type = nullptr;
	uint64_t total_space = 0;
	uint64_t used_space = 0;
	uint64_t free_space = 0;
};

struct RustNetworkInfo {
	const char *interface_name = nullptr;
	const char *ipv4_address = nullptr;
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

struct RustOSInfo {
	const char *name = nullptr;
	const char *version = nullptr;
	const char *host_name = nullptr;
	int32_t handle_count = 0;
	int32_t process_count = 0;
	int32_t thread_count = 0;
	const char *architecture = nullptr;
	uint64_t uptime = 0;
};

struct RustCPUInfoHandle;
struct RustDiskInfoList;
struct RustNetworkInfoList;
struct RustOSInfoHandle;

RustCPUInfoHandle *system_stats_cpu_info();
const RustCPUInfo *system_stats_cpu_info_get(const RustCPUInfoHandle *handle);
void system_stats_cpu_info_free(RustCPUInfoHandle *handle);

RustMemoryInfo system_stats_memory_info();

RustDiskInfoList *system_stats_disk_info();
size_t system_stats_disk_info_len(const RustDiskInfoList *list);
const RustDiskInfo *system_stats_disk_info_get(const RustDiskInfoList *list, size_t index);
void system_stats_disk_info_free(RustDiskInfoList *list);

RustNetworkInfoList *system_stats_network_info();
size_t system_stats_network_info_len(const RustNetworkInfoList *list);
const RustNetworkInfo *system_stats_network_info_get(const RustNetworkInfoList *list, size_t index);
void system_stats_network_info_free(RustNetworkInfoList *list);

RustOSInfoHandle *system_stats_os_info();
const RustOSInfo *system_stats_os_info_get(const RustOSInfoHandle *handle);
void system_stats_os_info_free(RustOSInfoHandle *handle);
}
