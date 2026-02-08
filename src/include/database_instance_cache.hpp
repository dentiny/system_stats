#pragma once

#include "duckdb/common/shared_ptr.hpp"
#include "duckdb/main/database.hpp"
#include "duckdb/storage/object_cache.hpp"

namespace duckdb {

// ObjectCacheEntry that stores a weak pointer to DatabaseInstance
// This allows per-database access to the DatabaseInstance for logging
// Using weak_ptr prevents circular references while allowing safe access
class DatabaseInstanceCacheEntry : public ObjectCacheEntry {
public:
	explicit DatabaseInstanceCacheEntry(shared_ptr<DatabaseInstance> db) : db_weak(db) {
	}

	static string ObjectType() {
		return "system_stats_database_instance_cache";
	}

	string GetObjectType() override {
		return ObjectType();
	}

	// Get the DatabaseInstance shared pointer
	// Returns nullptr if the DatabaseInstance has been destroyed
	shared_ptr<DatabaseInstance> GetDbInstance() {
		return db_weak.lock();
	}

private:
	weak_ptr<DatabaseInstance> db_weak;
};

// Utility function to get DatabaseInstance from ObjectCache using ClientContext
// Returns nullptr if not found or if DatabaseInstance has been destroyed
shared_ptr<DatabaseInstance> GetDbInstance(ClientContext &context);

} // namespace duckdb
