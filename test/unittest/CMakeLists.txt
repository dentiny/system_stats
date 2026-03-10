include_directories(include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_SOURCE_DIR}/third_party)
include_directories(${CMAKE_SOURCE_DIR}/src/include)
include_directories(${CMAKE_SOURCE_DIR}/src/utils/include)
include_directories(${DuckDB_SOURCE_DIR}/third_party)
include_directories(${DuckDB_SOURCE_DIR}/test/include)

set(SYSTEM_STATS_UNITTEST_OBJECTS main.cpp test_string_utils.cpp)

add_executable(unittest_system_stats ${SYSTEM_STATS_UNITTEST_OBJECTS})

if(NOT WIN32
   AND NOT SUN
   AND NOT ZOS)
  target_link_libraries(unittest_system_stats test_helpers duckdb
                        ${EXTENSION_NAME})
else()
  target_link_libraries(unittest_system_stats test_helpers duckdb_static
                        ${EXTENSION_NAME})
endif()
