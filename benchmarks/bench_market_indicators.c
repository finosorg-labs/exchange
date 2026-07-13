/**
 * @file bench_market_indicators.c
 * @brief Performance benchmarks for realtime market indicators
 */

#include "bench_framework.h"
#include "market_indicators.h"
#include "simd_detect.h"
#include <stdio.h>
#include <stdlib.h>

#define SECOND_NS 1000000000LL

static void generate_market_trades(
    fc_market_trade_t* trades,
    size_t num_trades,
    uint32_t num_symbols
) {
    for (size_t i = 0; i < num_trades; i++) {
        double volume          = 100.0 + (double) (rand() % 10000);
        double buy             = volume * (double) (rand() % 100) / 100.0;
        trades[i].symbol_id    = (uint32_t) (i % num_symbols);
        trades[i].price        = 100.0 + (double) (rand() % 10000) / 100.0;
        trades[i].volume       = volume;
        trades[i].buy_volume   = buy;
        trades[i].sell_volume  = volume - buy;
        trades[i].timestamp_ns = SECOND_NS + (int64_t) i * 1000000LL;
    }
}

typedef struct {
    fc_market_indicators_ctx_t* ctx;
    fc_market_trade_t* trades;
    size_t num_trades;
} bench_update_data_t;

static void bench_update_fn(void* user_data) {
    bench_update_data_t* data = (bench_update_data_t*) user_data;
    fc_market_indicators_reset_all(data->ctx);
    fc_market_indicators_update_batch(data->ctx, data->trades, data->num_trades);
}

static void bench_single_symbol_update(void) {
    fc_market_indicators_ctx_t* ctx =
        fc_market_indicators_create(1, 60 * SECOND_NS, FC_MARKET_INDICATORS_PRECISION_KAHAN);
    if (ctx == NULL) {
        fprintf(stderr, "Failed to create market indicators context\n");
        return;
    }

    const size_t num_trades   = 10000;
    fc_market_trade_t* trades = (fc_market_trade_t*) malloc(num_trades * sizeof(fc_market_trade_t));
    generate_market_trades(trades, num_trades, 1);

    bench_update_data_t data = {.ctx = ctx, .trades = trades, .num_trades = num_trades};
    fc_bench_config_t config = FC_BENCH_CONFIG_DEFAULT;
    config.name              = "market_indicators_single_symbol_10K";
    config.data_size         = num_trades * sizeof(fc_market_trade_t);
    config.min_time_ms       = 100.0;

    fc_bench_result_t result;
    fc_bench_run(&config, bench_update_fn, &data, &result);
    fc_bench_result_print(&result);

    free(trades);
    fc_market_indicators_destroy(ctx);
}

static void bench_full_market_update(void) {
    const uint32_t num_symbols      = 5000;
    fc_market_indicators_ctx_t* ctx = fc_market_indicators_create(
        num_symbols, 60 * SECOND_NS, FC_MARKET_INDICATORS_PRECISION_KAHAN
    );
    if (ctx == NULL) {
        fprintf(stderr, "Failed to create market indicators context\n");
        return;
    }

    const size_t num_trades   = 10000;
    fc_market_trade_t* trades = (fc_market_trade_t*) malloc(num_trades * sizeof(fc_market_trade_t));
    generate_market_trades(trades, num_trades, num_symbols);

    bench_update_data_t data = {.ctx = ctx, .trades = trades, .num_trades = num_trades};
    fc_bench_config_t config = FC_BENCH_CONFIG_DEFAULT;
    config.name              = "market_indicators_5000_symbols_10K";
    config.data_size         = num_trades * sizeof(fc_market_trade_t);
    config.min_time_ms       = 100.0;

    fc_bench_result_t result;
    fc_bench_run(&config, bench_update_fn, &data, &result);
    fc_bench_result_print(&result);

    free(trades);
    fc_market_indicators_destroy(ctx);
}

typedef struct {
    fc_market_indicators_ctx_t* ctx;
    fc_market_indicators_t* values;
} bench_get_all_data_t;

