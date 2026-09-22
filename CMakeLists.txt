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
include_directories(src/include rust/include)

set(EXTENSION_SOURCES
    src/cpu_stats_query_function.cpp
    src/disk_stats_query_function.cpp
    src/memory_stats_query_function.cpp
    src/memory_unit_util.cpp
    src/network_stats_query_function.cpp
    src/os_info_query_function.cpp
    src/rust_stats.cpp
    src/system_stats_extension.cpp)

# The system-statistics implementation is Rust; C++ only adapts its C ABI to
# DuckDB's static extension API.
find_program(CARGO_EXECUTABLE cargo)
if(NOT CARGO_EXECUTABLE)
  message(FATAL_ERROR "cargo is required to build system_stats")
endif()

if(APPLE)
  set(PLATFORM_LIBS m c System resolv "-framework CoreFoundation"
                    "-framework IOKit")
elseif(UNIX)
  set(PLATFORM_LIBS m c dl pthread resolv)
elseif(WIN32)
  set(PLATFORM_LIBS
      ntdll
      crypt32
      ncrypt
      secur32
      ws2_32
      userenv
      bcrypt
      advapi32)
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  set(CARGO_PROFILE_FLAG "")
  set(CARGO_PROFILE_DIR "debug")
else()
  set(CARGO_PROFILE_FLAG "--release")
  set(CARGO_PROFILE_DIR "release")
endif()

set(RUST_TARGET_DIR "${CMAKE_BINARY_DIR}/rust-system-stats")
if(DEFINED Rust_CARGO_TARGET AND NOT "${Rust_CARGO_TARGET}" STREQUAL "")
  set(RUST_CARGO_TARGET_FLAG "--target=${Rust_CARGO_TARGET}")
  set(SYSTEM_STATS_RUST_LIB
      "${RUST_TARGET_DIR}/${Rust_CARGO_TARGET}/${CARGO_PROFILE_DIR}/${CMAKE_STATIC_LIBRARY_PREFIX}system_stats${CMAKE_STATIC_LIBRARY_SUFFIX}"
  )
else()
  set(RUST_CARGO_TARGET_FLAG "")
  set(SYSTEM_STATS_RUST_LIB
      "${RUST_TARGET_DIR}/${CARGO_PROFILE_DIR}/${CMAKE_STATIC_LIBRARY_PREFIX}system_stats${CMAKE_STATIC_LIBRARY_SUFFIX}"
  )
endif()

file(
  GLOB_RECURSE
  SYSTEM_STATS_RUST_SOURCES
  "${CMAKE_CURRENT_SOURCE_DIR}/src/*.rs"
  "${CMAKE_CURRENT_SOURCE_DIR}/rust/include/*.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/Cargo.toml"
  "${CMAKE_CURRENT_SOURCE_DIR}/Cargo.lock")

add_custom_command(
  OUTPUT "${SYSTEM_STATS_RUST_LIB}"
  COMMAND
    ${CMAKE_COMMAND} -E env --unset=CC --unset=CXX --unset=LD
    ${CARGO_EXECUTABLE} build --manifest-path
    "${CMAKE_CURRENT_SOURCE_DIR}/Cargo.toml" --target-dir "${RUST_TARGET_DIR}"
    ${CARGO_PROFILE_FLAG} ${RUST_CARGO_TARGET_FLAG}
  DEPENDS ${SYSTEM_STATS_RUST_SOURCES}
  WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
  COMMENT "Building system statistics Rust library"
  VERBATIM)

add_custom_target(system_stats_rust DEPENDS "${SYSTEM_STATS_RUST_LIB}")

build_static_extension(${TARGET_NAME} ${EXTENSION_SOURCES})
build_loadable_extension(${TARGET_NAME} " " ${EXTENSION_SOURCES})

target_link_libraries(${EXTENSION_NAME} "${SYSTEM_STATS_RUST_LIB}"
                      ${PLATFORM_LIBS})
target_link_libraries(${LOADABLE_EXTENSION_NAME} "${SYSTEM_STATS_RUST_LIB}"
                      ${PLATFORM_LIBS})
add_dependencies(${EXTENSION_NAME} system_stats_rust)
add_dependencies(${LOADABLE_EXTENSION_NAME} system_stats_rust)

install(
  TARGETS ${EXTENSION_NAME}
  EXPORT "${DUCKDB_EXPORT_SET}"
  LIBRARY DESTINATION "${INSTALL_LIB_DIR}"
  ARCHIVE DESTINATION "${INSTALL_LIB_DIR}")
