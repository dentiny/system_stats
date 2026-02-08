#include "database_instance_cache.hpp"

#include "duckdb/common/exception.hpp"
#include "duckdb/common/string.hpp"
#include "duckdb/main/client_context.hpp"
#include "duckdb/storage/object_cache.hpp"

namespace duckdb {

DatabaseInstanceCacheEntry::DatabaseInstanceCacheEntry(shared_ptr<DatabaseInstance> db) : db_weak(std::move(db)) {
}

string DatabaseInstanceCacheEntry::ObjectType() {
	return "system_stats_database_instance_cache";
}

string DatabaseInstanceCacheEntry::GetObjectType() {
	return ObjectType();
}

shared_ptr<DatabaseInstance> DatabaseInstanceCacheEntry::GetDbInstance() {
	auto db = db_weak.lock();
	if (!db) {
		throw InternalException("DatabaseInstance has been destroyed");
	}
	return db;
}

shared_ptr<DatabaseInstance> GetDbInstance(ClientContext &context) {
	auto &cache = context.db->GetObjectCache();
	auto entry = cache.Get<DatabaseInstanceCacheEntry>("system_stats_db_instance");
	if (!entry) {
		throw InternalException("DatabaseInstance cache entry not found");
	}
	return entry->GetDbInstance();
}

} // namespace duckdb
