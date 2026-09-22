use std::net::IpAddr;

use sysinfo::{Disks, Networks, System};

#[derive(Debug)]
pub struct CpuInfo {
    pub model_name: String,
    pub architecture: String,
    pub logical_processors: i32,
    pub physical_processors: i32,
    pub byte_order: String,
}

#[derive(Debug)]
pub struct MemoryInfo {
    pub total: u64,
    pub used: u64,
    pub free: u64,
    pub cached: u64,
    pub total_swap: u64,
    pub used_swap: u64,
    pub free_swap: u64,
}

#[derive(Debug)]
pub struct DiskInfo {
    pub mount_point: String,
    pub file_system: String,
    pub file_system_type: String,
    pub total: u64,
    pub used: u64,
    pub free: u64,
}

#[derive(Debug)]
pub struct NetworkInfo {
    pub interface_name: String,
    pub ip_address: String,
    pub tx_bytes: u64,
    pub tx_packets: u64,
    pub tx_errors: u64,
    pub tx_dropped: u64,
    pub rx_bytes: u64,
    pub rx_packets: u64,
    pub rx_errors: u64,
    pub rx_dropped: u64,
    pub link_speed_mbps: u64,
}

#[derive(Debug)]
pub struct OsInfo {
    pub name: String,
    pub version: String,
    pub host_name: String,
    pub handle_count: i32,
    pub process_count: i32,
    pub thread_count: i32,
    pub architecture: String,
    pub uptime: u64,
}

pub fn cpu_info() -> CpuInfo {
    let system = System::new_all();
    let logical_processors = system.cpus().len();
    let physical_processors = System::physical_core_count().unwrap_or(logical_processors);
    let model_name = system
        .cpus()
        .first()
        .map(|cpu| cpu.brand().to_owned())
        .unwrap_or_default();

    CpuInfo {
        model_name,
        architecture: System::cpu_arch(),
        logical_processors: saturating_i32(logical_processors),
        physical_processors: saturating_i32(physical_processors),
        byte_order: if cfg!(target_endian = "little") {
            "Little Endian".to_owned()
        } else {
            "Big Endian".to_owned()
        },
    }
}

pub fn memory_info() -> MemoryInfo {
    let system = System::new_all();
    MemoryInfo {
        total: system.total_memory(),
        used: system.used_memory(),
        free: system.free_memory(),
        // sysinfo deliberately does not expose a portable cache metric.
        cached: 0,
        total_swap: system.total_swap(),
        used_swap: system.used_swap(),
        free_swap: system.free_swap(),
    }
}

pub fn disk_info() -> Vec<DiskInfo> {
    Disks::new_with_refreshed_list()
        .list()
        .iter()
        .filter_map(|disk| {
            let mount_point = disk.mount_point().to_string_lossy().into_owned();
            let file_system_type = disk.file_system().to_string_lossy().into_owned();
            let total = disk.total_space();
            if total == 0 || ignored_file_system(&file_system_type) || ignored_mount(&mount_point) {
                return None;
            }
            let free = disk.available_space();
            Some(DiskInfo {
                mount_point,
                file_system: disk.name().to_string_lossy().into_owned(),
                file_system_type,
                total,
                used: total.saturating_sub(free),
                free,
            })
        })
        .collect()
}

pub fn network_info() -> Vec<NetworkInfo> {
    Networks::new_with_refreshed_list()
        .iter()
        .map(|(name, network)| NetworkInfo {
            interface_name: name.clone(),
            ip_address: network
                .ip_networks()
                .iter()
                .find_map(|network| match network.addr {
                    IpAddr::V4(address) => Some(address.to_string()),
                    IpAddr::V6(_) => None,
                })
                .unwrap_or_default(),
            tx_bytes: network.total_transmitted(),
            tx_packets: network.total_packets_transmitted(),
            tx_errors: network.total_errors_on_transmitted(),
            tx_dropped: 0,
            rx_bytes: network.total_received(),
            rx_packets: network.total_packets_received(),
            rx_errors: network.total_errors_on_received(),
            rx_dropped: 0,
            // There is no portable API for interface link speed.
            link_speed_mbps: 0,
        })
        .collect()
}

pub fn os_info() -> OsInfo {
    let system = System::new_all();
    let process_count = system.processes().len();
    let handle_count: usize = system
        .processes()
        .values()
        .filter_map(|process| process.open_files())
        .sum();
    let thread_count: usize = system
        .processes()
        .values()
        .map(|process| process.tasks().map_or(1, |tasks| tasks.len().max(1)))
        .sum();

    OsInfo {
        name: System::name().unwrap_or_else(|| std::env::consts::OS.to_owned()),
        version: System::long_os_version().unwrap_or_else(|| "Unknown".to_owned()),
        host_name: System::host_name().unwrap_or_default(),
        handle_count: saturating_i32(handle_count),
        process_count: saturating_i32(process_count),
        thread_count: saturating_i32(thread_count),
        architecture: System::cpu_arch(),
        uptime: System::uptime(),
    }
}

fn saturating_i32(value: usize) -> i32 {
    i32::try_from(value).unwrap_or(i32::MAX)
}

fn ignored_file_system(file_system: &str) -> bool {
    matches!(
        file_system.to_ascii_lowercase().as_str(),
        "autofs"
            | "binfmt_misc"
            | "bpf"
            | "cgroup"
            | "cgroup2"
            | "configfs"
            | "debugfs"
            | "devpts"
            | "devtmpfs"
            | "fusectl"
            | "hugetlbfs"
            | "iso9660"
            | "mqueue"
            | "nsfs"
            | "overlay"
            | "proc"
            | "procfs"
            | "pstore"
            | "rpc_pipefs"
            | "securityfs"
            | "selinuxfs"
            | "squashfs"
            | "sysfs"
            | "tracefs"
    )
}

fn ignored_mount(mount_point: &str) -> bool {
    ["/dev", "/proc", "/sys", "/run", "/snap", "/var/lib/docker"]
        .iter()
        .any(|prefix| {
            mount_point == *prefix
                || mount_point
                    .strip_prefix(prefix)
                    .is_some_and(|suffix| suffix.starts_with('/'))
        })
}

#[cfg(test)]
mod tests {
    use super::{cpu_info, disk_info, memory_info, network_info, os_info};

    #[test]
    fn collects_host_statistics() {
        assert!(cpu_info().logical_processors > 0);
        assert!(memory_info().total > 0);
        assert!(!disk_info().is_empty());
        assert!(!network_info().is_empty());
        assert!(!os_info().name.is_empty());
    }
}
