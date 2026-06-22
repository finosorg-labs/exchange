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
    test_ofi.c
)

# Link with exchange library
target_link_libraries(exchange_tests
    PRIVATE
        finkit_exchange_static_base
        finkit::stats
      finkit::sort
        finkit::ds
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
