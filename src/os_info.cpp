#include "os_info.hpp"

#include "duckdb/common/exception.hpp"
#include "duckdb/common/string.hpp"
#include "duckdb/common/string_util.hpp"
#include "scope_guard.hpp"
#include "string_utils.hpp"

#ifdef __linux__
#include <array>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <dirent.h>
#include <fstream>
#include <sys/sysinfo.h>
#include <sys/utsname.h>
#include <unistd.h>
#elif __APPLE__
#include <array>
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
// Read OS name from /etc/os-release
bool ReadOSName(string &os_name) {
	std::ifstream os_file("/etc/os-release");
	if (!os_file.is_open()) {
		return false;
	}

	string line;
	while (std::getline(os_file, line)) {
		if (line.find("PRETTY_NAME=") != string::npos) {
			size_t pos = line.find("PRETTY_NAME=");
			string value = line.substr(pos + 12);
			os_name = RemoveQuotes(TrimString(value));
			return true;
		}
	}

	return false;
}

// Read handle count from /proc/sys/fs/file-nr
bool ReadHandleCount(int32_t &handle_count) {
	std::ifstream file("/proc/sys/fs/file-nr");
	if (!file.is_open()) {
		return false;
	}

	int allocated = 0;
	int unallocated = 0;
	int max_handles = 0;
	file >> allocated >> unallocated >> max_handles;
	if (file.fail()) {
		return false;
	}

	handle_count = allocated;
	return true;
}

// Read process status from /proc directory
bool ReadProcessStatus(int32_t &active_processes, int32_t &running_processes, int32_t &sleeping_processes,
                       int32_t &stopped_processes, int32_t &zombie_processes, int32_t &total_threads) {
	DIR *dirp = opendir("/proc");
	if (!dirp) {
		return false;
	}

	SCOPE_EXIT {
		closedir(dirp);
	};

	active_processes = 0;
	running_processes = 0;
	sleeping_processes = 0;
	stopped_processes = 0;
	zombie_processes = 0;
	total_threads = 0;

	struct dirent *ent;
	while ((ent = readdir(dirp)) != nullptr) {
		if (!std::isdigit(static_cast<unsigned char>(ent->d_name[0]))) {
			continue;
		}

		active_processes++;

		string stat_path = StringUtil::Format("/proc/%s/stat", ent->d_name);
		FILE *stat_file = fopen(stat_path.c_str(), "r");
		if (!stat_file) {
			continue;
		}

		SCOPE_EXIT {
			fclose(stat_file);
		};

		// Parse stat file format:
		// pid comm state ppid pgrp session tty_nr tpgid flags minflt cminflt majflt cmajflt
		// utime stime cutime cstime priority nice num_threads ...
		// State is 3rd field, threads is 20th field
		int pid, ppid, pgrp, session, tty_nr, tpgid;
		unsigned long flags, minflt, cminflt, majflt, cmajflt;
		unsigned long utime, stime, cutime, cstime;
		long priority, nice;
		char state = '\0';
		unsigned long threads = 0;
		std::array<char, 256> comm;

		// Use fscanf to parse the stat file
		if (fscanf(stat_file, "%d %s %c %d %d %d %d %d %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %ld %ld %lu", &pid,
		           comm.data(), &state, &ppid, &pgrp, &session, &tty_nr, &tpgid, &flags, &minflt, &cminflt, &majflt,
		           &cmajflt, &utime, &stime, &cutime, &cstime, &priority, &nice, &threads) < 20) {
			continue;
		}

		if (state == 'R') {
			running_processes++;
		} else if (state == 'S' || state == 'D') {
			sleeping_processes++;
		} else if (state == 'T') {
			stopped_processes++;
		} else if (state == 'Z') {
			zombie_processes++;
		}

		total_threads += threads;
	}

	return true;
}

