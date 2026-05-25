/**
 * @file test_exchange.c
 * @brief exchange module test entry point
 *
 * This file serves as the main test registration point for the exchange module.
 * Individual test modules are in separate files:
 */

#include "test_framework.h"

/* External test registration functions from sub-modules */
extern void register_ticker_tests(void);
extern void register_ticker_merge_tests(void);

/* Entry point for exchange tests */
void register_exchange_tests(void) {
    /* Register all sub-module tests */
    register_ticker_tests();
    register_ticker_merge_tests();
}
