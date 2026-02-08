#include "os_info.hpp"

#include "database_instance_cache.hpp"
#include "duckdb/common/exception.hpp"
#include "duckdb/common/numeric_utils.hpp"
#include "duckdb/common/string.hpp"
#include "duckdb/common/string_util.hpp"
#include "duckdb/logging/logger.hpp"
#include "duckdb/main/client_context.hpp"
#include "scope_guard.hpp"
#include "string_utils.hpp"

#ifdef __linux__
#include <array>
#include <cerrno>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <dirent.h>
#include <fstream>
#include <string_view>
#include <sys/sysinfo.h>
#include <sys/utsname.h>
#include <unistd.h>
#elif __APPLE__
#include <array>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <libproc.h>
#include <sys/proc_info.h>
#include <sys/sysctl.h>
#include <sys/types.h>
#include <sys/utsname.h>
#include <unistd.h>
#endif

namespace duckdb {

namespace {

#ifdef __linux__

struct ProcessStatus {
	int32_t active_processes = 0;
	int32_t running_processes = 0;
	int32_t sleeping_processes = 0;
	int32_t stopped_processes = 0;
	int32_t zombie_processes = 0;
	int32_t total_threads = 0;
};

// Read OS name from /etc/os-release; return empty string if not found.
string ReadOSName(ClientContext &context) {
	// Key-value pair example: PRETTY_NAME="Debian GNU/Linux 13 (trixie)"
	static constexpr std::string_view OS_NAME_PREFIX = "PRETTY_NAME=";

	std::ifstream os_file("/etc/os-release");
	if (!os_file.is_open()) {
		if (auto *db = GetDbInstance(context)) {
			DUCKDB_LOG_DEBUG(*db, "Failed to open /etc/os-release: %s", strerror(errno));
		}
		return "";
	}

	string line;
	while (std::getline(os_file, line)) {
		size_t pos = line.find(OS_NAME_PREFIX);
		if (pos != string::npos) {
			std::string_view line_sv {line};
			std::string_view value_sv = line_sv.substr(pos + OS_NAME_PREFIX.length());
			std::string_view os_name = RemoveQuotes(TrimString(value_sv));
			return string {os_name};
		}
	}

	return "";
}

// Read handle count from /proc/sys/fs/file-nr
int32_t ReadHandleCount(ClientContext &context) {
	std::ifstream file("/proc/sys/fs/file-nr");
	if (!file.is_open()) {
		if (auto *db = GetDbInstance(context)) {
			DUCKDB_LOG_DEBUG(*db, "Failed to open /proc/sys/fs/file-nr: %s", strerror(errno));
		}
		return 0;
	}

	int allocated = 0;
	file >> allocated;
	if (file.fail()) {
		return 0;
	}
	return allocated;
}

// Fallback: Count file descriptors from /proc/*/fd directories
// This is used when /proc/sys/fs/file-nr is not available (e.g., older kernels or restricted environments)
int32_t ReadHandleCountFallback() {
	DIR *dirp = opendir("/proc");
	if (!dirp) {
		return 0;
	}

	SCOPE_EXIT {
		closedir(dirp);
	};

	int32_t handle_count = 0;
	struct dirent *ent = nullptr;
	while ((ent = readdir(dirp)) != nullptr) {
		if (ent->d_name[0] == '\0' || !std::isdigit(static_cast<unsigned char>(ent->d_name[0]))) {
			continue;
		}

		// Count file descriptors in /proc/pid/fd
		string fd_path = StringUtil::Format("/proc/%s/fd", ent->d_name);
		DIR *fd_dirp = opendir(fd_path.c_str());
		if (!fd_dirp) {
			continue;
		}
		SCOPE_EXIT {
			closedir(fd_dirp);
		};

		struct dirent *fd_ent = nullptr;
		while ((fd_ent = readdir(fd_dirp)) != nullptr) {
			if (fd_ent->d_name[0] != '\0' && std::isdigit(static_cast<unsigned char>(fd_ent->d_name[0]))) {
				handle_count++;
			}
		}
	}

	return handle_count;
}

// Read process status from /proc directory
ProcessStatus ReadProcessStatus() {
	ProcessStatus status;

	DIR *dirp = opendir("/proc");
	if (!dirp) {
		return status;
	}
	SCOPE_EXIT {
		closedir(dirp);
	};

	struct dirent *ent = nullptr;
	while ((ent = readdir(dirp)) != nullptr) {
		// Skip non-numeric entries, only PID directories start with digits.
		if (!std::isdigit(static_cast<unsigned char>(ent->d_name[0]))) {
			continue;
		}
		status.active_processes++;
		std::string stat_path = StringUtil::Format("/proc/%s/stat", ent->d_name);
		FILE *stat_file = fopen(stat_path.c_str(), "r");
		if (!stat_file) {
			continue;
		}
		SCOPE_EXIT {
			fclose(stat_file);
		};

		std::array<char, 512> line_buf {};
		if (!fgets(line_buf.data(), line_buf.size(), stat_file)) {
			continue;
		}

		// `line` is our parsing cursor into the stat line; we advance it as we consume fields.
		// Example: [pid] [comm] [state] [ppid] ...
		// Skip the PID field by finding the first space
		char *line = line_buf.data();

		// Skip PID (first field) - find first space
		while (*line != '\0' && *line != ' ' && *line != '\t') {
			line++;
		}
		if (*line == '\0') {
			continue; // malformed line
		}

		// Skip whitespace before '(' of comm field
		while (*line == ' ' || *line == '\t') {
			line++;
		}
		if (*line != '(') {
			continue;
		}
		line++; // skip '('

		// Find the closing ')' of comm (process name field)
		char *comm_end = strrchr(line, ')');
		if (!comm_end) {
			continue; // malformed /proc entry
		}

		// Extract comm if needed, move cursor past ')'.
		line = comm_end + 1;

		// Skip whitespace after comm
		while (*line == ' ' || *line == '\t') {
			line++;
		}
		if (*line == '\0') {
			continue;
		}

		// Parse fields after comm
		char state = '\0';
		int ppid = 0, pgrp = 0, session = 0, tty_nr = 0, tpgid = 0;
		unsigned long flags = 0, minflt = 0, cminflt = 0, majflt = 0, cmajflt = 0;
		unsigned long utime = 0, stime = 0, cutime = 0, cstime = 0, threads = 0;
		long priority = 0, nice = 0;

		int scanned = sscanf(line,
		                     "%c %d %d %d %d %d %lu %lu %lu %lu %lu "
		                     "%lu %lu %lu %lu %ld %ld %lu",
		                     &state, &ppid, &pgrp, &session, &tty_nr, &tpgid, &flags, &minflt, &cminflt, &majflt,
		                     &cmajflt, &utime, &stime, &cutime, &cstime, &priority, &nice, &threads);
		if (scanned < 18) {
			continue; // incomplete record
		}

		// Count processes by state
		switch (state) {
		case 'R':
			status.running_processes++;
			break;
		case 'S':
		case 'D':
			status.sleeping_processes++;
			break;
		case 'T':
			status.stopped_processes++;
			break;
		case 'Z':
			status.zombie_processes++;
			break;
		}

		status.total_threads += threads;
	}

	return status;
}

OSInfo GetOSInfoLinux(ClientContext &context) {
	OSInfo info;

	struct utsname uts;
	if (uname(&uts) != 0) {
		if (auto *db = GetDbInstance(context)) {
			DUCKDB_LOG_DEBUG(*db, "uname() failed: %s", strerror(errno));
		}
	} else {
		info.version = StringUtil::Format("%s %s", uts.sysname, uts.release);
		info.architecture = uts.machine;
	}

	// Get hostname
	std::array<char, 256> hostname_buf {};
	if (gethostname(hostname_buf.data(), hostname_buf.size()) != 0) {
		if (auto *db = GetDbInstance(context)) {
			DUCKDB_LOG_DEBUG(*db, "gethostname() failed: %s", strerror(errno));
		}
	} else {
		info.host_name = hostname_buf.data();
	}

	// Read OS name from /etc/os-release
	info.name = ReadOSName(context);
	if (info.name.empty()) {
		// Fallback to sysname from uname
		if (uname(&uts) == 0) {
			info.name = uts.sysname;
		}
	}

	// Read handle count
	info.handle_count = ReadHandleCount(context);
	if (info.handle_count == 0) {
		// Fallback: count file descriptors from /proc/*/fd if /proc/sys/fs/file-nr is unavailable
		info.handle_count = ReadHandleCountFallback();
	}

	// Read process status
	ProcessStatus proc_status = ReadProcessStatus();
	info.process_count = proc_status.active_processes;
	info.thread_count = proc_status.total_threads;

	// Get uptime
	struct sysinfo s_info;
	if (sysinfo(&s_info) != 0) {
		if (auto *db = GetDbInstance(context)) {
			DUCKDB_LOG_DEBUG(*db, "sysinfo() failed: %s", strerror(errno));
		}
	} else {
		info.os_up_since_seconds = NumericCast<uint64_t>(s_info.uptime);
	}

	return info;
}
#elif __APPLE__
// Get thread count on macOS by summing threads from all processes
int32_t GetThreadCountMacOS(ClientContext &context) {
	std::array<int, 4> mib = {CTL_KERN, KERN_PROC, KERN_PROC_ALL, 0};
	size_t len = 0;
	if (sysctl(mib.data(), mib.size(), nullptr, &len, nullptr, 0) != 0 || len == 0) {
		if (auto *db = GetDbInstance(context)) {
			DUCKDB_LOG_DEBUG(*db, "sysctl() failed to get process list size: %s", strerror(errno));
		}
		return 0;
	}

	char *buf = static_cast<char *>(malloc(len));
	if (buf == nullptr) {
		if (auto *db = GetDbInstance(context)) {
			DUCKDB_LOG_DEBUG(*db, "malloc() failed to allocate %zu bytes for process list", len);
		}
		return 0;
	}

	SCOPE_EXIT {
		free(buf);
	};

	if (sysctl(mib.data(), mib.size(), buf, &len, nullptr, 0) != 0) {
		if (auto *db = GetDbInstance(context)) {
			DUCKDB_LOG_DEBUG(*db, "sysctl() failed to get process list: %s", strerror(errno));
		}
		return 0;
	}

	size_t num_procs = len / sizeof(struct kinfo_proc);
	struct kinfo_proc *procs = reinterpret_cast<struct kinfo_proc *>(buf);
	int32_t total_threads = 0;

	for (size_t idx = 0; idx < num_procs; idx++) {
		pid_t pid = procs[idx].kp_proc.p_pid;
		struct proc_taskinfo info;
		int ret = proc_pidinfo(pid, PROC_PIDTASKINFO, 0, &info, sizeof(info));
		if (ret > 0) {
			total_threads += info.pti_threadnum;
		}
	}

	return total_threads;
}

// Get handle count on macOS by summing file descriptors from all processes
int32_t GetHandleCountMacOS(ClientContext &context) {
	std::array<int, 4> mib = {CTL_KERN, KERN_PROC, KERN_PROC_ALL, 0};
	size_t len = 0;
	if (sysctl(mib.data(), mib.size(), nullptr, &len, nullptr, 0) != 0 || len == 0) {
		if (auto *db = GetDbInstance(context)) {
			DUCKDB_LOG_DEBUG(*db, "sysctl() failed to get process list size: %s", strerror(errno));
		}
		return 0;
	}

	char *buf = static_cast<char *>(malloc(len));
	if (buf == nullptr) {
		if (auto *db = GetDbInstance(context)) {
			DUCKDB_LOG_DEBUG(*db, "malloc() failed to allocate %zu bytes for process list", len);
		}
		return 0;
	}

	SCOPE_EXIT {
		free(buf);
	};

	if (sysctl(mib.data(), mib.size(), buf, &len, nullptr, 0) != 0) {
		if (auto *db = GetDbInstance(context)) {
			DUCKDB_LOG_DEBUG(*db, "sysctl() failed to get process list: %s", strerror(errno));
		}
		return 0;
	}

	size_t num_procs = len / sizeof(struct kinfo_proc);
	struct kinfo_proc *procs = reinterpret_cast<struct kinfo_proc *>(buf);
	int32_t total_handles = 0;

	for (size_t idx = 0; idx < num_procs; idx++) {
		pid_t pid = procs[idx].kp_proc.p_pid;
		int num_fds = proc_pidinfo(pid, PROC_PIDLISTFDS, 0, nullptr, 0);
		if (num_fds > 0) {
			total_handles += num_fds / sizeof(proc_fdinfo);
		}
	}

	return total_handles;
}

OSInfo GetOSInfoMacOS(ClientContext &context) {
	OSInfo info;

	struct utsname uts;
	if (uname(&uts) != 0) {
		if (auto *db = GetDbInstance(context)) {
			DUCKDB_LOG_DEBUG(*db, "uname() failed: %s", strerror(errno));
		}
	} else {
		info.name = uts.sysname;
		info.version = uts.version;
		info.architecture = uts.machine;
	}

	// Get hostname
	std::array<char, 256> hostname_buf;
	if (gethostname(hostname_buf.data(), hostname_buf.size()) != 0) {
		if (auto *db = GetDbInstance(context)) {
			DUCKDB_LOG_DEBUG(*db, "gethostname() failed: %s", strerror(errno));
		}
	} else {
		info.host_name = hostname_buf.data();
	}

	// Get process count using sysctl
	std::array<int, 4> mib = {CTL_KERN, KERN_PROC, KERN_PROC_ALL, 0};
	size_t len = 0;
	if (sysctl(mib.data(), mib.size(), nullptr, &len, nullptr, 0) != 0) {
		if (auto *db = GetDbInstance(context)) {
			DUCKDB_LOG_DEBUG(*db, "sysctl() failed to get process count: %s", strerror(errno));
		}
	} else if (len > 0) {
		info.process_count = NumericCast<int32_t>(len / sizeof(struct kinfo_proc));
	}

	// Get thread count by summing threads from all processes
	info.thread_count = GetThreadCountMacOS(context);

	// Get handle count by summing file descriptors from all processes
	info.handle_count = GetHandleCountMacOS(context);

	// Get uptime using clock_gettime
	struct timespec uptime;
	if (clock_gettime(CLOCK_MONOTONIC_RAW, &uptime) != 0) {
		if (auto *db = GetDbInstance(context)) {
			DUCKDB_LOG_DEBUG(*db, "clock_gettime() failed: %s", strerror(errno));
		}
	} else {
		info.os_up_since_seconds = NumericCast<uint64_t>(uptime.tv_sec);
	}

	return info;
}
#endif

} // namespace

OSInfo GetOSInfo(ClientContext &context) {
#ifdef __linux__
	return GetOSInfoLinux(context);
#elif __APPLE__
	return GetOSInfoMacOS(context);
#else
	throw NotImplementedException("OS information is not supported on this platform");
#endif
}

} // namespace duckdb
