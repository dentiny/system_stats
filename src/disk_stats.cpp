// TODO(hjiang): Add system stats for containerized environments.

#include "disk_stats.hpp"

#include "duckdb/common/exception.hpp"
#include "duckdb/common/string.hpp"
#include "duckdb/common/string_util.hpp"

#ifdef __linux__
#include <mntent.h>
#include <regex.h>
#include <sys/statvfs.h>
#include <cstring>
#elif __APPLE__
#include <sys/mount.h>
#include <sys/statvfs.h>
#include <cstring>
#endif

namespace duckdb {

namespace {

#ifdef __linux__
// Regex patterns to ignore virtual filesystem types
const char *IGNORE_FILE_SYSTEM_TYPE_REGEX =
    "^(autofs|binfmt_misc|bpf|cgroup2?|configfs|debugfs|devpts|devtmpfs|fusectl|hugetlbfs|iso9660|mqueue|nsfs|overlay|"
    "proc|procfs|pstore|rpc_pipefs|securityfs|selinuxfs|squashfs|sysfs|tracefs)$";
const char *IGNORE_MOUNT_POINTS_REGEX = "^/(dev|proc|sys|run|snap|var/lib/docker/.+)($|/)";

bool IgnoreFileSystemType(const string &fs_type) {
	regex_t regex;
	int reg_return = regcomp(&regex, IGNORE_FILE_SYSTEM_TYPE_REGEX, REG_EXTENDED);
	if (reg_return != 0) {
		return false;
	}

	reg_return = regexec(&regex, fs_type.c_str(), 0, NULL, 0);
	bool ret = (reg_return == 0);
	regfree(&regex);
	return ret;
}

bool IgnoreMountPoint(const string &mount_point) {
	regex_t regex;
	int reg_return = regcomp(&regex, IGNORE_MOUNT_POINTS_REGEX, REG_EXTENDED);
	if (reg_return != 0) {
		return false;
	}

	reg_return = regexec(&regex, mount_point.c_str(), 0, NULL, 0);
	bool ret = (reg_return == 0);
	regfree(&regex);
	return ret;
}

std::vector<DiskInfo> GetDiskInfoLinux() {
	std::vector<DiskInfo> disks;

	FILE *fp = setmntent("/etc/mtab", "r");
	if (!fp) {
		// Fallback to /proc/mounts if /etc/mtab doesn't exist
		fp = setmntent("/proc/mounts", "r");
	}

	if (!fp) {
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
			continue;
		}

		uint64_t total_space = static_cast<uint64_t>(buf.f_blocks * buf.f_bsize);
		if (total_space == 0) {
			continue;
		}

		DiskInfo info;
		info.mount_point = mount_point;
		info.file_system = ent->mnt_fsname;
		info.file_system_type = fs_type;
		info.total_space = total_space;
		info.used_space = static_cast<uint64_t>((buf.f_blocks - buf.f_bfree) * buf.f_bsize);
		info.free_space = static_cast<uint64_t>(buf.f_bavail * buf.f_bsize);
		info.total_inodes = static_cast<uint64_t>(buf.f_files);
		info.free_inodes = static_cast<uint64_t>(buf.f_ffree);
		info.used_inodes = info.total_inodes - info.free_inodes;

		disks.push_back(info);
	}

	endmntent(fp);
	return disks;
}
#endif

#ifdef __APPLE__
std::vector<DiskInfo> GetDiskInfoMacOS() {
	std::vector<DiskInfo> disks;

	struct statfs *mntbuf;
	int count = getmntinfo(&mntbuf, MNT_NOWAIT);
	if (count <= 0) {
		return disks;
	}

	for (int i = 0; i < count; i++) {
		struct statvfs buf;
		if (statvfs(mntbuf[i].f_mntonname, &buf) != 0) {
			continue;
		}

		uint64_t total_space = static_cast<uint64_t>(buf.f_blocks * buf.f_bsize);
		if (total_space == 0) {
			continue;
		}

		DiskInfo info;
		info.mount_point = mntbuf[i].f_mntonname;
		info.file_system = mntbuf[i].f_mntfromname;
		info.file_system_type = mntbuf[i].f_fstypename;
		info.total_space = total_space;
		info.used_space = static_cast<uint64_t>((buf.f_blocks - buf.f_bfree) * buf.f_bsize);
		info.free_space = static_cast<uint64_t>(buf.f_bavail * buf.f_bsize);
		info.total_inodes = static_cast<uint64_t>(buf.f_files);
		info.free_inodes = static_cast<uint64_t>(buf.f_ffree);
		info.used_inodes = info.total_inodes - info.free_inodes;

		disks.push_back(info);
	}

	return disks;
}
#endif

} // namespace

std::vector<DiskInfo> GetDiskInfo() {
#ifdef __linux__
	return GetDiskInfoLinux();
#elif __APPLE__
	return GetDiskInfoMacOS();
#else
	throw NotImplementedException("Disk statistics are not supported on this platform");
#endif
}

} // namespace duckdb
