#pragma once

namespace duckdb {

// Forward declarations
class ExtensionLoader;

void RegisterSystemStatsFunctions(ExtensionLoader &loader);

} // namespace duckdb
