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
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cgo_source.go.in
    ${CMAKE_CURRENT_SOURCE_DIR}/cgo_source.go
    @ONLY
)

# Use prebuilt platform library
if(NOT TARGET finkit::platform)
    # Detect platform
    if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
        if(CMAKE_SYSTEM_PROCESSOR MATCHES "arm64|aarch64")
            set(PLATFORM_BUILD_DIR "${CMAKE_CURRENT_SOURCE_DIR}/modules/platform/build/darwin_arm64")
        else()
            set(PLATFORM_BUILD_DIR "${CMAKE_CURRENT_SOURCE_DIR}/modules/platform/build/darwin_x86_64")
        endif()
    elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        set(PLATFORM_BUILD_DIR "${CMAKE_CURRENT_SOURCE_DIR}/modules/platform/build/linux_amd64")
    elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
        set(PLATFORM_BUILD_DIR "${CMAKE_CURRENT_SOURCE_DIR}/modules/platform/build/windows_amd64")
    else()
        message(FATAL_ERROR "Unsupported platform: ${CMAKE_SYSTEM_NAME}")
    endif()

    # Import prebuilt platform library
    add_library(finkit_platform_static STATIC IMPORTED)
    set_target_properties(finkit_platform_static PROPERTIES
        IMPORTED_LOCATION "${PLATFORM_BUILD_DIR}/libfinkit_platform_static.a"
    )
    target_include_directories(finkit_platform_static INTERFACE
        "${CMAKE_CURRENT_SOURCE_DIR}/modules/platform/include"
    )
    add_library(finkit::platform ALIAS finkit_platform_static)
endif()

# Use prebuilt ds library
if(NOT TARGET finkit::ds)
    # Detect ds
    if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
        if(CMAKE_SYSTEM_PROCESSOR MATCHES "arm64|aarch64")
            set(DS_BUILD_DIR "${CMAKE_CURRENT_SOURCE_DIR}/modules/ds/build/darwin_arm64")
        else()
            set(DS_BUILD_DIR "${CMAKE_CURRENT_SOURCE_DIR}/modules/ds/build/darwin_x86_64")
        endif()
    elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        set(DS_BUILD_DIR "${CMAKE_CURRENT_SOURCE_DIR}/modules/ds/build/linux_amd64")
    elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
        set(DS_BUILD_DIR "${CMAKE_CURRENT_SOURCE_DIR}/modules/ds/build/windows_amd64")
    else()
        message(FATAL_ERROR "Unsupported platform: ${CMAKE_SYSTEM_NAME}")
    endif()

    # Import prebuilt ds library
    add_library(finkit_ds_static STATIC IMPORTED)
    set_target_properties(finkit_ds_static PROPERTIES
        IMPORTED_LOCATION "${DS_BUILD_DIR}/libfinkit_ds_static.a"
    )
    target_include_directories(finkit_ds_static INTERFACE
        "${CMAKE_CURRENT_SOURCE_DIR}/modules/ds/include"
    )
    add_library(finkit::ds ALIAS finkit_ds_static)
endif()

# Use prebuilt sort library
if(NOT TARGET finkit::sort)
    # Detect sort
    if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
        if(CMAKE_SYSTEM_PROCESSOR MATCHES "arm64|aarch64")
            set(SORT_BUILD_DIR "${CMAKE_CURRENT_SOURCE_DIR}/modules/sort/build/darwin_arm64")
        else()
            set(SORT_BUILD_DIR "${CMAKE_CURRENT_SOURCE_DIR}/modules/sort/build/darwin_x86_64")
        endif()
    elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        set(SORT_BUILD_DIR "${CMAKE_CURRENT_SOURCE_DIR}/modules/sort/build/linux_amd64")
    elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
        set(SORT_BUILD_DIR "${CMAKE_CURRENT_SOURCE_DIR}/modules/sort/build/windows_amd64")
    else()
        message(FATAL_ERROR "Unsupported sort: ${CMAKE_SYSTEM_NAME}")
    endif()

    # Import prebuilt sort library
    add_library(finkit_sort_static STATIC IMPORTED)
    set_target_properties(finkit_sort_static PROPERTIES
        IMPORTED_LOCATION "${SORT_BUILD_DIR}/libfinkit_sort_static.a"
    )
    target_include_directories(finkit_sort_static INTERFACE
        "${CMAKE_CURRENT_SOURCE_DIR}/modules/sort/include"
    )
    add_library(finkit::sort ALIAS finkit_sort_static)