static void bench_get_all_fn(void* user_data) {
    bench_get_all_data_t* data = (bench_get_all_data_t*) user_data;
    fc_market_indicators_get_all(data->ctx, data->values);
}

static void bench_full_market_get_all(void) {
    const uint32_t num_symbols      = 5000;
    fc_market_indicators_ctx_t* ctx = fc_market_indicators_create(
        num_symbols, 60 * SECOND_NS, FC_MARKET_INDICATORS_PRECISION_KAHAN
    );
    if (ctx == NULL) {
        fprintf(stderr, "Failed to create market indicators context\n");
        return;
    }

    fc_market_trade_t* trades =
        (fc_market_trade_t*) malloc(num_symbols * sizeof(fc_market_trade_t));
    fc_market_indicators_t* values =
        (fc_market_indicators_t*) malloc(num_symbols * sizeof(fc_market_indicators_t));
    generate_market_trades(trades, num_symbols, num_symbols);
    fc_market_indicators_update_batch(ctx, trades, num_symbols);

    bench_get_all_data_t data = {.ctx = ctx, .values = values};
    fc_bench_config_t config  = FC_BENCH_CONFIG_DEFAULT;
    config.name               = "market_indicators_get_all_5000";
    config.data_size          = num_symbols * sizeof(fc_market_indicators_t);
    config.min_time_ms        = 100.0;

    fc_bench_result_t result;
    fc_bench_run(&config, bench_get_all_fn, &data, &result);
    fc_bench_result_print(&result);

    free(values);
    free(trades);
    fc_market_indicators_destroy(ctx);
}

typedef struct {
    fc_market_indicators_ctx_t* ctx;
    uint32_t* symbols;
    double* values;
    size_t count;
} bench_rank_data_t;

static void bench_rank_fn(void* user_data) {
    bench_rank_data_t* data = (bench_rank_data_t*) user_data;
    size_t out_count        = 0;
    fc_market_indicators_rank(
        data->ctx, FC_MARKET_INDICATOR_VWAP, data->symbols, data->values, data->count, &out_count, 1
    );
}

static void bench_full_market_ranking(void) {
    const uint32_t num_symbols      = 5000;
    fc_market_indicators_ctx_t* ctx = fc_market_indicators_create(
        num_symbols, 60 * SECOND_NS, FC_MARKET_INDICATORS_PRECISION_KAHAN
    );
    if (ctx == NULL) {
        fprintf(stderr, "Failed to create market indicators context\n");
        return;
    }

    fc_market_trade_t* trades =
        (fc_market_trade_t*) malloc(num_symbols * sizeof(fc_market_trade_t));
    uint32_t* symbols = (uint32_t*) malloc(num_symbols * sizeof(uint32_t));
    double* values    = (double*) malloc(num_symbols * sizeof(double));
    generate_market_trades(trades, num_symbols, num_symbols);
    fc_market_indicators_update_batch(ctx, trades, num_symbols);

    bench_rank_data_t data = {
        .ctx = ctx, .symbols = symbols, .values = values, .count = num_symbols
    };
    fc_bench_config_t config = FC_BENCH_CONFIG_DEFAULT;
    config.name              = "market_indicators_rank_vwap_5000";
    config.data_size         = num_symbols * (sizeof(uint32_t) + sizeof(double));
    config.min_time_ms       = 100.0;

    fc_bench_result_t result;
    fc_bench_run(&config, bench_rank_fn, &data, &result);
    fc_bench_result_print(&result);

    free(values);
    free(symbols);
    free(trades);
    fc_market_indicators_destroy(ctx);
}

typedef struct {
    fc_market_indicators_ctx_t* ctx;
    fc_market_trade_t* trades;
    size_t num_trades;
    int force_scalar;
} bench_update_batch_data_t;

