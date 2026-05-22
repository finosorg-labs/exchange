/**
 * @file bench_ticker_merge.c
 * @brief Benchmark for K-line merging performance
 */

#include "bench_framework.h"
#include "ticker_merge.h"
#include <stdio.h>
#include <stdlib.h>

#define NSEC_PER_SEC 1000000000LL

static void generate_ticks(fc_tick_t* ticks, size_t num_ticks, uint32_t num_symbols) {
    int64_t base_time = 1000000000000LL;
    for (size_t i = 0; i < num_ticks; i++) {
        ticks[i].symbol_id    = i % num_symbols;
        ticks[i].timestamp_ns = base_time + (i * 1000000LL);
        ticks[i].price        = 100.0 + (i % 100) * 0.01;
        ticks[i].volume       = 10.0;
        ticks[i].amount       = ticks[i].price * ticks[i].volume;
    }
}

static void bench_base_only_fn(void* user_data) {
    void** data_ptr              = (void**) user_data;
    fc_ticker_merge_ctx_t* ctx   = (fc_ticker_merge_ctx_t*) data_ptr[0];
    fc_tick_t* ticks             = (fc_tick_t*) data_ptr[1];
    size_t num_ticks             = (size_t) (uintptr_t) data_ptr[2];

    for (size_t i = 0; i < num_ticks; i++) {
        fc_ticker_merge_update(ctx, &ticks[i]);
    }
}

static void bench_5min_merge_fn(void* user_data) {
    void** data_ptr              = (void**) user_data;
    fc_ticker_merge_ctx_t* ctx   = (fc_ticker_merge_ctx_t*) data_ptr[0];
    fc_tick_t* ticks             = (fc_tick_t*) data_ptr[1];
    size_t num_ticks             = (size_t) (uintptr_t) data_ptr[2];

    for (size_t i = 0; i < num_ticks; i++) {
        fc_ticker_merge_update(ctx, &ticks[i]);
    }
}

static void bench_multiple_periods_fn(void* user_data) {
    void** data_ptr              = (void**) user_data;
    fc_ticker_merge_ctx_t* ctx   = (fc_ticker_merge_ctx_t*) data_ptr[0];
    fc_tick_t* ticks             = (fc_tick_t*) data_ptr[1];
    size_t num_ticks             = (size_t) (uintptr_t) data_ptr[2];

    for (size_t i = 0; i < num_ticks; i++) {
        fc_ticker_merge_update(ctx, &ticks[i]);
    }
}

static void bench_multi_symbol_fn(void* user_data) {
    void** data_ptr              = (void**) user_data;
    fc_ticker_merge_ctx_t* ctx   = (fc_ticker_merge_ctx_t*) data_ptr[0];
    fc_tick_t* ticks             = (fc_tick_t*) data_ptr[1];
    size_t num_ticks             = (size_t) (uintptr_t) data_ptr[2];

    for (size_t i = 0; i < num_ticks; i++) {
        fc_ticker_merge_update(ctx, &ticks[i]);
    }
}

static void bench_batch_update_fn(void* user_data) {
    void** data_ptr              = (void**) user_data;
    fc_ticker_merge_ctx_t* ctx   = (fc_ticker_merge_ctx_t*) data_ptr[0];
    fc_tick_t* ticks             = (fc_tick_t*) data_ptr[1];
    size_t num_ticks             = (size_t) (uintptr_t) data_ptr[2];

    fc_ticker_merge_update_batch(ctx, ticks, num_ticks);
}

