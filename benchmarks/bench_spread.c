/**
 * @file bench_spread.c
 * @brief Benchmark for effective spread and Amihud illiquidity signal computation
 */

#include "bench_framework.h"
#include "mem_aligned.h"
#include "platform.h"
#include "signal/spread.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SMALL_BATCH  64
#define MEDIUM_BATCH 1024
#define LARGE_BATCH  10000

typedef struct {
    double* eff_out;
    double* illiq_out;
    double* trade_price;
    double* micro_price;
    double* returns;
    double* volume;
    size_t n;
} bench_spread_data_t;

static void setup_test_data(
    double* trade_price,
    double* micro_price,
    double* returns,
    double* volume,
    size_t n
) {
    for (size_t i = 0; i < n; i++) {
        trade_price[i] = 100.0 + ((double) i / (double) n) * 10.0;
        micro_price[i] = 100.0 + ((double) i / (double) n) * 9.5;
        returns[i]     = ((double) i / (double) n) * 0.02 - 0.01;
        volume[i]      = 1000.0 + ((double) i / (double) n) * 5000.0;
    }
}

static void bench_eff_spread_fn(void* user_data) {
    bench_spread_data_t* data = (bench_spread_data_t*) user_data;
    fc_ex_sig_eff_spread_batch(data->eff_out, data->trade_price, data->micro_price, data->n);
}

static void bench_amihud_fn(void* user_data) {
    bench_spread_data_t* data = (bench_spread_data_t*) user_data;
    fc_ex_sig_amihud_batch(data->illiq_out, data->returns, data->volume, data->n);
}

static void bench_eff_spread_impl(size_t n, const char* name) {
    bench_spread_data_t data;
    data.n           = n;
    data.eff_out     = fc_aligned_alloc(n * sizeof(double), 64);
    data.trade_price = fc_aligned_alloc(n * sizeof(double), 64);
    data.micro_price = fc_aligned_alloc(n * sizeof(double), 64);
    data.returns     = fc_aligned_alloc(n * sizeof(double), 64);
    data.volume      = fc_aligned_alloc(n * sizeof(double), 64);
    data.illiq_out   = fc_aligned_alloc(n * sizeof(double), 64);

    setup_test_data(data.trade_price, data.micro_price, data.returns, data.volume, n);

    fc_bench_config_t config = FC_BENCH_CONFIG_DEFAULT;
    config.name              = name;
    config.data_size         = n * sizeof(double) * 2;
    config.min_iterations    = 1000;

    fc_bench_result_t result;
    fc_bench_run(&config, bench_eff_spread_fn, &data, &result);
    fc_bench_result_print(&result);

    fc_aligned_free(data.eff_out);
    fc_aligned_free(data.trade_price);
    fc_aligned_free(data.micro_price);
    fc_aligned_free(data.returns);
    fc_aligned_free(data.volume);
    fc_aligned_free(data.illiq_out);
}

static void bench_amihud_impl(size_t n, const char* name) {
    bench_spread_data_t data;
    data.n           = n;
    data.eff_out     = fc_aligned_alloc(n * sizeof(double), 64);
    data.trade_price = fc_aligned_alloc(n * sizeof(double), 64);
    data.micro_price = fc_aligned_alloc(n * sizeof(double), 64);
    data.returns     = fc_aligned_alloc(n * sizeof(double), 64);
    data.volume      = fc_aligned_alloc(n * sizeof(double), 64);
    data.illiq_out   = fc_aligned_alloc(n * sizeof(double), 64);

    setup_test_data(data.trade_price, data.micro_price, data.returns, data.volume, n);

    fc_bench_config_t config = FC_BENCH_CONFIG_DEFAULT;
    config.name              = name;
    config.data_size         = n * sizeof(double) * 2;
    config.min_iterations    = 1000;

    fc_bench_result_t result;
    fc_bench_run(&config, bench_amihud_fn, &data, &result);
    fc_bench_result_print(&result);

    fc_aligned_free(data.eff_out);
    fc_aligned_free(data.trade_price);
    fc_aligned_free(data.micro_price);
    fc_aligned_free(data.returns);
    fc_aligned_free(data.volume);
    fc_aligned_free(data.illiq_out);
}

void bench_spread_run(void) {
    printf("\nSpread Signal Benchmarks\n");
    printf("------------------------------------------------------------\n");

    bench_eff_spread_impl(SMALL_BATCH, "EffSpread/Batch/N=64");
    bench_eff_spread_impl(MEDIUM_BATCH, "EffSpread/Batch/N=1024");
    bench_eff_spread_impl(LARGE_BATCH, "EffSpread/Batch/N=10000");

    printf("\n");

    bench_amihud_impl(SMALL_BATCH, "Amihud/Batch/N=64");
    bench_amihud_impl(MEDIUM_BATCH, "Amihud/Batch/N=1024");
    bench_amihud_impl(LARGE_BATCH, "Amihud/Batch/N=10000");

    printf("\nPerformance Targets:\n");
    printf("  Effective spread (SIMD):        ~5-10 ns per symbol\n");
    printf("  Amihud illiquidity (SIMD):      ~5-10 ns per symbol\n");
    printf("  Signal layer total budget:      ~0.5-2 μs\n");
}
