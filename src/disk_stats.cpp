// TODO(hjiang): Add system stats for containerized environments.

#include "disk_stats.hpp"

#include <cstring>
#include <regex>

#include "database_instance_cache.hpp"
#include "duckdb/common/exception.hpp"
#include "duckdb/common/numeric_utils.hpp"
#include "duckdb/common/string.hpp"
#include "duckdb/common/string_util.hpp"
#include "duckdb/common/types.hpp"
#include "duckdb/logging/logger.hpp"
#include "duckdb/main/client_context.hpp"

#ifdef __linux__
#include <cerrno>
#include <mntent.h>
#include <sys/statvfs.h>
#elif __APPLE__
#include <cerrno>
#include <sys/mount.h>
#include <sys/statvfs.h>
#endif

namespace duckdb {

namespace {

// Regex patterns to ignore virtual filesystem types
constexpr const char *IGNORE_FILE_SYSTEM_TYPE_REGEX =
    "^(autofs|binfmt_misc|bpf|cgroup2?|configfs|debugfs|devpts|devtmpfs|fusectl|hugetlbfs|iso9660|mqueue|nsfs|overlay|"
    "proc|procfs|pstore|rpc_pipefs|securityfs|selinuxfs|squashfs|sysfs|tracefs)$";
constexpr const char *IGNORE_MOUNT_POINTS_REGEX = "^/(dev|proc|sys|run|snap|var/lib/docker/.+)($|/)";

bool IgnoreFileSystemType(const string &fs_type) {
	static const std::regex pattern(IGNORE_FILE_SYSTEM_TYPE_REGEX, std::regex_constants::extended);
	try {
		return std::regex_match(fs_type, pattern);
	} catch (const std::regex_error &) {
		return false;
	}
}

bool IgnoreMountPoint(const string &mount_point) {
	static const std::regex pattern(IGNORE_MOUNT_POINTS_REGEX, std::regex_constants::extended);
	try {
		return std::regex_match(mount_point, pattern);
	} catch (const std::regex_error &) {
		return false;
	}
}

#ifdef __linux__

std::vector<DiskInfo> GetDiskInfoLinux(ClientContext &context) {
	std::vector<DiskInfo> disks;

	FILE *fp = setmntent("/etc/mtab", "r");
	if (!fp) {
		// Fallback to /proc/mounts if /etc/mtab doesn't exist
		fp = setmntent("/proc/mounts", "r");
	}

	if (!fp) {
		if (auto *db = GetDbInstance(context)) {
			DUCKDB_LOG_DEBUG(*db, "Failed to open /etc/mtab and /proc/mounts: %s", strerror(errno));
		}
		return disks;
	}

	struct mntent *ent = nullptr;
	struct statvfs buf;

	while ((ent = getmntent(fp)) != NULL) {
		string fs_type = ent->mnt_type;
		string mount_point = ent->mnt_dir;

		// Skip ignored filesystem types and mount points
		if (IgnoreFileSystemType(fs_type) || IgnoreMountPoint(mount_point)) {
			continue;
		}

		memset(&buf, 0, sizeof(buf));
		if (statvfs(mount_point.c_str(), &buf) != 0) {
			if (auto *db = GetDbInstance(context)) {
				DUCKDB_LOG_DEBUG(*db, "statvfs() failed for %s: %s", mount_point.c_str(), strerror(errno));
			}
			continue;
		}

		uint64_t total_space = NumericCast<uint64_t>(buf.f_blocks * buf.f_bsize);
		if (total_space == 0) {
			continue;
		}

		DiskInfo info;
		info.mount_point = mount_point;
		info.file_system = ent->mnt_fsname;
		info.file_system_type = fs_type;
		info.total_space = total_space;
		info.used_space = NumericCast<uint64_t>((buf.f_blocks - buf.f_bfree) * buf.f_bsize);
		info.free_space = NumericCast<uint64_t>(buf.f_bavail * buf.f_bsize);

		disks.emplace_back(info);
	}

	endmntent(fp);
	return disks;
}
#endif

#ifdef __APPLE__
std::vector<DiskInfo> GetDiskInfoMacOS(ClientContext &context) {
	std::vector<DiskInfo> disks;

	struct statfs *mntbuf;
	int count = getmntinfo(&mntbuf, MNT_NOWAIT);
	if (count <= 0) {
		if (auto *db = GetDbInstance(context)) {
			DUCKDB_LOG_DEBUG(*db, "getmntinfo() failed: %s", strerror(errno));
		}
		return disks;
	}

	for (idx_t idx = 0; idx < count; idx++) {
		string fs_type = mntbuf[idx].f_fstypename;
		string mount_point = mntbuf[idx].f_mntonname;

		// Skip ignored filesystem types and mount points
		if (IgnoreFileSystemType(fs_type) || IgnoreMountPoint(mount_point)) {
			continue;
		}

		struct statvfs buf;
		if (statvfs(mount_point.c_str(), &buf) != 0) {
			if (auto *db = GetDbInstance(context)) {
				DUCKDB_LOG_DEBUG(*db, "statvfs() failed for %s: %s", mount_point.c_str(), strerror(errno));
			}
			continue;
		}

		uint64_t total_space = NumericCast<uint64_t>(buf.f_blocks * buf.f_bsize);
		if (total_space == 0) {
			continue;
		}

		DiskInfo info;
		info.mount_point = mount_point;
		info.file_system = mntbuf[idx].f_mntfromname;
		info.file_system_type = fs_type;
		info.total_space = total_space;
		info.used_space = NumericCast<uint64_t>((buf.f_blocks - buf.f_bfree) * buf.f_bsize);
		info.free_space = NumericCast<uint64_t>(buf.f_bavail * buf.f_bsize);

		disks.emplace_back(info);
	}

	return disks;
}
#endif

} // namespace

std::vector<DiskInfo> GetDiskInfo(ClientContext &context) {
#ifdef __linux__
	return GetDiskInfoLinux(context);
#elif __APPLE__
	return GetDiskInfoMacOS(context);
#else
	throw NotImplementedException("Disk statistics are not supported on this platform");
#endif
}

} // namespace duckdb
