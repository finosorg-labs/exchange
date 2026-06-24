/**
 * @file bench_exchange.c
 * @brief exchange module benchmark entry point
 *
 * This file serves as the main benchmark registration point for the exchange module.
 * Individual benchmark modules are in separate files:
 */

#include "bench_framework.h"
#include <simd_detect.h>
#include <stdio.h>

/* External benchmark functions from sub-modules */
extern void bench_ticker_run(void);
extern void bench_ticker_merge_run(void);
extern void bench_order_book_run(void);
extern void bench_market_indicators_run(void);
extern void bench_ofi_run(void);
extern void bench_microprice_run(void);
extern void bench_spread_run(void);
extern void bench_vwap_dev_run(void);
extern void bench_kyle_lambda_run(void);
extern void bench_arbitrage_run(void);

/* Entry point for exchange benchmarks */
void bench_exchange_run(void) {
    printf("\n");
    printf("============================================================\n");
    printf("  exchange Module Performance Benchmarks\n");
    printf("  SIMD level: %s\n", fc_simd_level_string(fc_detect_simd()));
    printf("============================================================\n");

    /* Run all sub-module benchmarks */
    bench_ticker_run();
    bench_ticker_merge_run();
    bench_order_book_run();
    bench_market_indicators_run();
    bench_ofi_run();
    bench_microprice_run();
    bench_spread_run();
    bench_vwap_dev_run();
    bench_kyle_lambda_run();
    bench_arbitrage_run();

    printf("\n");
    printf("============================================================\n");
    printf("  exchange benchmarks complete\n");
    printf("============================================================\n");
}