endif()

# Use prebuilt stats library
if(NOT TARGET finkit::stats)
    # Detect stats
    if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
        if(CMAKE_SYSTEM_PROCESSOR MATCHES "arm64|aarch64")
            set(STATS_BUILD_DIR "${CMAKE_CURRENT_SOURCE_DIR}/modules/stats/build/darwin_arm64")
        else()
            set(STATS_BUILD_DIR "${CMAKE_CURRENT_SOURCE_DIR}/modules/stats/build/darwin_x86_64")
        endif()
    elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        set(STATS_BUILD_DIR "${CMAKE_CURRENT_SOURCE_DIR}/modules/stats/build/linux_amd64")
    elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
        set(STATS_BUILD_DIR "${CMAKE_CURRENT_SOURCE_DIR}/modules/stats/build/windows_amd64")
    else()
        message(FATAL_ERROR "Unsupported stats: ${CMAKE_SYSTEM_NAME}")
    endif()

    # Import prebuilt stats library
    add_library(finkit_stats_static STATIC IMPORTED)
    set_target_properties(finkit_stats_static PROPERTIES
        IMPORTED_LOCATION "${STATS_BUILD_DIR}/libfinkit_stats_static.a"
    )
    target_include_directories(finkit_stats_static INTERFACE
        "${CMAKE_CURRENT_SOURCE_DIR}/modules/stats/include"
    )
    add_library(finkit::stats ALIAS finkit_stats_static)
endif()

# Use prebuilt optim library
if(NOT TARGET finkit::optim)
    # Detect optim
    if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
        if(CMAKE_SYSTEM_PROCESSOR MATCHES "arm64|aarch64")
            set(OPTIM_BUILD_DIR "${CMAKE_CURRENT_SOURCE_DIR}/modules/optim/build/darwin_arm64")
        else()
            set(OPTIM_BUILD_DIR "${CMAKE_CURRENT_SOURCE_DIR}/modules/optim/build/darwin_x86_64")
        endif()
    elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        set(OPTIM_BUILD_DIR "${CMAKE_CURRENT_SOURCE_DIR}/modules/optim/build/linux_amd64")
    elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
        set(OPTIM_BUILD_DIR "${CMAKE_CURRENT_SOURCE_DIR}/modules/optim/build/windows_amd64")
    else()
        message(FATAL_ERROR "Unsupported optim: ${CMAKE_SYSTEM_NAME}")
    endif()

    # Import prebuilt optim library
    add_library(finkit_optim_static STATIC IMPORTED)
    set_target_properties(finkit_optim_static PROPERTIES
        IMPORTED_LOCATION "${OPTIM_BUILD_DIR}/libfinkit_optim_static.a"
    )
    target_include_directories(finkit_optim_static INTERFACE
        "${CMAKE_CURRENT_SOURCE_DIR}/modules/optim/include"
    )
    add_library(finkit::optim ALIAS finkit_optim_static)
endif()

# Use prebuilt math library
if(NOT TARGET finkit::math)
    # Detect math
    if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
        if(CMAKE_SYSTEM_PROCESSOR MATCHES "arm64|aarch64")
            set(MATH_BUILD_DIR "${CMAKE_CURRENT_SOURCE_DIR}/modules/math/build/darwin_arm64")
        else()
            set(MATH_BUILD_DIR "${CMAKE_CURRENT_SOURCE_DIR}/modules/math/build/darwin_x86_64")
        endif()
    elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        set(MATH_BUILD_DIR "${CMAKE_CURRENT_SOURCE_DIR}/modules/math/build/linux_amd64")
    elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
        set(MATH_BUILD_DIR "${CMAKE_CURRENT_SOURCE_DIR}/modules/math/build/windows_amd64")
    else()
        message(FATAL_ERROR "Unsupported math: ${CMAKE_SYSTEM_NAME}")
    endif()

    # Import prebuilt math library
    add_library(finkit_math_static STATIC IMPORTED)
    set_target_properties(finkit_math_static PROPERTIES
        IMPORTED_LOCATION "${MATH_BUILD_DIR}/libfinkit_math_static.a"
    )
    target_include_directories(finkit_math_static INTERFACE
        "${CMAKE_CURRENT_SOURCE_DIR}/modules/math/include"
    )
    add_library(finkit::math ALIAS finkit_math_static)
