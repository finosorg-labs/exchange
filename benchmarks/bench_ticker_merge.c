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
    fc_tick_t* tick              = (fc_tick_t*) data_ptr[1];

    fc_ticker_merge_update(ctx, tick);
}

static void bench_5min_merge_fn(void* user_data) {
    void** data_ptr              = (void**) user_data;
    fc_ticker_merge_ctx_t* ctx   = (fc_ticker_merge_ctx_t*) data_ptr[0];
    fc_tick_t* tick              = (fc_tick_t*) data_ptr[1];

    fc_ticker_merge_update(ctx, tick);
}

static void bench_multiple_periods_fn(void* user_data) {
    void** data_ptr              = (void**) user_data;
    fc_ticker_merge_ctx_t* ctx   = (fc_ticker_merge_ctx_t*) data_ptr[0];
    fc_tick_t* tick              = (fc_tick_t*) data_ptr[1];

    fc_ticker_merge_update(ctx, tick);
}

static void bench_multi_symbol_fn(void* user_data) {
    void** data_ptr              = (void**) user_data;
    fc_ticker_merge_ctx_t* ctx   = (fc_ticker_merge_ctx_t*) data_ptr[0];
    fc_tick_t* tick              = (fc_tick_t*) data_ptr[1];

    fc_ticker_merge_update(ctx, tick);
}

static void bench_batch_update_fn(void* user_data) {
    void** data_ptr              = (void**) user_data;
    fc_ticker_merge_ctx_t* ctx   = (fc_ticker_merge_ctx_t*) data_ptr[0];
    fc_tick_t* ticks             = (fc_tick_t*) data_ptr[1];
    size_t batch_size            = (size_t) (uintptr_t) data_ptr[2];

    fc_ticker_merge_update_batch(ctx, ticks, batch_size);
}