OSInfo GetOSInfoLinux() {
	OSInfo info;

	struct utsname uts;
	if (uname(&uts) == 0) {
		info.version = StringUtil::Format("%s %s", uts.sysname, uts.release);
		info.architecture = uts.machine;
	}

	// Get hostname
	std::array<char, 256> hostname_buf;
	if (gethostname(hostname_buf.data(), hostname_buf.size()) == 0) {
		info.host_name = hostname_buf.data();
	}

	// Read OS name from /etc/os-release
	if (!ReadOSName(info.name)) {
		// Fallback to sysname from uname
		if (uname(&uts) == 0) {
			info.name = uts.sysname;
		}
	}

	// Read handle count
	ReadHandleCount(info.handle_count);

	// Read process status
	int32_t running = 0, sleeping = 0, stopped = 0, zombie = 0;
	if (ReadProcessStatus(info.process_count, running, sleeping, stopped, zombie, info.thread_count)) {
		// process_count already set
	}

	// Get uptime
	struct sysinfo s_info;
	if (sysinfo(&s_info) == 0) {
		info.os_up_since_seconds = static_cast<int32_t>(s_info.uptime);
	}

	return info;
}
#elif __APPLE__
// Get thread count on macOS by summing threads from all processes
int32_t GetThreadCountMacOS() {
	std::array<int, 4> mib = {CTL_KERN, KERN_PROC, KERN_PROC_ALL, 0};
	size_t len = 0;
	if (sysctl(mib.data(), mib.size(), nullptr, &len, nullptr, 0) != 0 || len == 0) {
		return 0;
	}

	char *buf = static_cast<char *>(malloc(len));
	if (buf == nullptr) {
		return 0;
	}

	SCOPE_EXIT {
		free(buf);
	};

	if (sysctl(mib.data(), mib.size(), buf, &len, nullptr, 0) != 0) {
		return 0;
	}

	size_t num_procs = len / sizeof(struct kinfo_proc);
	struct kinfo_proc *procs = reinterpret_cast<struct kinfo_proc *>(buf);
	int32_t total_threads = 0;
	for (size_t i = 0; i < num_procs; i++) {
		total_threads += procs[i].kp_proc.p_nthreads;
	}

	return total_threads;
}

// Get handle count on macOS by summing file descriptors from all processes
int32_t GetHandleCountMacOS() {
	std::array<int, 4> mib = {CTL_KERN, KERN_PROC, KERN_PROC_ALL, 0};
	size_t len = 0;
	if (sysctl(mib.data(), mib.size(), nullptr, &len, nullptr, 0) != 0 || len == 0) {
		return 0;
	}

	char *buf = static_cast<char *>(malloc(len));
	if (buf == nullptr) {
		return 0;
	}

	SCOPE_EXIT {
		free(buf);
	};

	if (sysctl(mib.data(), mib.size(), buf, &len, nullptr, 0) != 0) {
		return 0;
	}

	size_t num_procs = len / sizeof(struct kinfo_proc);
	struct kinfo_proc *procs = reinterpret_cast<struct kinfo_proc *>(buf);
	int32_t total_handles = 0;

	for (size_t i = 0; i < num_procs; i++) {
		pid_t pid = procs[i].kp_proc.p_pid;
		int num_fds = proc_pidinfo(pid, PROC_PIDLISTFDS, 0, nullptr, 0);
		if (num_fds > 0) {
			total_handles += num_fds / sizeof(proc_fdinfo);
		}
	}

	return total_handles;
}

OSInfo GetOSInfoMacOS() {
	OSInfo info;

	struct utsname uts;
	if (uname(&uts) == 0) {
		info.name = uts.sysname;
		info.version = uts.version;
		info.architecture = uts.machine;
	}

	// Get hostname
	std::array<char, 256> hostname_buf;
	if (gethostname(hostname_buf.data(), hostname_buf.size()) == 0) {
		info.host_name = hostname_buf.data();
	}

	// Get process count using sysctl
	std::array<int, 4> mib = {CTL_KERN, KERN_PROC, KERN_PROC_ALL, 0};
	size_t len = 0;
	if (sysctl(mib.data(), mib.size(), nullptr, &len, nullptr, 0) == 0 && len > 0) {
		info.process_count = static_cast<int32_t>(len / sizeof(struct kinfo_proc));
	}

	// Get thread count by summing threads from all processes
	info.thread_count = GetThreadCountMacOS();

	// Get handle count by summing file descriptors from all processes
	info.handle_count = GetHandleCountMacOS();

	// Get uptime using clock_gettime
	struct timespec uptime;
	if (clock_gettime(CLOCK_MONOTONIC_RAW, &uptime) == 0) {
		info.os_up_since_seconds = static_cast<int32_t>(uptime.tv_sec);
	}

	return info;
}
#endif

} // namespace

OSInfo GetOSInfo() {
#ifdef __linux__
	return GetOSInfoLinux();
#elif __APPLE__
	return GetOSInfoMacOS();
#else
	throw NotImplementedException("OS information is not supported on this platform");
#endif
}

} // namespace duckdb