endif()

# exchange module sources
set(EXCHANGE_SOURCES
    exchange-c/ticker.c
    exchange-c/ticker_merge.c
    exchange-c/orderbook.c
    exchange-c/market_indicators.c
    exchange-c/market_maker_quotes.c
    exchange-c/signal-c/ofi.c
    exchange-c/signal-c/microprice.c
    exchange-c/signal-c/spread.c
    exchange-c/signal-c/vwap_dev.c
    exchange-c/signal-c/kyle_lambda.c
    exchange-c/signal-c/arbitrage.c
    exchange-c/signal-c/feature.c
    exchange-c/signal-c/normalize.c
    exchange-c/signal-c/postprocess.c
    exchange-c/signal-c/alpha.c
    exchange-c/strategy-c/market_maker.c
    exchange-c/strategy-c/stat_arb.c
    exchange-c/strategy-c/latency_arb.c
    exchange-c/strategy-c/index_rebal.c
    exchange-c/strategy-c/delta_hedge.c
    exchange-c/risk-c/marginal_var.c
    exchange-c/risk-c/factor_risk.c
    exchange-c/risk-c/stress_scenario.c
)

# Headers are now directly available from submodules via sparse-checkout
# No need to copy them to a separate directory

# Create object library
add_library(finkit_exchange OBJECT ${EXCHANGE_SOURCES})

target_include_directories(finkit_exchange
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/modules/platform/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/modules/ds/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/modules/sort/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/modules/stats/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/modules/optim/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/modules/math/include>
        $<INSTALL_INTERFACE:include>
)

target_compile_definitions(finkit_exchange PRIVATE FC_STATIC_BUILD)

# Note: SIMD dispatch is handled via __attribute__((target(...))) in source files
# No global SIMD flags needed - each function specifies its own target ISA

# Create base static library (without dependencies)
add_library(finkit_exchange_static_base STATIC $<TARGET_OBJECTS:finkit_exchange>)
target_include_directories(finkit_exchange_static_base
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

# Determine exchange build directory
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

# Merge static libraries into a single library
set(MERGED_LIB_PATH ${CMAKE_CURRENT_SOURCE_DIR}/build/${EXCHANGE_BUILD_DIR}/libfinkit_exchange_static.a)
set(STATS_LIB_PATH "${STATS_BUILD_DIR}/libfinkit_stats_static.a")
set(OPTIM_LIB_PATH "${OPTIM_BUILD_DIR}/libfinkit_optim_static.a")
set(SORT_LIB_PATH "${SORT_BUILD_DIR}/libfinkit_sort_static.a")
set(DS_LIB_PATH "${DS_BUILD_DIR}/libfinkit_ds_static.a")
set(MATH_LIB_PATH "${MATH_BUILD_DIR}/libfinkit_math_static.a")
set(PLATFORM_LIB_PATH "${PLATFORM_BUILD_DIR}/libfinkit_platform_static.a")

if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    # Windows: use lib.exe to merge libraries
    add_custom_command(
        OUTPUT ${MERGED_LIB_PATH}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_SOURCE_DIR}/build/${EXCHANGE_BUILD_DIR}
     COMMAND lib.exe /OUT:${MERGED_LIB_PATH}
            $<TARGET_FILE:finkit_exchange_static_base>
            ${STATS_LIB_PATH}
            ${OPTIM_LIB_PATH}
        ${SORT_LIB_PATH}
            ${DS_LIB_PATH}
            ${MATH_LIB_PATH}
            ${PLATFORM_LIB_PATH}
        DEPENDS finkit_exchange_static_base ${STATS_LIB_PATH} ${OPTIM_LIB_PATH} ${SORT_LIB_PATH} ${DS_LIB_PATH} ${MATH_LIB_PATH} ${PLATFORM_LIB_PATH}
     COMMENT "Merging static libraries into ${MERGED_LIB_PATH}"
        VERBATIM
  )
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    # macOS: use libtool to merge libraries
    add_custom_command(
        OUTPUT ${MERGED_LIB_PATH}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_SOURCE_DIR}/build/${EXCHANGE_BUILD_DIR}
        COMMAND libtool -static -o ${MERGED_LIB_PATH}
            $<TARGET_FILE:finkit_exchange_static_base>
            ${STATS_LIB_PATH}
            ${OPTIM_LIB_PATH}
          ${SORT_LIB_PATH}
          ${DS_LIB_PATH}
            ${MATH_LIB_PATH}
            ${PLATFORM_LIB_PATH}
        2>&1 | grep -v "warning duplicate member name" || true
        DEPENDS finkit_exchange_static_base ${STATS_LIB_PATH} ${OPTIM_LIB_PATH} ${SORT_LIB_PATH} ${DS_LIB_PATH} ${MATH_LIB_PATH} ${PLATFORM_LIB_PATH}
        COMMENT "Merging static libraries into ${MERGED_LIB_PATH}"
        VERBATIM
    )
