/**
 * @file bench_ticker.c
 * @brief Performance benchmarks for ticker module
 */

#include "bench_framework.h"
#include "ticker.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void generate_random_ticks(fc_tick_t *ticks, size_t num_ticks, uint32_t num_symbols) {
    for (size_t i = 0; i < num_ticks; i++) {
        ticks[i].symbol_id = rand() % num_symbols;
        ticks[i].price = 100.0 + (rand() % 10000) / 100.0;
        ticks[i].volume = 100.0 + (rand() % 10000);
        ticks[i].amount = ticks[i].price * ticks[i].volume;
        ticks[i].timestamp_ns = 1000000000LL + i * 1000000LL;
    }
}

typedef struct {
    fc_ticker_ctx_t *ctx;
    fc_tick_t *ticks;
    size_t num_ticks;
} bench_ticker_data_t;

static void bench_ticker_update_fn(void* user_data) {
    bench_ticker_data_t* data = (bench_ticker_data_t*)user_data;
    for (size_t i = 0; i < data->num_ticks; i++) {
        fc_ticker_update(data->ctx, &data->ticks[i]);
    }
}

static void bench_single_symbol_single_period(void) {
    int64_t periods[] = {60000000000LL};
    fc_ticker_ctx_t *ctx = fc_ticker_create(1, 1, periods, FC_TICKER_PRECISION_KAHAN);
    if (ctx == NULL) {
        fprintf(stderr, "Failed to create ticker context\n");
        return;
    }

    const size_t num_ticks = 10000;
    fc_tick_t *ticks = (fc_tick_t *)malloc(num_ticks * sizeof(fc_tick_t));
    generate_random_ticks(ticks, num_ticks, 1);

    bench_ticker_data_t data = {
        .ctx = ctx,
        .ticks = ticks,
        .num_ticks = num_ticks
    };

    fc_bench_config_t config = FC_BENCH_CONFIG_DEFAULT;
    config.name = "ticker_single_symbol_10K";
    config.data_size = num_ticks * sizeof(fc_tick_t);
    config.min_time_ms = 100.0;

    fc_bench_result_t result;
    fc_bench_run(&config, bench_ticker_update_fn, &data, &result);
    fc_bench_result_print(&result);

    free(ticks);
    fc_ticker_destroy(ctx);
}

static void bench_multi_symbol_single_period(void) {
    const uint32_t num_symbols = 5000;
    int64_t periods[] = {60000000000LL};
    fc_ticker_ctx_t *ctx = fc_ticker_create(num_symbols, 1, periods, FC_TICKER_PRECISION_KAHAN);
    if (ctx == NULL) {
        fprintf(stderr, "Failed to create ticker context\n");
        return;
    }

    const size_t num_ticks = 10000;
    fc_tick_t *ticks = (fc_tick_t *)malloc(num_ticks * sizeof(fc_tick_t));
    generate_random_ticks(ticks, num_ticks, num_symbols);

    bench_ticker_data_t data = {
        .ctx = ctx,
        .ticks = ticks,
        .num_ticks = num_ticks
    };

    fc_bench_config_t config = FC_BENCH_CONFIG_DEFAULT;
    config.name = "ticker_5000_symbols_10K";
    config.data_size = num_ticks * sizeof(fc_tick_t);
    config.min_time_ms = 100.0;

    fc_bench_result_t result;
    fc_bench_run(&config, bench_ticker_update_fn, &data, &result);
    fc_bench_result_print(&result);

    free(ticks);
    fc_ticker_destroy(ctx);
}

typedef struct {
    fc_ticker_ctx_t *ctx;
    fc_tick_t *ticks;
    size_t batch_size;
} bench_batch_data_t;

static void bench_batch_fn(void* user_data) {
    bench_batch_data_t* data = (bench_batch_data_t*)user_data;
    fc_ticker_update_batch(data->ctx, data->ticks, data->batch_size);
}

static void bench_full_market_aggregation(void) {
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

    bench_batch_data_t data = {
        .ctx = ctx,
        .ticks = ticks,
        .batch_size = batch_size
    };

    fc_bench_config_t config = FC_BENCH_CONFIG_DEFAULT;
    config.name = "ticker_full_market_batch";
    config.data_size = batch_size * sizeof(fc_tick_t);
    config.min_time_ms = 100.0;
    config.min_iterations = 10;

    fc_bench_result_t result;
    fc_bench_run(&config, bench_batch_fn, &data, &result);
    fc_bench_result_print(&result);

    free(ticks);
    fc_ticker_destroy(ctx);
}

static void bench_precision_mode(fc_ticker_precision_mode_t mode, const char* mode_name) {
    const uint32_t num_symbols = 1000;
    int64_t periods[] = {60000000000LL};

    fc_ticker_ctx_t *ctx = fc_ticker_create(num_symbols, 1, periods, mode);
    if (ctx == NULL) {
        fprintf(stderr, "Failed to create ticker context\n");
        return;
    }

    const size_t num_ticks = 10000;
    fc_tick_t *ticks = (fc_tick_t *)malloc(num_ticks * sizeof(fc_tick_t));
    generate_random_ticks(ticks, num_ticks, num_symbols);

    bench_ticker_data_t data = {
        .ctx = ctx,
        .ticks = ticks,
        .num_ticks = num_ticks
    };

    char name[64];
    snprintf(name, sizeof(name), "ticker_precision_%s", mode_name);

    fc_bench_config_t config = FC_BENCH_CONFIG_DEFAULT;
    config.name = name;
    config.data_size = num_ticks * sizeof(fc_tick_t);
    config.min_time_ms = 100.0;

    fc_bench_result_t result;
    fc_bench_run(&config, bench_ticker_update_fn, &data, &result);
    fc_bench_result_print(&result);

    free(ticks);
    fc_ticker_destroy(ctx);
}

void bench_ticker_run(void) {
    printf("\n");
    printf("============================================================\n");
    printf("  Ticker Performance Benchmarks\n");
    printf("============================================================\n");
    printf("\n");

    srand(42);

    fc_bench_print_header();

    bench_single_symbol_single_period();
    bench_multi_symbol_single_period();
    bench_full_market_aggregation();

    printf("\nPrecision Mode Comparison\n");
    printf("------------------------------------------------------------\n");
    bench_precision_mode(FC_TICKER_PRECISION_STANDARD, "standard");
    bench_precision_mode(FC_TICKER_PRECISION_KAHAN, "kahan");
    bench_precision_mode(FC_TICKER_PRECISION_BIGFLOAT, "bigfloat");

    printf("\n");
}
