/**
 * @file bench_ticker.c
 * @brief Performance benchmarks for ticker module
 */

#include "bench_framework.h"
#include "ticker.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#define NSEC_PER_SEC 1000000000LL

static inline int64_t get_time_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (int64_t)ts.tv_sec * NSEC_PER_SEC + ts.tv_nsec;
}

static void generate_random_ticks(fc_tick_t *ticks, size_t num_ticks, uint32_t num_symbols) {
    for (size_t i = 0; i < num_ticks; i++) {
        ticks[i].symbol_id = rand() % num_symbols;
        ticks[i].price = 100.0 + (rand() % 10000) / 100.0;
        ticks[i].volume = 100.0 + (rand() % 10000);
        ticks[i].amount = ticks[i].price * ticks[i].volume;
        ticks[i].timestamp_ns = 1000000000LL + i * 1000000LL;
    }
}

static void bench_single_symbol_single_period(void) {
    printf("\n=== Benchmark: Single Symbol, Single Period ===\n");

    int64_t periods[] = {60000000000LL};
    fc_ticker_ctx_t *ctx = fc_ticker_create(1, 1, periods, FC_TICKER_PRECISION_KAHAN);
    if (ctx == NULL) {
        fprintf(stderr, "Failed to create ticker context\n");
        return;
    }

    const size_t num_ticks = 100000;
    fc_tick_t *ticks = (fc_tick_t *)malloc(num_ticks * sizeof(fc_tick_t));
    generate_random_ticks(ticks, num_ticks, 1);

    int64_t start = get_time_ns();
    for (size_t i = 0; i < num_ticks; i++) {
        fc_ticker_update(ctx, &ticks[i]);
    }
    int64_t end = get_time_ns();

    double elapsed_ms = (end - start) / 1e6;
    double ticks_per_sec = num_ticks / (elapsed_ms / 1000.0);

    printf("Processed %zu ticks in %.3f ms\n", num_ticks, elapsed_ms);
    printf("Throughput: %.0f ticks/sec\n", ticks_per_sec);
    printf("Latency per tick: %.3f ns\n", (end - start) / (double)num_ticks);

    free(ticks);
    fc_ticker_destroy(ctx);
}

static void bench_multi_symbol_single_period(void) {
    printf("\n=== Benchmark: 5000 Symbols, Single Period ===\n");

    const uint32_t num_symbols = 5000;
    int64_t periods[] = {60000000000LL};
    fc_ticker_ctx_t *ctx = fc_ticker_create(num_symbols, 1, periods, FC_TICKER_PRECISION_KAHAN);
    if (ctx == NULL) {
        fprintf(stderr, "Failed to create ticker context\n");
        return;
    }

    const size_t num_ticks = 500000;
    fc_tick_t *ticks = (fc_tick_t *)malloc(num_ticks * sizeof(fc_tick_t));
    generate_random_ticks(ticks, num_ticks, num_symbols);

    int64_t start = get_time_ns();
    for (size_t i = 0; i < num_ticks; i++) {
        fc_ticker_update(ctx, &ticks[i]);
    }
    int64_t end = get_time_ns();

    double elapsed_ms = (end - start) / 1e6;
    double ticks_per_sec = num_ticks / (elapsed_ms / 1000.0);

    printf("Processed %zu ticks across %u symbols in %.3f ms\n", num_ticks, num_symbols,
           elapsed_ms);
    printf("Throughput: %.0f ticks/sec\n", ticks_per_sec);
    printf("Latency per tick: %.3f ns\n", (end - start) / (double)num_ticks);

    free(ticks);
    fc_ticker_destroy(ctx);
}

static void bench_full_market_aggregation(void) {
    printf("\n=== Benchmark: Full Market Aggregation (Target: <1ms) ===\n");

    const uint32_t num_symbols = 5000;
    const uint32_t num_periods = 4;
    int64_t periods[] = {
        60000000000LL,
        300000000000LL,
        900000000000LL,
        3600000000000LL
    };

    fc_ticker_ctx_t *ctx =
        fc_ticker_create(num_symbols, num_periods, periods, FC_TICKER_PRECISION_KAHAN);
    if (ctx == NULL) {
        fprintf(stderr, "Failed to create ticker context\n");
        return;
    }

    const size_t batch_size = 10000;
    fc_tick_t *ticks = (fc_tick_t *)malloc(batch_size * sizeof(fc_tick_t));
    generate_random_ticks(ticks, batch_size, num_symbols);

    const int num_iterations = 100;
    int64_t total_time = 0;
    int64_t min_time = NSEC_PER_SEC;
    int64_t max_time = 0;

    for (int iter = 0; iter < num_iterations; iter++) {
        int64_t start = get_time_ns();
        fc_ticker_update_batch(ctx, ticks, batch_size);
        int64_t end = get_time_ns();

        int64_t elapsed = end - start;
        total_time += elapsed;
        if (elapsed < min_time) min_time = elapsed;
        if (elapsed > max_time) max_time = elapsed;
    }

    double avg_ms = (total_time / num_iterations) / 1e6;
    double min_ms = min_time / 1e6;
    double max_ms = max_time / 1e6;

    printf("Batch size: %zu ticks\n", batch_size);
    printf("Iterations: %d\n", num_iterations);
    printf("Average latency: %.3f ms\n", avg_ms);
    printf("Min latency: %.3f ms\n", min_ms);
    printf("Max latency: %.3f ms\n", max_ms);
    printf("Target: <1ms - %s\n", avg_ms < 1.0 ? "PASS" : "FAIL");

    free(ticks);
    fc_ticker_destroy(ctx);
}

static void bench_precision_modes(void) {
    printf("\n=== Benchmark: Precision Mode Comparison ===\n");

    const uint32_t num_symbols = 1000;
    int64_t periods[] = {60000000000LL};

    const size_t num_ticks = 100000;
    fc_tick_t *ticks = (fc_tick_t *)malloc(num_ticks * sizeof(fc_tick_t));
    generate_random_ticks(ticks, num_ticks, num_symbols);

    fc_ticker_ctx_t *ctx_kahan =
        fc_ticker_create(num_symbols, 1, periods, FC_TICKER_PRECISION_KAHAN);
    int64_t start = get_time_ns();
    fc_ticker_update_batch(ctx_kahan, ticks, num_ticks);
    int64_t end = get_time_ns();
    double kahan_ms = (end - start) / 1e6;
    fc_ticker_destroy(ctx_kahan);

    fc_ticker_ctx_t *ctx_standard =
        fc_ticker_create(num_symbols, 1, periods, FC_TICKER_PRECISION_STANDARD);
    start = get_time_ns();
    fc_ticker_update_batch(ctx_standard, ticks, num_ticks);
    end = get_time_ns();
    double standard_ms = (end - start) / 1e6;
    fc_ticker_destroy(ctx_standard);

    printf("Kahan summation: %.3f ms\n", kahan_ms);
    printf("Standard summation: %.3f ms\n", standard_ms);
    printf("Overhead: %.1f%%\n", ((kahan_ms - standard_ms) / standard_ms) * 100.0);

    free(ticks);
}

void bench_ticker_run(void) {
    printf("\n");
    printf("========================================\n");
    printf("Ticker Performance Benchmarks\n");
    printf("========================================\n");

    srand(42);

    bench_single_symbol_single_period();
    bench_multi_symbol_single_period();
    bench_full_market_aggregation();
    bench_precision_modes();

    printf("\n========================================\n");
    printf("Ticker Benchmarks Complete\n");
    printf("========================================\n");
}
