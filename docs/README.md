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
This function returns memory usage information. All values are in bytes.

**Output columns:**
- `total_memory`: Total physical memory in bytes
- `used_memory`: Used physical memory in bytes
- `free_memory`: Free physical memory in bytes
- `cached_memory`: Cached memory in bytes
- `total_swap`: Total swap space in bytes
- `used_swap`: Used swap space in bytes
- `free_swap`: Free swap space in bytes

**Example:**
```sql
SELECT * FROM sys_memory_info();
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
This function returns disk and filesystem information.

**Output columns:**
- `mount_point`: Mount point path
- `file_system`: File system identifier
- `file_system_type`: File system type (e.g., "ext4", "xfs", "apfs")
- `total_space`: Total space in bytes
- `used_space`: Used space in bytes
- `free_space`: Free space in bytes

**Example:**
```sql
SELECT * FROM sys_disk_info();
```

**Note:** Virtual filesystems (e.g., proc, sysfs, devtmpfs) and certain mount points
(e.g., /dev, /proc, /sys) are automatically filtered out from the results.

## Limitations

- Cache sizes may not be available in containerized environments
- Some fields may return NULL on certain platforms where the information is not available
