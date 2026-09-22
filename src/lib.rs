//! Cross-platform system-statistics implementation exposed through a C ABI.
#![allow(clippy::missing_safety_doc)]

mod stats;

use std::{
    ffi::{CString, c_char},
    ptr,
};

#[repr(C)]
pub struct RustCpuInfo {
    model_name: *const c_char,
    architecture: *const c_char,
    logical_cpus: i32,
    physical_cpus: i32,
    l1d_cache_kb: i32,
    l1i_cache_kb: i32,
    l2_cache_kb: i32,
    l3_cache_kb: i32,
    byte_order: *const c_char,
}

pub struct RustCpuInfoHandle {
    row: RustCpuInfo,
    _strings: Vec<CString>,
}

#[repr(C)]
pub struct RustMemoryInfo {
    total_memory: u64,
    used_memory: u64,
    free_memory: u64,
    total_swap: u64,
    used_swap: u64,
    free_swap: u64,
    cached_memory: u64,
}

#[repr(C)]
pub struct RustDiskInfo {
    mount_point: *const c_char,
    file_system: *const c_char,
    file_system_type: *const c_char,
    total_space: u64,
    used_space: u64,
    free_space: u64,
}

pub struct RustDiskInfoList {
    rows: Vec<RustDiskInfo>,
    _strings: Vec<CString>,
}

#[repr(C)]
pub struct RustNetworkInfo {
    interface_name: *const c_char,
    ipv4_address: *const c_char,
    tx_bytes: u64,
    tx_packets: u64,
    tx_errors: u64,
    tx_dropped: u64,
    rx_bytes: u64,
    rx_packets: u64,
    rx_errors: u64,
    rx_dropped: u64,
    speed_mbps: u64,
}

pub struct RustNetworkInfoList {
    rows: Vec<RustNetworkInfo>,
    _strings: Vec<CString>,
}

#[repr(C)]
pub struct RustOsInfo {
    name: *const c_char,
    version: *const c_char,
    host_name: *const c_char,
    handle_count: i32,
    process_count: i32,
    thread_count: i32,
    architecture: *const c_char,
    uptime: u64,
}

pub struct RustOsInfoHandle {
    row: RustOsInfo,
    _strings: Vec<CString>,
}

fn c_string(value: String) -> CString {
    CString::new(value.replace('\0', "\u{fffd}")).expect("replacement removes NUL bytes")
}

