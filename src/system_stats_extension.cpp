#define DUCKDB_EXTENSION_MAIN

#include "system_stats_extension.hpp"

#include "cpu_stats_query_function.hpp"
#include "disk_stats_query_function.hpp"
#include "duckdb.hpp"
#include "memory_stats_query_function.hpp"
#include "network_stats_query_function.hpp"
#include "os_info_query_function.hpp"

namespace duckdb {

namespace {

void LoadInternal(ExtensionLoader &loader) {
	RegisterSysMemoryInfoFunction(loader);
	RegisterSysCPUInfoFunction(loader);
	RegisterSysDiskInfoFunction(loader);
	RegisterSysNetworkInfoFunction(loader);
	RegisterSysOSInfoFunction(loader);
}

} // namespace

void SystemStatsExtension::Load(ExtensionLoader &loader) {
	LoadInternal(loader);
}

string SystemStatsExtension::Name() {
	return "system_stats";
}

string SystemStatsExtension::Version() const {
#ifdef EXT_VERSION_SYSTEM_STATS
	return EXT_VERSION_SYSTEM_STATS;
#else
	return "0.1.0";
#endif
}

} // namespace duckdb

extern "C" {

DUCKDB_CPP_EXTENSION_ENTRY(system_stats, loader) {
	duckdb::LoadInternal(loader);
}
}
