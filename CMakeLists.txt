cmake_minimum_required(VERSION 3.16)
project(finkit-exchange VERSION 1.0.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Include SIMD detection and compiler flags
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/DetectSIMD.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/CompilerFlags.cmake)

# Sanitizer configuration
if(FC_ENABLE_SANITIZERS)
    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/cmake/sanitizers.cmake)
        include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/sanitizers.cmake)
    endif()
endif()

# Generate cgo_source.go with detected SIMD flags
if(FC_SIMD_LEVEL STREQUAL "AVX512")
    set(FC_SIMD_CFLAGS "${FC_AVX512_FLAGS}")
elseif(FC_SIMD_LEVEL STREQUAL "AVX2")
    set(FC_SIMD_CFLAGS "${FC_AVX2_FLAGS}")
elseif(FC_SIMD_LEVEL STREQUAL "SSE42")
    set(FC_SIMD_CFLAGS "${FC_SSE42_FLAGS}")
else()
    set(FC_SIMD_CFLAGS "")
endif()
# Remove -mfma flag for Go (not allowed in CGO)
string(REPLACE "-mfma" "" FC_SIMD_CFLAGS "${FC_SIMD_CFLAGS}")
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cgo_source.go.in
    ${CMAKE_CURRENT_SOURCE_DIR}/cgo_source.go
    @ONLY
)

# Detect platform for library paths
if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "arm64|aarch64")
        set(EXCHANGE_BUILD_DIR "darwin_arm64")
    else()
        set(EXCHANGE_BUILD_DIR "darwin_x86_64")
    endif()
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(EXCHANGE_BUILD_DIR "linux_amd64")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(EXCHANGE_BUILD_DIR "windows_amd64")
endif()

# Build exchange-c subdirectory (creates exchange_obj target)
add_subdirectory(exchange-c)

# Create base static library (without dependencies)
add_library(finkit_exchange_static_base STATIC $<TARGET_OBJECTS:exchange_obj>)
target_include_directories(finkit_exchange_static_base
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

# Merge static libraries with deduplication
# Use build-specific path to avoid conflicts between different build types (sanitizers)
set(MERGED_LIB_PATH ${CMAKE_CURRENT_BINARY_DIR}/libfinkit_exchange_static.a)
# Fixed path for Go cgo bindings
set(FINAL_LIB_PATH ${CMAKE_CURRENT_SOURCE_DIR}/build/${EXCHANGE_BUILD_DIR}/libfinkit_exchange_static.a)

# Determine merge tool based on platform
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(MERGE_TOOL "lib.exe")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set(MERGE_TOOL "libtool")
else()
    set(MERGE_TOOL "${CMAKE_AR}")
endif()

# Unified merge command for all platforms
set(MERGE_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/scripts/merge_libs.sh)

add_custom_command(
    OUTPUT ${MERGED_LIB_PATH}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_SOURCE_DIR}/build/${EXCHANGE_BUILD_DIR}
    COMMAND bash ${MERGE_SCRIPT}
        ${MERGED_LIB_PATH}
        $<TARGET_FILE:finkit_exchange_static_base>
        ${MERGE_TOOL}
        --mode=smart
        --project-root=${CMAKE_CURRENT_SOURCE_DIR}
        --build-subdir=${EXCHANGE_BUILD_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${MERGED_LIB_PATH} ${FINAL_LIB_PATH}
    DEPENDS finkit_exchange_static_base ${MERGE_SCRIPT}
    COMMENT "Merging static libraries with smart deduplication"
    VERBATIM
)

# Create custom target to trigger the merge
add_custom_target(finkit_exchange_static ALL
    DEPENDS ${MERGED_LIB_PATH}
    COMMAND ${CMAKE_COMMAND} -E echo "Merged library created at: ${MERGED_LIB_PATH}"
    COMMAND ${CMAKE_COMMAND} -E echo "Final library for Go: ${FINAL_LIB_PATH}"
)

# Create interface library for linking
add_library(finkit_exchange_static_interface INTERFACE)
target_include_directories(finkit_exchange_static_interface
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)
target_link_libraries(finkit_exchange_static_interface
    INTERFACE
        ${MERGED_LIB_PATH}
        m
        gfortran
)

# Create alias
add_library(finkit::exchange ALIAS finkit_exchange_static_interface)

# Install merged static library
install(FILES ${MERGED_LIB_PATH}
    DESTINATION lib
)

install(DIRECTORY include/
    DESTINATION include/exchange
    FILES_MATCHING PATTERN "*.h"
)

# Install interface library target
install(TARGETS finkit_exchange_static_interface
    EXPORT finkit-exchange-targets
)

install(EXPORT finkit-exchange-targets
    FILE finkit-exchangeTargets.cmake
    NAMESPACE finkit::
    DESTINATION lib/cmake/finkit-exchange
)

# Config files
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/finkit-exchangeConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY AnyNewerVersion
)

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/finkit-exchangeConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/finkit-exchangeConfig.cmake"
    INSTALL_DESTINATION lib/cmake/finkit-exchange
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/finkit-exchangeConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/finkit-exchangeConfigVersion.cmake"
    DESTINATION lib/cmake/finkit-exchange
)

# Tests
option(FC_BUILD_TESTS "Build tests" ON)
if(FC_BUILD_TESTS AND EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/tests)
    enable_testing()
    add_subdirectory(tests)
endif()

# Benchmarks
option(FC_BUILD_BENCHMARKS "Build benchmarks" ON)
if(FC_BUILD_BENCHMARKS AND EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/benchmarks)
    add_subdirectory(benchmarks)
endif()

# Print configuration
message(STATUS "")
message(STATUS "============================================================")
message(STATUS " finkit-exchange ${PROJECT_VERSION}")
message(STATUS "============================================================")
message(STATUS "  C Compiler: ${CMAKE_C_COMPILER_ID} ${CMAKE_C_COMPILER_VERSION}")
message(STATUS "  Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  System: ${CMAKE_SYSTEM_NAME}")
message(STATUS "============================================================")
message(STATUS "")