#[unsafe(no_mangle)]
pub extern "C" fn system_stats_cpu_info() -> *mut RustCpuInfoHandle {
    std::panic::catch_unwind(|| {
        let info = stats::cpu_info();
        let strings = vec![
            c_string(info.model_name),
            c_string(info.architecture),
            c_string(info.byte_order),
        ];
        let row = RustCpuInfo {
            model_name: strings[0].as_ptr(),
            architecture: strings[1].as_ptr(),
            logical_cpus: info.logical_processors,
            physical_cpus: info.physical_processors,
            l1d_cache_kb: 0,
            l1i_cache_kb: 0,
            l2_cache_kb: 0,
            l3_cache_kb: 0,
            byte_order: strings[2].as_ptr(),
        };
        Box::into_raw(Box::new(RustCpuInfoHandle {
            row,
            _strings: strings,
        }))
    })
    .unwrap_or(ptr::null_mut())
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn system_stats_cpu_info_get(
    handle: *const RustCpuInfoHandle,
) -> *const RustCpuInfo {
    unsafe { handle.as_ref() }.map_or(ptr::null(), |handle| &handle.row)
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn system_stats_cpu_info_free(handle: *mut RustCpuInfoHandle) {
    if !handle.is_null() {
        drop(unsafe { Box::from_raw(handle) });
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn system_stats_memory_info() -> RustMemoryInfo {
    let info = std::panic::catch_unwind(stats::memory_info).unwrap_or(stats::MemoryInfo {
        total: 0,
        used: 0,
        free: 0,
        cached: 0,
        total_swap: 0,
        used_swap: 0,
        free_swap: 0,
    });
    RustMemoryInfo {
        total_memory: info.total,
        used_memory: info.used,
        free_memory: info.free,
        total_swap: info.total_swap,
        used_swap: info.used_swap,
        free_swap: info.free_swap,
        cached_memory: info.cached,
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn system_stats_disk_info() -> *mut RustDiskInfoList {
    std::panic::catch_unwind(|| {
        let disks = stats::disk_info();
        let mut strings = Vec::with_capacity(disks.len() * 3);
        let mut rows = Vec::with_capacity(disks.len());
        for disk in disks {
            let offset = strings.len();
            strings.push(c_string(disk.mount_point));
            strings.push(c_string(disk.file_system));
            strings.push(c_string(disk.file_system_type));
            rows.push(RustDiskInfo {
                mount_point: strings[offset].as_ptr(),
                file_system: strings[offset + 1].as_ptr(),
                file_system_type: strings[offset + 2].as_ptr(),
                total_space: disk.total,
                used_space: disk.used,
                free_space: disk.free,
            });
        }
        Box::into_raw(Box::new(RustDiskInfoList {
            rows,
            _strings: strings,
        }))
    })
    .unwrap_or(ptr::null_mut())
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn system_stats_disk_info_len(list: *const RustDiskInfoList) -> usize {
    unsafe { list.as_ref() }.map_or(0, |list| list.rows.len())
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn system_stats_disk_info_get(
    list: *const RustDiskInfoList,
    index: usize,
) -> *const RustDiskInfo {
    unsafe { list.as_ref() }
        .and_then(|list| list.rows.get(index))
        .map_or(ptr::null(), |row| row)
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn system_stats_disk_info_free(list: *mut RustDiskInfoList) {
    if !list.is_null() {
        drop(unsafe { Box::from_raw(list) });
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn system_stats_network_info() -> *mut RustNetworkInfoList {
    std::panic::catch_unwind(|| {
        let networks = stats::network_info();
        let mut strings = Vec::with_capacity(networks.len() * 2);
        let mut rows = Vec::with_capacity(networks.len());
        for network in networks {
            let offset = strings.len();
            strings.push(c_string(network.interface_name));
            strings.push(c_string(network.ip_address));
            rows.push(RustNetworkInfo {
                interface_name: strings[offset].as_ptr(),
                ipv4_address: strings[offset + 1].as_ptr(),
                tx_bytes: network.tx_bytes,
                tx_packets: network.tx_packets,
                tx_errors: network.tx_errors,
                tx_dropped: network.tx_dropped,
                rx_bytes: network.rx_bytes,
                rx_packets: network.rx_packets,
                rx_errors: network.rx_errors,
                rx_dropped: network.rx_dropped,
                speed_mbps: network.link_speed_mbps,
            });
        }
        Box::into_raw(Box::new(RustNetworkInfoList {
            rows,
            _strings: strings,
        }))
    })
    .unwrap_or(ptr::null_mut())
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn system_stats_network_info_len(list: *const RustNetworkInfoList) -> usize {
    unsafe { list.as_ref() }.map_or(0, |list| list.rows.len())
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn system_stats_network_info_get(
    list: *const RustNetworkInfoList,
    index: usize,
) -> *const RustNetworkInfo {
    unsafe { list.as_ref() }
        .and_then(|list| list.rows.get(index))
        .map_or(ptr::null(), |row| row)
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn system_stats_network_info_free(list: *mut RustNetworkInfoList) {
    if !list.is_null() {
        drop(unsafe { Box::from_raw(list) });
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn system_stats_os_info() -> *mut RustOsInfoHandle {
    std::panic::catch_unwind(|| {
        let info = stats::os_info();
        let strings = vec![
            c_string(info.name),
            c_string(info.version),
            c_string(info.host_name),
            c_string(info.architecture),
        ];
        let row = RustOsInfo {
            name: strings[0].as_ptr(),
            version: strings[1].as_ptr(),
            host_name: strings[2].as_ptr(),
            handle_count: info.handle_count,
            process_count: info.process_count,
            thread_count: info.thread_count,
            architecture: strings[3].as_ptr(),
            uptime: info.uptime,
        };
        Box::into_raw(Box::new(RustOsInfoHandle {
            row,
            _strings: strings,
        }))
    })
    .unwrap_or(ptr::null_mut())
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn system_stats_os_info_get(
    handle: *const RustOsInfoHandle,
) -> *const RustOsInfo {
    unsafe { handle.as_ref() }.map_or(ptr::null(), |handle| &handle.row)
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn system_stats_os_info_free(handle: *mut RustOsInfoHandle) {
    if !handle.is_null() {
        drop(unsafe { Box::from_raw(handle) });
    }
}
