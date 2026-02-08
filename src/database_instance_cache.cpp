#include "database_instance_cache.hpp"
#include "duckdb/storage/object_cache.hpp"

namespace duckdb {

DatabaseInstance *GetDbInstance(ClientContext &context) {
	auto &cache = ObjectCache::GetObjectCache(context);
	auto entry = cache.Get<DatabaseInstanceCacheEntry>("system_stats_db_instance");
	if (!entry) {
		return nullptr;
	}
	return entry->GetDbInstance();
}

} // namespace duckdb
