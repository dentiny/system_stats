#define DUCKDB_EXTENSION_MAIN

#include "system_stats_extension.hpp"

#include "system_stats_functions.hpp"

namespace duckdb {

static void LoadInternal(ExtensionLoader &loader) {
	RegisterSystemStatsFunctions(loader);
	loader.SetDescription(
	    "Provides system information functions including CPU, memory, disk, network, and OS statistics");
}

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
	return "1.0.0";
#endif
}

} // namespace duckdb

extern "C" {

DUCKDB_CPP_EXTENSION_ENTRY(system_stats, loader) {
	duckdb::LoadInternal(loader);
}
}
