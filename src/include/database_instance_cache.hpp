#pragma once

#include "duckdb/storage/object_cache.hpp"
#include "duckdb/main/database.hpp"
#include "duckdb/common/shared_ptr.hpp"

namespace duckdb {

// ObjectCacheEntry that stores a pointer to DatabaseInstance
// This allows per-database access to the DatabaseInstance for logging
// Note: DatabaseInstance lifetime is managed by DuckDB, so we store a raw pointer
// The ObjectCache itself is per-DatabaseInstance, so this is safe
class DatabaseInstanceCacheEntry : public ObjectCacheEntry {
public:
	explicit DatabaseInstanceCacheEntry(DatabaseInstance &db) : db_ptr(&db) {
	}

	static string ObjectType() {
		return "system_stats_database_instance_cache";
	}

	string GetObjectType() override {
		return ObjectType();
	}

	// Get the DatabaseInstance pointer
	DatabaseInstance *GetDbInstance() {
		return db_ptr;
	}

private:
	DatabaseInstance *db_ptr;
};

// Utility function to get DatabaseInstance from ObjectCache using ClientContext
// Returns nullptr if not found
DatabaseInstance *GetDbInstance(ClientContext &context);

} // namespace duckdb
