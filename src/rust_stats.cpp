#include "cpu_stats.hpp"
#include "disk_stats.hpp"
#include "memory_stats.hpp"
#include "network_stats.hpp"
#include "os_info.hpp"
#include "system_stats_ffi.h"

namespace duckdb {

namespace {

template <class HANDLE, HANDLE *(*CREATE)(), void (*DESTROY)(HANDLE *)>
class RustHandle {
public:
	RustHandle() : handle(CREATE()) {
	}

	~RustHandle() {
		DESTROY(handle);
	}

	RustHandle(const RustHandle &) = delete;
	RustHandle &operator=(const RustHandle &) = delete;

	explicit operator bool() const {
		return handle != nullptr;
	}

	HANDLE *Get() const {
		return handle;
	}

private:
	HANDLE *handle;
};

using CPUInfoHandle = RustHandle<RustCPUInfoHandle, system_stats_cpu_info, system_stats_cpu_info_free>;
using DiskInfoHandle = RustHandle<RustDiskInfoList, system_stats_disk_info, system_stats_disk_info_free>;
using NetworkInfoHandle = RustHandle<RustNetworkInfoList, system_stats_network_info, system_stats_network_info_free>;
using OSInfoHandle = RustHandle<RustOSInfoHandle, system_stats_os_info, system_stats_os_info_free>;

string CopyString(const char *value) {
	return value ? string(value) : string();
}

} // namespace

CPUInfo GetCPUInfo() {
	CPUInfo result;
	CPUInfoHandle handle;
	if (!handle) {
		return result;
	}
	auto info = system_stats_cpu_info_get(handle.Get());
	if (info) {
		result.model_name = CopyString(info->model_name);
		result.architecture = CopyString(info->architecture);
		result.logical_cpus = info->logical_cpus;
		result.physical_cpus = info->physical_cpus;
		result.l1d_cache_kb = info->l1d_cache_kb;
		result.l1i_cache_kb = info->l1i_cache_kb;
		result.l2_cache_kb = info->l2_cache_kb;
		result.l3_cache_kb = info->l3_cache_kb;
		result.byte_order = CopyString(info->byte_order);
	}
	return result;
}

MemoryInfo GetMemoryInfo() {
	auto info = system_stats_memory_info();
	MemoryInfo result;
	result.total_memory = info.total_memory;
	result.used_memory = info.used_memory;
	result.free_memory = info.free_memory;
	result.total_swap = info.total_swap;
	result.used_swap = info.used_swap;
	result.free_swap = info.free_swap;
	result.cached_memory = info.cached_memory;
	return result;
}

vector<DiskInfo> GetDiskInfo() {
	vector<DiskInfo> result;
	DiskInfoHandle handle;
	if (!handle) {
		return result;
	}
	auto count = system_stats_disk_info_len(handle.Get());
	result.reserve(count);
	for (size_t index = 0; index < count; index++) {
		auto info = system_stats_disk_info_get(handle.Get(), index);
		if (!info) {
			continue;
		}
		DiskInfo disk;
		disk.mount_point = CopyString(info->mount_point);
		disk.file_system = CopyString(info->file_system);
		disk.file_system_type = CopyString(info->file_system_type);
		disk.total_space = info->total_space;
		disk.used_space = info->used_space;
		disk.free_space = info->free_space;
		result.emplace_back(std::move(disk));
	}
	return result;
}

vector<NetworkInfo> GetNetworkInfo() {
	vector<NetworkInfo> result;
	NetworkInfoHandle handle;
	if (!handle) {
		return result;
	}
	auto count = system_stats_network_info_len(handle.Get());
	result.reserve(count);
	for (size_t index = 0; index < count; index++) {
		auto info = system_stats_network_info_get(handle.Get(), index);
		if (!info) {
			continue;
		}
		NetworkInfo network;
		network.interface_name = CopyString(info->interface_name);
		network.ipv4_address = CopyString(info->ipv4_address);
		network.tx_bytes = info->tx_bytes;
		network.tx_packets = info->tx_packets;
		network.tx_errors = info->tx_errors;
		network.tx_dropped = info->tx_dropped;
		network.rx_bytes = info->rx_bytes;
		network.rx_packets = info->rx_packets;
		network.rx_errors = info->rx_errors;
		network.rx_dropped = info->rx_dropped;
		network.speed_mbps = info->speed_mbps;
		result.emplace_back(std::move(network));
	}
	return result;
}

OSInfo GetOSInfo() {
	OSInfo result;
	OSInfoHandle handle;
	if (!handle) {
		return result;
	}
	auto info = system_stats_os_info_get(handle.Get());
	if (info) {
		result.name = CopyString(info->name);
		result.version = CopyString(info->version);
		result.host_name = CopyString(info->host_name);
		result.handle_count = info->handle_count;
		result.process_count = info->process_count;
		result.thread_count = info->thread_count;
		result.architecture = CopyString(info->architecture);
		result.os_up_since_seconds = info->uptime;
	}
	return result;
}

} // namespace duckdb
