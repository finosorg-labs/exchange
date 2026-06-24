/**
 * @file bench_feature.c
 * @brief Benchmarks for feature extraction from order book
 */

#include "signal/feature.h"
#include "bench_framework.h"
#include "platform.h"
#include "mem_aligned.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define SMALL_BATCH 64
#define MEDIUM_BATCH 1024
#define LARGE_BATCH 10000

typedef struct {
    double* features;
    double* bid_p;
    double* bid_q;
    double* ask_p;
    double* ask_q;
    size_t n_symbols;
    int n_levels;
} bench_feature_data_t;

static void setup_test_data(
    double* bid_p,
    double* bid_q,
    double* ask_p,
    double* ask_q,
    size_t n_symbols,
    int n_levels
) {
    for (size_t i = 0; i < n_symbols; i++) {
        for (int k = 0; k < n_levels; k++) {
            size_t idx = k * n_symbols + i;
            bid_p[idx] = 100.0 + (double)(i % 100) * 0.1 - (double)k * 0.1;
            bid_q[idx] = 500.0 + (double)(i % 1000) - (double)k * 100.0;
            ask_p[idx] = bid_p[idx] + 0.5 + (double)k * 0.1;
            ask_q[idx] = 500.0 + (double)((n_symbols - i) % 1000) - (double)k * 100.0;
        }
    }
}

static void bench_feature_extract_fn(void* user_data) {
    bench_feature_data_t* data = (bench_feature_data_t*)user_data;
    fc_ex_sig_feature_extract(
        data->features,
        data->bid_p,
        data->bid_q,
        data->ask_p,
        data->ask_q,
        data->n_symbols,
        data->n_levels
    );
}

static void bench_feature_extract_core_fn(void* user_data) {
    bench_feature_data_t* data = (bench_feature_data_t*)user_data;
    fc_ex_sig_feature_extract_core(
        data->features,
        data->bid_p,
        data->bid_q,
        data->ask_p,
        data->ask_q,
        data->n_symbols,
        data->n_levels
    );
}

static void bench_feature_impl(size_t n_symbols, int n_levels, const char* name) {
    const size_t n_features = fc_ex_sig_feature_count(n_levels);
    bench_feature_data_t data;
    data.n_symbols = n_symbols;
    data.n_levels = n_levels;
    data.features = fc_aligned_alloc(n_symbols * n_features * sizeof(double), 64);
    data.bid_p = fc_aligned_alloc(n_symbols * n_levels * sizeof(double), 64);
    data.bid_q = fc_aligned_alloc(n_symbols * n_levels * sizeof(double), 64);
    data.ask_p = fc_aligned_alloc(n_symbols * n_levels * sizeof(double), 64);
    data.ask_q = fc_aligned_alloc(n_symbols * n_levels * sizeof(double), 64);

    setup_test_data(data.bid_p, data.bid_q, data.ask_p, data.ask_q, n_symbols, n_levels);

    fc_bench_config_t config = FC_BENCH_CONFIG_DEFAULT;
    config.name = name;
    config.data_size = n_symbols * n_levels * sizeof(double) * 4 + n_symbols * n_features * sizeof(double);
    config.min_iterations = 100;

    fc_bench_result_t result;
    fc_bench_run(&config, bench_feature_extract_fn, &data, &result);
    fc_bench_result_print(&result);

    fc_aligned_free(data.features);
    fc_aligned_free(data.bid_p);
    fc_aligned_free(data.bid_q);
    fc_aligned_free(data.ask_p);
    fc_aligned_free(data.ask_q);
}

static void bench_feature_core_impl(size_t n_symbols, int n_levels, const char* name) {
    bench_feature_data_t data;
    data.n_symbols = n_symbols;
    data.n_levels = n_levels;
    data.features = fc_aligned_alloc(n_symbols * 9 * sizeof(double), 64);
    data.bid_p = fc_aligned_alloc(n_symbols * n_levels * sizeof(double), 64);
    data.bid_q = fc_aligned_alloc(n_symbols * n_levels * sizeof(double), 64);
    data.ask_p = fc_aligned_alloc(n_symbols * n_levels * sizeof(double), 64);
    data.ask_q = fc_aligned_alloc(n_symbols * n_levels * sizeof(double), 64);

    setup_test_data(data.bid_p, data.bid_q, data.ask_p, data.ask_q, n_symbols, n_levels);

    fc_bench_config_t config = FC_BENCH_CONFIG_DEFAULT;
    config.name = name;
    config.data_size = n_symbols * n_levels * sizeof(double) * 4 + n_symbols * 9 * sizeof(double);
    config.min_iterations = 100;

    fc_bench_result_t result;
    fc_bench_run(&config, bench_feature_extract_core_fn, &data, &result);
    fc_bench_result_print(&result);

    fc_aligned_free(data.features);
    fc_aligned_free(data.bid_p);
    fc_aligned_free(data.bid_q);
    fc_aligned_free(data.ask_p);
    fc_aligned_free(data.ask_q);
}

static void bench_feature_by_levels(size_t n_symbols) {
    int levels[] = {5, 10, 20};
    for (size_t i = 0; i < sizeof(levels) / sizeof(levels[0]); i++) {
        char name[128];
        snprintf(name, sizeof(name), "Feature/Full/N=%zu/L=%d", n_symbols, levels[i]);
        bench_feature_impl(n_symbols, levels[i], name);
    }
}

void bench_feature_run(void) {
    printf("\nFeature Extraction Benchmarks\n");
    printf("------------------------------------------------------------\n");

    printf("\nFull Feature Extraction (varying levels):\n");
    bench_feature_by_levels(1000);

    printf("\nFull Feature Extraction (5 levels, varying symbols):\n");
    bench_feature_impl(SMALL_BATCH, 5, "Feature/Full/N=64/L=5");
    bench_feature_impl(MEDIUM_BATCH, 5, "Feature/Full/N=1024/L=5");
    bench_feature_impl(LARGE_BATCH, 5, "Feature/Full/N=10000/L=5");

    printf("\nCore Feature Extraction Only (9 features):\n");
    bench_feature_core_impl(SMALL_BATCH, 5, "Feature/Core/N=64/L=5");
    bench_feature_core_impl(MEDIUM_BATCH, 5, "Feature/Core/N=1024/L=5");
    bench_feature_core_impl(LARGE_BATCH, 5, "Feature/Core/N=10000/L=5");

    printf("\nPerformance Targets:\n");
    printf("  Feature extraction:              ~200 ns per symbol (AVX-512)\n");
    printf("  Signal layer total budget:       ~0.5-2 μs\n");
    printf("  Features per symbol (5 levels):  49 features\n");
    printf("  Features per symbol (10 levels): 89 features\n");
}