static void bench_update_batch_fn(void* user_data) {
    bench_update_batch_data_t* data = (bench_update_batch_data_t*) user_data;
    fc_simd_level_t saved           = g_fc_simd_level;
    if (data->force_scalar) {
        g_fc_simd_level = FC_SIMD_SCALAR;
    }
    fc_market_indicators_reset_all(data->ctx);
    fc_market_indicators_update_batch(data->ctx, data->trades, data->num_trades);
    g_fc_simd_level = saved;
}

static void bench_update_batch_at_scale(
    size_t num_trades,
    uint32_t num_symbols,
    fc_market_indicators_precision_mode_t precision,
    const char* label
) {
    fc_market_indicators_ctx_t* ctx =
        fc_market_indicators_create(num_symbols, 60 * SECOND_NS, precision);
    if (ctx == NULL) {
        fprintf(stderr, "Failed to create context for %s\n", label);
        return;
    }

    fc_market_trade_t* trades = (fc_market_trade_t*) malloc(num_trades * sizeof(fc_market_trade_t));
    generate_market_trades(trades, num_trades, num_symbols);

    /* SIMD run */
    bench_update_batch_data_t simd_data = {
        .ctx = ctx, .trades = trades, .num_trades = num_trades, .force_scalar = 0
    };
    fc_bench_config_t config = FC_BENCH_CONFIG_DEFAULT;
    char name_buf[128];
    snprintf(name_buf, sizeof(name_buf), "%s_simd", label);
    config.name        = name_buf;
    config.data_size   = num_trades * sizeof(fc_market_trade_t);
    config.min_time_ms = 100.0;

    fc_bench_result_t simd_result;
    fc_bench_run(&config, bench_update_batch_fn, &simd_data, &simd_result);

    /* Scalar run */
    bench_update_batch_data_t scalar_data = {
        .ctx = ctx, .trades = trades, .num_trades = num_trades, .force_scalar = 1
    };
    snprintf(name_buf, sizeof(name_buf), "%s_scalar", label);
    config.name = name_buf;

    fc_bench_result_t scalar_result;
    fc_bench_run(&config, bench_update_batch_fn, &scalar_data, &scalar_result);

    fc_bench_result_print(&simd_result);
    fc_bench_result_print(&scalar_result);

    double simd_ns_per_trade   = simd_result.mean_ns / (double) num_trades;
    double scalar_ns_per_trade = scalar_result.mean_ns / (double) num_trades;
    double mtrades_simd        = (double) num_trades / simd_result.mean_ns / 1000.0;
    double mtrades_scalar      = (double) num_trades / scalar_result.mean_ns / 1000.0;
    double speedup             = scalar_result.mean_ns / simd_result.mean_ns;

    printf(
        "  %-24s  simd_ns/trade=%.2f  scalar_ns/trade=%.2f  "
        "simd=%.1f Mt/s  scalar=%.1f Mt/s  speedup=%.2fx\n",
        label,
        simd_ns_per_trade,
        scalar_ns_per_trade,
        mtrades_simd,
        mtrades_scalar,
        speedup
    );

    free(trades);
    fc_market_indicators_destroy(ctx);
}

static void bench_update_batch_compare(void) {
    const uint32_t num_symbols = 64;
    const size_t scales[]      = {1000, 10000, 100000};
    const char* scale_labels[] = {"batch_64sym_1K", "batch_64sym_10K", "batch_64sym_100K"};

    for (int i = 0; i < 3; i++) {
        bench_update_batch_at_scale(
            scales[i], num_symbols, FC_MARKET_INDICATORS_PRECISION_KAHAN, scale_labels[i]
        );
    }

    bench_update_batch_at_scale(
        100000, num_symbols, FC_MARKET_INDICATORS_PRECISION_STANDARD, "batch_64sym_100K_standard"
    );
}

void bench_market_indicators_run(void) {
    srand(42);
#if FC_ARCH_X86_64
    fc_init();
#endif
    printf("\n=== Market Indicators Benchmarks ===\n");
    bench_single_symbol_update();
    bench_full_market_update();
    bench_full_market_get_all();
    bench_full_market_ranking();
    printf("\n--- SIMD vs Scalar Comparison ---\n");
    bench_update_batch_compare();
}
