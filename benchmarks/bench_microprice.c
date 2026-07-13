/**
 * @file bench_microprice.c
 * @brief Benchmarks for micro-price signal computation
 */

#include "bench_framework.h"
#include "mem_aligned.h"
#include "platform.h"
#include "signal/microprice.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SMALL_BATCH  64
#define MEDIUM_BATCH 1024
#define LARGE_BATCH  10000

typedef struct {
    double* mp_out;
    double* bid_p;
    double* bid_q;
    double* ask_p;
    double* ask_q;
    size_t n;
} bench_microprice_data_t;

static void setup_test_data(double* bid_p, double* bid_q, double* ask_p, double* ask_q, size_t n) {
    for (size_t i = 0; i < n; i++) {
        bid_p[i] = 100.0 + (double) (i % 100) * 0.1;
        bid_q[i] = 500.0 + (double) (i % 1000);
        ask_p[i] = bid_p[i] + 0.5;
        ask_q[i] = 500.0 + (double) ((n - i) % 1000);
    }
}

static void bench_microprice_fn(void* user_data) {
    bench_microprice_data_t* data = (bench_microprice_data_t*) user_data;
    fc_ex_sig_microprice_batch(
        data->mp_out, data->bid_p, data->bid_q, data->ask_p, data->ask_q, data->n
    );
}

static void bench_microprice_impl(size_t n, const char* name) {
    bench_microprice_data_t data;
    data.n      = n;
    data.mp_out = fc_aligned_alloc(n * sizeof(double), 64);
    data.bid_p  = fc_aligned_alloc(n * sizeof(double), 64);
    data.bid_q  = fc_aligned_alloc(n * sizeof(double), 64);
    data.ask_p  = fc_aligned_alloc(n * sizeof(double), 64);
    data.ask_q  = fc_aligned_alloc(n * sizeof(double), 64);

    setup_test_data(data.bid_p, data.bid_q, data.ask_p, data.ask_q, n);

    fc_bench_config_t config = FC_BENCH_CONFIG_DEFAULT;
    config.name              = name;
    config.data_size         = n * sizeof(double) * 5;
    config.min_iterations    = 1000;

    fc_bench_result_t result;
    fc_bench_run(&config, bench_microprice_fn, &data, &result);
    fc_bench_result_print(&result);

    fc_aligned_free(data.mp_out);
    fc_aligned_free(data.bid_p);
    fc_aligned_free(data.bid_q);
    fc_aligned_free(data.ask_p);
    fc_aligned_free(data.ask_q);
}

void bench_microprice_run(void) {
    printf("\nMicro-price Signal Benchmarks\n");
    printf("------------------------------------------------------------\n");

    bench_microprice_impl(SMALL_BATCH, "Microprice/Batch/N=64");
    bench_microprice_impl(MEDIUM_BATCH, "Microprice/Batch/N=1024");
    bench_microprice_impl(LARGE_BATCH, "Microprice/Batch/N=10000");

    printf("\nPerformance Targets:\n");
    printf("  Single micro-price (pure Go):   ~50-100 ns\n");
    printf("  Batch micro-price (SIMD):       ~5-10 ns per symbol\n");
    printf("  Signal layer total budget:      ~0.5-2 μs\n");
}
