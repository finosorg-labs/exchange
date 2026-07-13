#
# tests/CMakeLists.txt - Test configuration for exchange module
#

# Test executable
add_executable(exchange_tests
    test_framework.c
    test_main.c
    test_exchange.c
    test_ticker.c
    test_ticker_merge.c
    test_orderbook.c
    test_market_indicators.c
    test_market_maker_quotes.c
    test_ofi.c
    test_microprice.c
    test_spread.c
    test_vwap_dev.c
    test_kyle_lambda.c
    test_arbitrage.c
    test_feature.c
    test_normalize.c
    test_postprocess.c
    test_alpha.c
    test_market_maker.c
    test_stat_arb.c
    test_latency_arb.c
    test_index_rebal.c
    test_delta_hedge.c
    test_marginal_var.c
    test_factor_risk.c
)

# Link with exchange library
target_link_libraries(exchange_tests
    PRIVATE
        finkit_exchange_static_base
        finkit::stats
        finkit::optim
      finkit::sort
        finkit::ds
        finkit::math
        finkit::platform
        m
)

target_compile_definitions(exchange_tests PRIVATE
    FC_STATIC_BUILD
)

# Include directories
target_include_directories(exchange_tests
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/modules/platform/include
        ${CMAKE_SOURCE_DIR}/modules/ds/include
        ${CMAKE_SOURCE_DIR}/modules/sort/include
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# Compiler flags for tests
target_compile_options(exchange_tests PRIVATE -Wall -Wextra -Wpedantic)

# Run tests with ctest
add_test(
    NAME "exchange_tests"
    COMMAND exchange_tests
)

# Installation
install(TARGETS exchange_tests
    RUNTIME DESTINATION bin
)