else()
    # Linux: use ar with MRI script to merge libraries
    set(MRI_SCRIPT ${CMAKE_CURRENT_BINARY_DIR}/merge_libs.mri)
    file(GENERATE OUTPUT ${MRI_SCRIPT} CONTENT
"CREATE ${MERGED_LIB_PATH}
ADDLIB $<TARGET_FILE:finkit_exchange_static_base>
ADDLIB ${STATS_LIB_PATH}
ADDLIB ${OPTIM_LIB_PATH}
ADDLIB ${SORT_LIB_PATH}
ADDLIB ${DS_LIB_PATH}
ADDLIB ${MATH_LIB_PATH}
ADDLIB ${PLATFORM_LIB_PATH}
SAVE
END
")

    add_custom_command(
        OUTPUT ${MERGED_LIB_PATH}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_SOURCE_DIR}/build/${EXCHANGE_BUILD_DIR}
        COMMAND ${CMAKE_AR} -M < ${MRI_SCRIPT}
     DEPENDS finkit_exchange_static_base ${STATS_LIB_PATH} ${OPTIM_LIB_PATH} ${SORT_LIB_PATH} ${DS_LIB_PATH} ${MATH_LIB_PATH} ${PLATFORM_LIB_PATH} ${MRI_SCRIPT}
        COMMENT "Merging static libraries into ${MERGED_LIB_PATH}"
        VERBATIM
    )
endif()

# Create custom target to trigger the merge
add_custom_target(finkit_exchange_static ALL
    DEPENDS ${MERGED_LIB_PATH}
    COMMAND ${CMAKE_COMMAND} -E echo "Merged library created at: ${MERGED_LIB_PATH}"
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
        m
        gcov
        ${MERGED_LIB_PATH}
)

# Create alias
add_library(finkit::exchange ALIAS finkit_exchange_static_interface)

# Install merged static library
install(FILES ${MERGED_LIB_PATH}
    DESTINATION lib
)

# Install headers from this project and submodules
install(DIRECTORY include/
    DESTINATION include/exchange
    FILES_MATCHING PATTERN "*.h"
)
install(DIRECTORY modules/platform/include/
    DESTINATION include/exchange
    FILES_MATCHING PATTERN "*.h"
)
install(DIRECTORY modules/ds/include/
    DESTINATION include/exchange
    FILES_MATCHING PATTERN "*.h"
)
install(DIRECTORY modules/stats/include/
    DESTINATION include/exchange
    FILES_MATCHING PATTERN "*.h"
)
install(DIRECTORY modules/math/include/
    DESTINATION include/exchange
    FILES_MATCHING PATTERN "*.h"
)
install(DIRECTORY modules/optim/include/
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
