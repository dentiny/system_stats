#include "database_instance_cache.hpp"
#include "duckdb/storage/object_cache.hpp"
#include "duckdb/main/client_context.hpp"

namespace duckdb {

shared_ptr<DatabaseInstance> GetDbInstance(ClientContext &context) {
	auto &cache = context.db->GetObjectCache();
	auto entry = cache.Get<DatabaseInstanceCacheEntry>("system_stats_db_instance");
	if (!entry) {
		return nullptr;
	}
	return entry->GetDbInstance();
}

} // namespace duckdb
