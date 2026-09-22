#include "system_stats_functions.hpp"

#include "cpu_stats_query_function.hpp"
#include "disk_stats_query_function.hpp"
#include "duckdb/main/extension/extension_loader.hpp"
#include "duckdb/parser/parsed_data/create_table_function_info.hpp"
#include "memory_stats_query_function.hpp"
#include "network_stats_query_function.hpp"
#include "os_info_query_function.hpp"

namespace duckdb {

namespace {

void RegisterTableFunction(ExtensionLoader &loader, TableFunction function, vector<string> parameter_names,
                           string description, vector<string> examples, vector<string> categories) {
	CreateTableFunctionInfo info(std::move(function));
	info.on_conflict = OnCreateConflict::ALTER_ON_CONFLICT;

	FunctionDescription function_description;
	function_description.parameter_names = std::move(parameter_names);
	function_description.description = std::move(description);
	function_description.examples = std::move(examples);
	function_description.categories = std::move(categories);
	info.descriptions.push_back(std::move(function_description));

	loader.RegisterFunction(std::move(info));
}

} // namespace

void RegisterSystemStatsFunctions(ExtensionLoader &loader) {
	RegisterTableFunction(
	    loader, GetSysCPUInfoFunction(),
	    /*parameter_names=*/ {},
	    /*description=*/
	    "Returns processor identity, architecture, core counts, cache sizes, and byte order for the host system.",
	    /*examples=*/ {"SELECT * FROM sys_cpu_info();"},
	    /*categories=*/ {"system", "cpu"});
	RegisterTableFunction(
	    loader, GetSysMemoryInfoFunction(),
	    /*parameter_names=*/ {"unit"},
	    /*description=*/
	    "Returns physical memory and swap usage for the host system, optionally converted to the requested unit.",
	    /*examples=*/ {"SELECT * FROM sys_memory_info(unit = 'GiB');"},
	    /*categories=*/ {"system", "memory"});
	RegisterTableFunction(
	    loader, GetSysDiskInfoFunction(),
	    /*parameter_names=*/ {"unit"},
	    /*description=*/"Returns mounted filesystem capacity and usage, optionally converted to the requested unit.",
	    /*examples=*/ {"SELECT * FROM sys_disk_info(unit = 'GiB');"},
	    /*categories=*/ {"system", "storage"});
	RegisterTableFunction(
	    loader, GetSysNetworkInfoFunction(),
	    /*parameter_names=*/ {},
	    /*description=*/"Returns IPv4 addresses and traffic counters for the host system's network interfaces.",
	    /*examples=*/ {"SELECT * FROM sys_network_info();"},
	    /*categories=*/ {"system", "network"});
	RegisterTableFunction(
	    loader, GetSysOSInfoFunction(),
	    /*parameter_names=*/ {},
	    /*description=*/
	    "Returns operating system, host, process, handle, thread, architecture, and uptime information.",
	    /*examples=*/ {"SELECT * FROM sys_os_info();"},
	    /*categories=*/ {"system", "operating_system"});
}

} // namespace duckdb
