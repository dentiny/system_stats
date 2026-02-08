cmake_minimum_required(VERSION 3.5)

set(TARGET_NAME system_stats)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_FLAGS
    "${CMAKE_CXX_FLAGS} -Wunused-variable -Wunused-function -Wunused-but-set-variable"
)

set(EXTENSION_NAME ${TARGET_NAME}_extension)
set(LOADABLE_EXTENSION_NAME ${TARGET_NAME}_loadable_extension)

project(${TARGET_NAME})
include_directories(src/include)

set(EXTENSION_SOURCES
    src/cpu_stats.cpp
    src/cpu_stats_query_function.cpp
    src/database_instance_cache.cpp
    src/disk_stats.cpp
    src/disk_stats_query_function.cpp
    src/memory_stats.cpp
    src/memory_stats_query_function.cpp
    src/memory_unit_util.cpp
    src/network_stats.cpp
    src/network_stats_query_function.cpp
    src/os_info.cpp
    src/os_info_query_function.cpp
    src/string_utils.cpp
    src/system_stats_extension.cpp)

build_static_extension(${TARGET_NAME} ${EXTENSION_SOURCES})
build_loadable_extension(${TARGET_NAME} " " ${EXTENSION_SOURCES})

install(
  TARGETS ${EXTENSION_NAME}
  EXPORT "${DUCKDB_EXPORT_SET}"
  LIBRARY DESTINATION "${INSTALL_LIB_DIR}"
  ARCHIVE DESTINATION "${INSTALL_LIB_DIR}")

# Test cases.
include_directories(duckdb/third_party/catch)

add_executable(test_string_utils unit/test_string_utils.cpp)
target_link_libraries(test_string_utils ${EXTENSION_NAME})