static void run_ticker_merge_benchmarks(void) {
    printf("\nTicker Merge Benchmarks\n");
    printf("------------------------------------------------------------\n");

    const int64_t base_period_ns = 60000000000LL;

    struct {
        const char* name;
        const int64_t* derived_periods;
        uint32_t num_derived_periods;
        uint32_t num_symbols;
        fc_bench_fn bench_fn;
    } tests[] = {
        {"TickerMerge/BaseOnly", NULL, 0, 1, bench_base_only_fn},
        {"TickerMerge/5min", (const int64_t[]){300000000000LL}, 1, 1, bench_5min_merge_fn},
        {
            "TickerMerge/Multi/5m-15m-1h",
            (const int64_t[]){300000000000LL, 900000000000LL, 3600000000000LL},
            3,
            1,
            bench_multiple_periods_fn
        },
        {
            "TickerMerge/MultiSymbol/100",
            (const int64_t[]){300000000000LL},
            1,
            100,
            bench_multi_symbol_fn
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

        fc_tick_t tick;
        tick.symbol_id    = 0;
        tick.timestamp_ns = 1000000000000LL;
        tick.price        = 100.0;
        tick.volume       = 10.0;
        tick.amount       = 1000.0;

        void* user_data[2] = {ctx, &tick};

        fc_bench_config_t config = FC_BENCH_CONFIG_DEFAULT;
        config.name              = tests[i].name;
        config.data_size         = sizeof(fc_tick_t);
        config.min_time_ms       = 100.0;
        config.quiet             = 0;

        fc_bench_result_t result;
        fc_bench_run(&config, tests[i].bench_fn, user_data, &result);

        fc_ticker_merge_destroy(ctx);
    }

    // Batch update benchmark
    {
        const size_t batch_size = 1000;
        fc_ticker_merge_ctx_t* ctx = fc_ticker_merge_create(
            1,
            base_period_ns,
            (const int64_t[]){300000000000LL},
            1,
            FC_TICKER_PRECISION_STANDARD,
            NULL,
            NULL
        );

        if (ctx != NULL) {
            fc_tick_t* ticks = (fc_tick_t*) malloc(batch_size * sizeof(fc_tick_t));
            if (ticks != NULL) {
                generate_ticks(ticks, batch_size, 1);

                void* user_data[3] = {ctx, ticks, (void*) (uintptr_t) batch_size};

                fc_bench_config_t config = FC_BENCH_CONFIG_DEFAULT;
                config.name              = "TickerMerge/Batch/1K";
                config.data_size         = batch_size * sizeof(fc_tick_t);
                config.min_time_ms       = 100.0;
                config.quiet             = 0;

                fc_bench_result_t result;
                fc_bench_run(&config, bench_batch_update_fn, user_data, &result);

                free(ticks);
            }
            fc_ticker_merge_destroy(ctx);
        }
    }
}

static void run_precision_mode_benchmarks(void) {
    printf("\nPrecision Mode Benchmarks\n");
    printf("------------------------------------------------------------\n");

    const int64_t base_period_ns = 60000000000LL;
    const int64_t derived_periods[] = {300000000000LL};

    struct {
        const char* name;
        fc_ticker_precision_mode_t mode;
    } tests[] = {
        {"TickerMerge/Precision/Standard", FC_TICKER_PRECISION_STANDARD},
        {"TickerMerge/Precision/Kahan", FC_TICKER_PRECISION_KAHAN},
        {"TickerMerge/Precision/BigFloat", FC_TICKER_PRECISION_BIGFLOAT},
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

        fc_tick_t tick;
        tick.symbol_id    = 0;
        tick.timestamp_ns = 1000000000000LL;
        tick.price        = 100.0;
        tick.volume       = 10.0;
        tick.amount       = 1000.0;

        void* user_data[2] = {ctx, &tick};

        fc_bench_config_t config = FC_BENCH_CONFIG_DEFAULT;
        config.name              = tests[i].name;
        config.data_size         = sizeof(fc_tick_t);
        config.min_time_ms       = 100.0;
        config.quiet             = 0;

        fc_bench_result_t result;
        fc_bench_run(&config, bench_base_only_fn, user_data, &result);

        fc_ticker_merge_destroy(ctx);
    }
}

static void bench_merge_operation_fn(void* user_data) {
    void** data_ptr              = (void**) user_data;
    fc_ticker_merge_ctx_t* ctx   = (fc_ticker_merge_ctx_t*) data_ptr[0];
    fc_tick_t* ticks             = (fc_tick_t*) data_ptr[1];
    size_t num_ticks             = (size_t) (uintptr_t) data_ptr[2];

    for (size_t i = 0; i < num_ticks; i++) {
        fc_ticker_merge_update(ctx, &ticks[i]);
    }
}

static void run_merge_count_benchmarks(void) {
    printf("\nMerge Count Benchmarks\n");
    printf("------------------------------------------------------------\n");

    const int64_t base_period_ns = 60000000000LL; // 1 minute

    struct {
        const char* name;
        int64_t derived_period_ns;
        size_t merge_count;
        fc_ticker_precision_mode_t mode;
    } tests[] = {
        {"TickerMerge/MergeCount/5/Standard", 300000000000LL, 5, FC_TICKER_PRECISION_STANDARD},
        {"TickerMerge/MergeCount/5/Kahan", 300000000000LL, 5, FC_TICKER_PRECISION_KAHAN},
        {"TickerMerge/MergeCount/5/BigFloat", 300000000000LL, 5, FC_TICKER_PRECISION_BIGFLOAT},
        {"TickerMerge/MergeCount/15/Standard", 900000000000LL, 15, FC_TICKER_PRECISION_STANDARD},
        {"TickerMerge/MergeCount/15/Kahan", 900000000000LL, 15, FC_TICKER_PRECISION_KAHAN},
        {"TickerMerge/MergeCount/15/BigFloat", 900000000000LL, 15, FC_TICKER_PRECISION_BIGFLOAT},
        {"TickerMerge/MergeCount/60/Standard", 3600000000000LL, 60, FC_TICKER_PRECISION_STANDARD},
        {"TickerMerge/MergeCount/60/Kahan", 3600000000000LL, 60, FC_TICKER_PRECISION_KAHAN},
        {"TickerMerge/MergeCount/60/BigFloat", 3600000000000LL, 60, FC_TICKER_PRECISION_BIGFLOAT},
        {"TickerMerge/MergeCount/240/Standard", 14400000000000LL, 240, FC_TICKER_PRECISION_STANDARD},
        {"TickerMerge/MergeCount/240/Kahan", 14400000000000LL, 240, FC_TICKER_PRECISION_KAHAN},
        {"TickerMerge/MergeCount/240/BigFloat", 14400000000000LL, 240, FC_TICKER_PRECISION_BIGFLOAT},
        {"TickerMerge/MergeCount/1440/Standard", 86400000000000LL, 1440, FC_TICKER_PRECISION_STANDARD},
        {"TickerMerge/MergeCount/1440/Kahan", 86400000000000LL, 1440, FC_TICKER_PRECISION_KAHAN},
        {"TickerMerge/MergeCount/1440/BigFloat", 86400000000000LL, 1440, FC_TICKER_PRECISION_BIGFLOAT},
    };

    for (size_t i = 0; i < sizeof(tests) / sizeof(tests[0]); i++) {
        fc_ticker_merge_ctx_t* ctx = fc_ticker_merge_create(
            1,
            base_period_ns,
            &tests[i].derived_period_ns,
            1,
            tests[i].mode,
            NULL,
            NULL
        );

        if (ctx == NULL) {
            fprintf(stderr, "Failed to create ticker_merge context for %s\n", tests[i].name);
            continue;
        }

        // Generate enough ticks to trigger one merge + 1 extra to complete
        size_t num_ticks = tests[i].merge_count + 1;
        fc_tick_t* ticks = (fc_tick_t*) malloc(num_ticks * sizeof(fc_tick_t));
        if (ticks == NULL) {
            fc_ticker_merge_destroy(ctx);
            continue;
        }

        int64_t base_time = 1000000000000LL;
        for (size_t j = 0; j < num_ticks; j++) {
            ticks[j].symbol_id    = 0;
            ticks[j].timestamp_ns = base_time + (j * base_period_ns);
            ticks[j].price        = 100.0 + (j % 10) * 0.1;
            ticks[j].volume       = 10.0;
            ticks[j].amount       = ticks[j].price * ticks[j].volume;
        }

        void* user_data[3] = {ctx, ticks, (void*) (uintptr_t) num_ticks};

        fc_bench_config_t config = FC_BENCH_CONFIG_DEFAULT;
        config.name              = tests[i].name;
        config.data_size         = num_ticks * sizeof(fc_tick_t);
        config.min_time_ms       = 100.0;
        config.quiet             = 0;

        fc_bench_result_t result;
        fc_bench_run(&config, bench_merge_operation_fn, user_data, &result);

        free(ticks);
        fc_ticker_merge_destroy(ctx);
    }
}

void bench_ticker_merge_run(void) {
    run_ticker_merge_benchmarks();
    run_precision_mode_benchmarks();
    run_merge_count_benchmarks();
}
