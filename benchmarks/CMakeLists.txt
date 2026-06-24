#
# benchmarks/CMakeLists.txt - Benchmark configuration for exchange module
#

# Benchmark framework library
add_library(bench_framework OBJECT
    bench_framework.c
)

target_include_directories(bench_framework
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/modules/platform/include
)

target_link_libraries(bench_framework
    PUBLIC
        finkit_exchange_static_base
        finkit::stats
        finkit::optim
      finkit::sort
        finkit::ds
        finkit::math
        finkit::platform
)

# Benchmark executable
add_executable(exchange_benchmarks
    bench_main.c
    bench_exchange.c
    bench_ticker.c
    bench_ticker_merge.c
    bench_orderbook.c
    bench_market_indicators.c
    bench_ofi.c
    bench_microprice.c
    bench_spread.c
    bench_vwap_dev.c
    bench_kyle_lambda.c
    bench_arbitrage.c
    bench_feature.c
    bench_normalize.c
)

# Link with benchmark framework and exchange library
target_link_libraries(exchange_benchmarks
    PRIVATE
        bench_framework
        finkit_exchange_static_base
        finkit::stats
        finkit::optim
      finkit::sort
        finkit::ds
        finkit::math
        finkit::platform
        m
)

target_compile_definitions(exchange_benchmarks PRIVATE
    FC_STATIC_BUILD
)

# Include directories
target_include_directories(exchange_benchmarks
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/modules/platform/include
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# Compiler flags for benchmarks
target_compile_options(exchange_benchmarks PRIVATE -Wall -Wextra -Wpedantic -ffp-contract=off)

# Installation
install(TARGETS exchange_benchmarks
    RUNTIME DESTINATION bin
)