static void run_ticker_merge_benchmarks(void) {
    printf("\nTicker Merge Benchmarks\n");
    printf("------------------------------------------------------------\n");

    const int64_t base_period_ns = 60000000000LL;
    const size_t num_ticks       = 100000;

    struct {
        const char* name;
        const int64_t* derived_periods;
        uint32_t num_derived_periods;
        uint32_t num_symbols;
        fc_bench_fn bench_fn;
    } tests[] = {
        {"TickerMerge/BaseOnly/100K", NULL, 0, 1, bench_base_only_fn},
        {"TickerMerge/5min/100K", (const int64_t[]){300000000000LL}, 1, 1, bench_5min_merge_fn},
        {
            "TickerMerge/Multi/5m-15m-1h/100K",
            (const int64_t[]){300000000000LL, 900000000000LL, 3600000000000LL},
            3,
            1,
            bench_multiple_periods_fn
        },
        {
            "TickerMerge/MultiSymbol/100/100K",
            (const int64_t[]){300000000000LL},
            1,
            100,
            bench_multi_symbol_fn
        },
        {
            "TickerMerge/Batch/100K",
            (const int64_t[]){300000000000LL},
            1,
            1,
            bench_batch_update_fn
        },
    };

    for (size_t i = 0; i < sizeof(tests) / sizeof(tests[0]); i++) {
        fc_ticker_merge_ctx_t* ctx = fc_ticker_merge_create(
            tests[i].num_symbols,
            base_period_ns,
            tests[i].derived_periods,
            tests[i].num_derived_periods,
            FC_TICKER_PRECISION_STANDARD,
            NULL,
            NULL
        );

        if (ctx == NULL) {
            fprintf(stderr, "Failed to create ticker_merge context\n");
            continue;
        }

        fc_tick_t* ticks = (fc_tick_t*) malloc(num_ticks * sizeof(fc_tick_t));
        if (ticks == NULL) {
            fc_ticker_merge_destroy(ctx);
            continue;
        }

        generate_ticks(ticks, num_ticks, tests[i].num_symbols);

        void* user_data[3] = {ctx, ticks, (void*) (uintptr_t) num_ticks};

        fc_bench_config_t config = FC_BENCH_CONFIG_DEFAULT;
        config.name              = tests[i].name;
        config.data_size         = num_ticks * sizeof(fc_tick_t);
        config.min_time_ms       = 100.0;
        config.quiet             = 0;

        fc_bench_result_t result;
        fc_bench_run(&config, tests[i].bench_fn, user_data, &result);

        free(ticks);
        fc_ticker_merge_destroy(ctx);
    }
}

static void run_precision_mode_benchmarks(void) {
    printf("\nPrecision Mode Benchmarks\n");
    printf("------------------------------------------------------------\n");

    const int64_t base_period_ns = 60000000000LL;
    const int64_t derived_periods[] = {300000000000LL};
    const size_t num_ticks = 10000;

    struct {
        const char* name;
        fc_ticker_precision_mode_t mode;
    } tests[] = {
        {"TickerMerge/Precision/Standard/10K", FC_TICKER_PRECISION_STANDARD},
        {"TickerMerge/Precision/Kahan/10K", FC_TICKER_PRECISION_KAHAN},
        {"TickerMerge/Precision/BigFloat/10K", FC_TICKER_PRECISION_BIGFLOAT},
    };

    for (size_t i = 0; i < sizeof(tests) / sizeof(tests[0]); i++) {
        fc_ticker_merge_ctx_t* ctx = fc_ticker_merge_create(
            1,
            base_period_ns,
            derived_periods,
            1,
            tests[i].mode,
            NULL,
            NULL
        );

        if (ctx == NULL) {
            fprintf(stderr, "Failed to create ticker_merge context\n");
            continue;
        }

        fc_tick_t* ticks = (fc_tick_t*) malloc(num_ticks * sizeof(fc_tick_t));
        if (ticks == NULL) {
            fc_ticker_merge_destroy(ctx);
            continue;
        }

        generate_ticks(ticks, num_ticks, 1);

        void* user_data[3] = {ctx, ticks, (void*) (uintptr_t) num_ticks};

        fc_bench_config_t config = FC_BENCH_CONFIG_DEFAULT;
        config.name              = tests[i].name;
        config.data_size         = num_ticks * sizeof(fc_tick_t);
        config.min_time_ms       = 100.0;
        config.quiet             = 0;

        fc_bench_result_t result;
        fc_bench_run(&config, bench_base_only_fn, user_data, &result);

        free(ticks);
        fc_ticker_merge_destroy(ctx);
    }
}

void bench_ticker_merge_run(void) {
    run_ticker_merge_benchmarks();
    run_precision_mode_benchmarks();
}
