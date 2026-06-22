/**
 * @file bench_ofi.c
 * @brief Performance benchmarks for OFI computation
 */

#include "signal/ofi.h"
#include "bench_framework.h"
#include <platform.h>
#include <simd_detect.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    double* ofi_out;
    double* bid_p_cur;
    double* bid_q_cur;
    double* ask_p_cur;
    double* ask_q_cur;
    double* bid_p_prev;
    double* bid_q_prev;
    double* ask_p_prev;
    double* ask_q_prev;
    double* level_weights;
    size_t n_symbols;
    int n_levels;
} bench_ofi_batch_data_t;

typedef struct {
    double* integral_out;
    double* ofi_series;
    size_t n_symbols;
    size_t T;
} bench_ofi_integral_data_t;

static void bench_ofi_batch_fn(void* user_data) {
    bench_ofi_batch_data_t* data = (bench_ofi_batch_data_t*)user_data;
    fc_ex_sig_ofi_batch(
        data->ofi_out,
        data->bid_p_cur, data->bid_q_cur,
        data->ask_p_cur, data->ask_q_cur,
        data->bid_p_prev, data->bid_q_prev,
        data->ask_p_prev, data->ask_q_prev,
        data->level_weights,
        data->n_symbols, data->n_levels
    );
}

static void bench_ofi_integral_fn(void* user_data) {
    bench_ofi_integral_data_t* data = (bench_ofi_integral_data_t*)user_data;
    fc_ex_sig_ofi_integral(data->integral_out, data->ofi_series, data->n_symbols, data->T);
}

static void bench_ofi_batch_impl(size_t n_symbols, int n_levels, const char* name) {
    const size_t total_elements = n_symbols * n_levels;

    double* bid_p_cur = aligned_alloc(64, total_elements * sizeof(double));
    double* bid_q_cur = aligned_alloc(64, total_elements * sizeof(double));
    double* ask_p_cur = aligned_alloc(64, total_elements * sizeof(double));
    double* ask_q_cur = aligned_alloc(64, total_elements * sizeof(double));
    double* bid_p_prev = aligned_alloc(64, total_elements * sizeof(double));
    double* bid_q_prev = aligned_alloc(64, total_elements * sizeof(double));
    double* ask_p_prev = aligned_alloc(64, total_elements * sizeof(double));
    double* ask_q_prev = aligned_alloc(64, total_elements * sizeof(double));
    double* level_weights = aligned_alloc(64, n_levels * sizeof(double));
    double* ofi_out = aligned_alloc(64, n_symbols * sizeof(double));

    if (!bid_p_cur || !ofi_out) {
        fprintf(stderr, "Memory allocation failed\n");
        return;
    }

    for (int lv = 0; lv < n_levels; lv++) {
        level_weights[lv] = 1.0 / ((lv + 1) * (lv + 1));
    }

    for (size_t i = 0; i < total_elements; i++) {
        int sym = i / n_levels;
        int lv = i % n_levels;
        double base_price = 100.0 + sym * 0.5;

        bid_p_prev[i] = base_price - lv * 0.01;
        bid_q_prev[i] = 500.0 - lv * 50.0;
        ask_p_prev[i] = base_price + 0.01 + lv * 0.01;
        ask_q_prev[i] = 400.0 - lv * 40.0;

        bid_p_cur[i] = bid_p_prev[i] + ((sym + lv) % 3 == 0 ? 0.01 : 0.0);
        bid_q_cur[i] = bid_q_prev[i] + (sym % 2 == 0 ? 50.0 : -30.0);
        ask_p_cur[i] = ask_p_prev[i] + ((sym + lv) % 5 == 0 ? -0.01 : 0.0);
        ask_q_cur[i] = ask_q_prev[i] + (sym % 3 == 0 ? -40.0 : 20.0);
    }

    bench_ofi_batch_data_t data = {
        .ofi_out = ofi_out,
        .bid_p_cur = bid_p_cur,
        .bid_q_cur = bid_q_cur,
        .ask_p_cur = ask_p_cur,
        .ask_q_cur = ask_q_cur,
        .bid_p_prev = bid_p_prev,
        .bid_q_prev = bid_q_prev,
        .ask_p_prev = ask_p_prev,
        .ask_q_prev = ask_q_prev,
        .level_weights = level_weights,
        .n_symbols = n_symbols,
        .n_levels = n_levels
    };

    fc_bench_config_t config = FC_BENCH_CONFIG_DEFAULT;
    config.name = name;
    config.data_size = total_elements * sizeof(double) * 8;
    config.min_iterations = 100;
    config.min_time_ms = 100.0;

    fc_bench_result_t result;
    fc_bench_run(&config, bench_ofi_batch_fn, &data, &result);
    fc_bench_result_print(&result);

    free(bid_p_cur);
    free(bid_q_cur);
    free(ask_p_cur);
    free(ask_q_cur);
    free(bid_p_prev);
    free(bid_q_prev);
    free(ask_p_prev);
    free(ask_q_prev);
    free(level_weights);
    free(ofi_out);
}

static void bench_ofi_integral_impl(size_t n_symbols, size_t T, const char* name) {
    const size_t total_elements = n_symbols * T;

    double* ofi_series = aligned_alloc(64, total_elements * sizeof(double));
    double* integral_out = aligned_alloc(64, n_symbols * sizeof(double));

    if (!ofi_series || !integral_out) {
        fprintf(stderr, "Memory allocation failed\n");
        return;
    }

    for (size_t i = 0; i < total_elements; i++) {
        ofi_series[i] = ((double)(i % 201) - 100.0) * 5.0;
    }

    bench_ofi_integral_data_t data = {
        .integral_out = integral_out,
        .ofi_series = ofi_series,
        .n_symbols = n_symbols,
        .T = T
    };

    fc_bench_config_t config = FC_BENCH_CONFIG_DEFAULT;
    config.name = name;
    config.data_size = total_elements * sizeof(double);
    config.min_iterations = 100;
    config.min_time_ms = 100.0;

    fc_bench_result_t result;
    fc_bench_run(&config, bench_ofi_integral_fn, &data, &result);
    fc_bench_result_print(&result);

    free(ofi_series);
    free(integral_out);
}

void bench_ofi_run(void) {
    printf("\nOFI Signal Benchmarks\n");
    printf("------------------------------------------------------------\n");

    bench_ofi_batch_impl(100, 1, "OFI/Batch/Symbols=100/Levels=1");
    bench_ofi_batch_impl(1000, 1, "OFI/Batch/Symbols=1000/Levels=1");
    bench_ofi_batch_impl(10000, 1, "OFI/Batch/Symbols=10000/Levels=1");

    bench_ofi_batch_impl(1000, 5, "OFI/Batch/Symbols=1000/Levels=5");
    bench_ofi_batch_impl(1000, 10, "OFI/Batch/Symbols=1000/Levels=10");

    bench_ofi_batch_impl(5000, 5, "OFI/Batch/Symbols=5000/Levels=5");

    bench_ofi_integral_impl(100, 1000, "OFI/Integral/Symbols=100/Window=1000");
    bench_ofi_integral_impl(1000, 1000, "OFI/Integral/Symbols=1000/Window=1000");
    bench_ofi_integral_impl(5000, 1000, "OFI/Integral/Symbols=5000/Window=1000");

    printf("\nPerformance Targets:\n");
    printf("  Single-tick OFI (pure Go):      50-500 ns\n");
    printf("  Batch OFI (SIMD):               ~200-500 ns per symbol\n");
    printf("  OFI integral:                   ~100-200 ns\n");
    printf("  Signal layer total budget:      ~0.5-2 μs\n");
}
