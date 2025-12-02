# System Statistics
*system_stats* is a DuckDB extension that provides table functions to access system
level statistics that can be used for monitoring. It supports Linux and macOS.

## Building and Installing

### Building
The extension can be built using the standard DuckDB extension build process:

```sh
CMAKE_BUILD_PARALLEL_LEVEL=$(nproc) make reldebug
```

### Installing the Extension
```sql
FORCE INSTALL system_stats FROM community;
LOAD system_stats;
```

## Functions
The following table functions are provided to fetch system level statistics.

### sys_memory_info()
This function returns memory usage information. All values are in bytes by default, but can be specified in other units using the `unit` parameter.

**Parameters:**
- `unit` (optional): Unit for memory values. Supported units: `bytes`, `KB`, `KiB`, `MB`, `MiB`, `GB`, `GiB`, `TB`, `TiB`. Defaults to `bytes`.

**Output columns:**
- `total_memory`: Total physical memory
- `used_memory`: Used physical memory
- `free_memory`: Free physical memory
- `cached_memory`: Cached memory
- `total_swap`: Total swap space
- `used_swap`: Used swap space
- `free_swap`: Free swap space

**Examples:**
```sql
-- Default: bytes
SELECT * FROM sys_memory_info();

-- Specify unit
SELECT * FROM sys_memory_info(unit='GB');
SELECT * FROM sys_memory_info(unit='GiB');
```

### sys_cpu_info()
This function returns CPU information.

**Output columns:**
- `model_name`: CPU model name
- `architecture`: System architecture (e.g., "x86_64", "arm64")
- `logical_processor`: Number of logical processors
- `physical_processor`: Number of physical processors
- `cpu_clock_speed_Hz`: CPU clock speed in hertz
- `l1dcache_size_KiB`: L1 data cache size in kibibytes
- `l1icache_size_KiB`: L1 instruction cache size in kibibytes
- `l2cache_size_KiB`: L2 cache size in kibibytes
- `l3cache_size_KiB`: L3 cache size in kibibytes
- `cpu_byte_order`: Byte order ("Little Endian" or "Big Endian")

**Example:**
```sql
SELECT * FROM sys_cpu_info();
```

**Note:** Cache sizes may return 0 in containerized environments (Docker, VMs) where
the host system information is not exposed to the container.

### sys_disk_info()
This function returns disk and filesystem information. All space values are in bytes by default, but can be specified in other units using the `unit` parameter.

**Parameters:**
- `unit` (optional): Unit for space values. Supported units: `bytes`, `KB`, `KiB`, `MB`, `MiB`, `GB`, `GiB`, `TB`, `TiB`. Defaults to `bytes`.

**Output columns:**
- `mount_point`: Mount point path
- `file_system`: File system identifier
- `file_system_type`: File system type (e.g., "ext4", "xfs", "apfs")
- `total_space`: Total space
- `used_space`: Used space
- `free_space`: Free space

**Examples:**
```sql
-- Default: bytes
SELECT * FROM sys_disk_info();

-- Specify unit
SELECT * FROM sys_disk_info(unit='GB');
SELECT * FROM sys_disk_info(unit='MiB');
```

**Note:** Virtual filesystems (e.g., proc, sysfs, devtmpfs) and certain mount points
(e.g., /dev, /proc, /sys) are automatically filtered out from the results.

### sys_network_info()
This function returns network interface information and statistics.

**Output columns:**
- `interface_name`: Network interface name
- `ip_address`: IPv4 address of the interface
- `tx_bytes`: Total bytes transmitted
- `tx_packets`: Total packets transmitted
- `tx_errors`: Total transmission errors
- `tx_dropped`: Total packets dropped during transmission
- `rx_bytes`: Total bytes received
- `rx_packets`: Total packets received
- `rx_errors`: Total receive errors
- `rx_dropped`: Total packets dropped during receive
- `link_speed_mbps`: Link speed in megabits per second (0 if not available)

**Example:**
```sql
SELECT * FROM sys_network_info();
```

**Note:** On macOS, `tx_dropped` and `link_speed_mbps` may return 0 as these values are not available through the system APIs.

### sys_os_info()
This function returns operating system information.

**Output columns:**
- `name`: Operating system name
- `version`: Operating system version
- `host_name`: System hostname
- `domain_name`: DNS domain name
- `handle_count`: Number of open file handles (Linux only, 0 on macOS)
- `process_count`: Total number of processes
- `thread_count`: Total number of threads (Linux only, 0 on macOS)
- `architecture`: System architecture
- `os_up_since_seconds`: System uptime in seconds

**Example:**
```sql
SELECT * FROM sys_os_info();
```

**Note:** 
- On macOS, `handle_count` and `thread_count` return 0 as these values are not available through the system APIs.
- `domain_name` may be empty in containerized environments (e.g., Docker) where no domain is configured.

## Limitations

- Cache sizes may not be available in containerized environments
- Some fields may return NULL on certain platforms where the information is not available
