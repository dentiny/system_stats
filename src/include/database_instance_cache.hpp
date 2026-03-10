#pragma once

#include "duckdb/common/shared_ptr.hpp"
#include "duckdb/storage/object_cache.hpp"

namespace duckdb {

// Forward declaration.
class DatabaseInstance;

// ObjectCacheEntry that stores a weak pointer to DatabaseInstance
// Using weak_ptr prevents circular references while allowing safe access
class DatabaseInstanceCacheEntry : public ObjectCacheEntry {
public:
	explicit DatabaseInstanceCacheEntry(shared_ptr<DatabaseInstance> db);

	static string ObjectType();

	string GetObjectType() override;

	// Get the DatabaseInstance shared pointer
	// Throws InternalException if the DatabaseInstance has been destroyed
	shared_ptr<DatabaseInstance> GetDbInstance();

	optional_idx GetEstimatedCacheMemory() const override {
		// Cannot be evicted.
		return optional_idx {};
	}

private:
	weak_ptr<DatabaseInstance> db_weak;
};

// Utility function to get DatabaseInstance from ObjectCache using ClientContext
// Throws InternalException if not found or if DatabaseInstance has been destroyed
shared_ptr<DatabaseInstance> GetDbInstance(ClientContext &context);

} // namespace duckdb
