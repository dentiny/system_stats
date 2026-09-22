#pragma once

#include <cstddef>
#include <cstdint>

extern "C" {

struct RustCPUInfo {
	const char *model_name;
	const char *architecture;
	int32_t logical_cpus;
	int32_t physical_cpus;
	int32_t l1d_cache_kb;
	int32_t l1i_cache_kb;
	int32_t l2_cache_kb;
	int32_t l3_cache_kb;
	const char *byte_order;
};

struct RustMemoryInfo {
	uint64_t total_memory;
	uint64_t used_memory;
	uint64_t free_memory;
	uint64_t total_swap;
	uint64_t used_swap;
	uint64_t free_swap;
	uint64_t cached_memory;
};

struct RustDiskInfo {
	const char *mount_point;
	const char *file_system;
	const char *file_system_type;
	uint64_t total_space;
	uint64_t used_space;
	uint64_t free_space;
};

struct RustNetworkInfo {
	const char *interface_name;
	const char *ipv4_address;
	uint64_t tx_bytes;
	uint64_t tx_packets;
	uint64_t tx_errors;
	uint64_t tx_dropped;
	uint64_t rx_bytes;
	uint64_t rx_packets;
	uint64_t rx_errors;
	uint64_t rx_dropped;
	uint64_t speed_mbps;
};

struct RustOSInfo {
	const char *name;
	const char *version;
	const char *host_name;
	int32_t handle_count;
	int32_t process_count;
	int32_t thread_count;
	const char *architecture;
	uint64_t uptime;
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
