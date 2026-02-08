#pragma once

#include "duckdb/main/database.hpp"

namespace duckdb {

// Storage for DatabaseInstance reference obtained during extension loading
// This allows us to access the DatabaseInstance for logging without passing it through every function
class DatabaseInstanceStorage {
public:
	static void Set(DatabaseInstance &db) {
		instance = &db;
	}

	static DatabaseInstance *Get() {
		return instance;
	}

private:
	static DatabaseInstance *instance;
};

} // namespace duckdb
