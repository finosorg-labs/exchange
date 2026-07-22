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

# Benchmark executable
add_executable(exchange_benchmarks
    bench_main.c
    bench_exchange.c
    bench_ticker.c
    bench_ticker_merge.c
    bench_orderbook.c
    bench_market_indicators.c
    bench_market_maker_quotes.c
    bench_ofi.c
    bench_microprice.c
    bench_spread.c
    bench_vwap_dev.c
    bench_kyle_lambda.c
    bench_arbitrage.c
    bench_feature.c
    bench_normalize.c
    bench_postprocess.c
    bench_alpha.c
    bench_market_maker.c
    bench_stat_arb.c
    bench_latency_arb.c
    bench_index_rebal.c
    bench_delta_hedge.c
    bench_marginal_var.c
    bench_factor_risk.c
    bench_stress_scenario.c
)

# Link with benchmark framework and merged exchange library
target_link_libraries(exchange_benchmarks
    PRIVATE
        bench_framework
        finkit_exchange_static_interface
        m
        gfortran
)

target_compile_definitions(exchange_benchmarks PRIVATE
    FC_STATIC_BUILD
)

# Include directories
target_include_directories(exchange_benchmarks
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/modules/platform/include
        ${CMAKE_SOURCE_DIR}/modules/ds/include
        ${CMAKE_SOURCE_DIR}/modules/sort/include
        ${CMAKE_SOURCE_DIR}/modules/stats/include
        ${CMAKE_SOURCE_DIR}/modules/math/include
        ${CMAKE_SOURCE_DIR}/modules/optim/include
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# Compiler flags for benchmarks
target_compile_options(exchange_benchmarks PRIVATE -Wall -Wextra -Wpedantic -ffp-contract=off)

# Installation
install(TARGETS exchange_benchmarks
    RUNTIME DESTINATION bin
)
