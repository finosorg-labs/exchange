/**
 * @file bench_arbitrage.c
 * @brief Performance benchmarks for cross-market arbitrage spread computation
 */

#include "bench_framework.h"
#include "mem_aligned.h"
#include "platform.h"
#include "signal/arbitrage.h"
#include "simd_detect.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    double* spread_out;
    double* best_bid;
    double* best_ask;
    double* fees;
    int n_markets;
} bench_arb_data_t;

static void bench_arb_fn(void* user_data) {
    bench_arb_data_t* data = (bench_arb_data_t*) user_data;
    fc_ex_sig_arb_spread(
        data->spread_out, data->best_bid, data->best_ask, data->fees, data->n_markets
    );
}

static void bench_arb_spread_impl(int n_markets, const char* name) {
    const size_t output_size = n_markets * n_markets;

    double* best_bid   = fc_aligned_alloc(n_markets * sizeof(double), 64);
    double* best_ask   = fc_aligned_alloc(n_markets * sizeof(double), 64);
    double* fees       = fc_aligned_alloc(n_markets * sizeof(double), 64);
    double* spread_out = fc_aligned_alloc(output_size * sizeof(double), 64);

    if (!best_bid || !spread_out) {
        fprintf(stderr, "Memory allocation failed\n");
        return;
    }

    /* Initialize with realistic trading data */
    for (int i = 0; i < n_markets; i++) {
        double base_price = 100.0 + i * 0.5;
        best_bid[i]       = base_price + (i % 3) * 0.01;
        best_ask[i]       = base_price + 0.01 + (i % 5) * 0.01;
        fees[i]           = 0.01 + (i % 10) * 0.001;
    }

    bench_arb_data_t data = {
        .spread_out = spread_out,
        .best_bid   = best_bid,
        .best_ask   = best_ask,
        .fees       = fees,
        .n_markets  = n_markets
    };

    fc_bench_config_t config = FC_BENCH_CONFIG_DEFAULT;
    config.name              = name;
    config.data_size         = (n_markets * 3 + output_size) * sizeof(double);
    config.min_iterations    = 1000;

    fc_bench_result_t result;
    fc_bench_run(&config, bench_arb_fn, &data, &result);
    fc_bench_result_print(&result);

    fc_aligned_free(best_bid);
    fc_aligned_free(best_ask);
    fc_aligned_free(fees);
    fc_aligned_free(spread_out);
}

void bench_arbitrage_run(void) {
    printf("\nCross-Market Arbitrage Spread Benchmarks\n");
    printf("------------------------------------------------------------\n");

    fc_simd_level_t level = fc_get_simd_level();
    printf("SIMD Level: %s\n", fc_simd_level_string(level));
    printf("\n");

    bench_arb_spread_impl(2, "Arbitrage/Spread/N=2");
    bench_arb_spread_impl(4, "Arbitrage/Spread/N=4");
    bench_arb_spread_impl(8, "Arbitrage/Spread/N=8");
    bench_arb_spread_impl(16, "Arbitrage/Spread/N=16");
    bench_arb_spread_impl(32, "Arbitrage/Spread/N=32");
    bench_arb_spread_impl(64, "Arbitrage/Spread/N=64");

    printf("\nPerformance Targets:\n");
    printf("  Cross-market arbitrage (batch): ~0.5-2 μs\n");
    printf("  Latency window for arbitrage:   10-500 μs\n");
    printf("  Signal layer total budget:      ~0.5-2 μs\n");
}
