/**
 * @file bench_normalize.c
 * @brief Performance benchmarks for online normalization using Welford Z-Score
 */

#include "bench_framework.h"
#include "mem_aligned.h"
#include "platform.h"
#include "signal/normalize.h"
#include "simd_detect.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    double* z_out;
    fc_welford_state_t* states;
    double* features;
    size_t n_symbols;
    int n_features;
} bench_normalize_data_t;

static void bench_normalize_fn(void* user_data) {
    bench_normalize_data_t* data = (bench_normalize_data_t*) user_data;
    fc_ex_sig_normalize_zscore(
        data->z_out, data->states, data->features, data->n_symbols, data->n_features
    );
}

static void bench_normalize_impl(size_t n_symbols, int n_features, const char* name) {
    /* Allocate aligned memory */
    fc_welford_state_t* states = fc_aligned_alloc(n_features * sizeof(fc_welford_state_t), 64);
    double* features           = fc_aligned_alloc(n_symbols * n_features * sizeof(double), 64);
    double* z_out              = fc_aligned_alloc(n_symbols * n_features * sizeof(double), 64);

    if (!states || !features || !z_out) {
        fprintf(stderr, "Memory allocation failed\n");
        return;
    }

    /* Initialize Welford states with realistic statistics */
    for (int f = 0; f < n_features; f++) {
        states[f].count = 1000;
        states[f].mean  = 100.0 + f * 0.5;
        states[f].m2    = 999.0 * (10.0 + f * 0.1);
    }

    /* Initialize features with realistic financial data patterns */
    for (size_t s = 0; s < n_symbols; s++) {
        for (int f = 0; f < n_features; f++) {
            double base                  = states[f].mean;
            double variation             = ((s * 7 + f * 13) % 100) / 10.0 - 5.0;
            features[s * n_features + f] = base + variation;
        }
    }

    bench_normalize_data_t data = {
        .z_out      = z_out,
        .states     = states,
        .features   = features,
        .n_symbols  = n_symbols,
        .n_features = n_features
    };

    fc_bench_config_t config = FC_BENCH_CONFIG_DEFAULT;
    config.name              = name;
    config.data_size         = (n_symbols * n_features * 2 + n_features * 3) * sizeof(double);
    config.min_iterations    = 1000;

    fc_bench_result_t result;
    fc_bench_run(&config, bench_normalize_fn, &data, &result);
    fc_bench_result_print(&result);

    fc_aligned_free(states);
    fc_aligned_free(features);
    fc_aligned_free(z_out);
}

static void bench_normalize_ml_features(void) {
    printf("\nML Feature Normalization Scenarios\n");
    printf("------------------------------------------------------------\n");

    bench_normalize_impl(10, 20, "Normalize/ML/Small/10x20");
    bench_normalize_impl(100, 50, "Normalize/ML/Medium/100x50");
    bench_normalize_impl(1000, 50, "Normalize/ML/Large/1000x50");
    bench_normalize_impl(1000, 200, "Normalize/ML/VeryLarge/1000x200");
    bench_normalize_impl(5000, 100, "Normalize/ML/UltraLarge/5000x100");
}

static void bench_normalize_vary_features(void) {
    printf("\nVarying Number of Features (1000 symbols)\n");
    printf("------------------------------------------------------------\n");

    bench_normalize_impl(1000, 10, "Normalize/Features/N=10");
    bench_normalize_impl(1000, 20, "Normalize/Features/N=20");
    bench_normalize_impl(1000, 50, "Normalize/Features/N=50");
    bench_normalize_impl(1000, 100, "Normalize/Features/N=100");
    bench_normalize_impl(1000, 200, "Normalize/Features/N=200");
}

static void bench_normalize_vary_symbols(void) {
    printf("\nVarying Number of Symbols (50 features)\n");
    printf("------------------------------------------------------------\n");

    bench_normalize_impl(10, 50, "Normalize/Symbols/N=10");
    bench_normalize_impl(100, 50, "Normalize/Symbols/N=100");
    bench_normalize_impl(500, 50, "Normalize/Symbols/N=500");
    bench_normalize_impl(1000, 50, "Normalize/Symbols/N=1000");
    bench_normalize_impl(5000, 50, "Normalize/Symbols/N=5000");
    bench_normalize_impl(10000, 50, "Normalize/Symbols/N=10000");
}

typedef struct {
    fc_welford_state_t* states;
    double* features;
    size_t n_symbols;
    int n_features;
} bench_update_data_t;

static void bench_update_fn(void* user_data) {
    bench_update_data_t* data = (bench_update_data_t*) user_data;
    fc_ex_sig_normalize_update_states(
        data->states, data->features, data->n_symbols, data->n_features
    );
}

static void bench_update_states_impl(size_t n_symbols, int n_features, const char* name) {
    fc_welford_state_t* states = fc_aligned_alloc(n_features * sizeof(fc_welford_state_t), 64);
    double* features           = fc_aligned_alloc(n_symbols * n_features * sizeof(double), 64);

    if (!states || !features) {
        fprintf(stderr, "Memory allocation failed\n");
        return;
    }

    memset(states, 0, n_features * sizeof(fc_welford_state_t));

    for (size_t s = 0; s < n_symbols; s++) {
        for (int f = 0; f < n_features; f++) {
            features[s * n_features + f] = 100.0 + ((s * 7 + f * 13) % 100) / 10.0;
        }
    }

    bench_update_data_t data = {
        .states = states, .features = features, .n_symbols = n_symbols, .n_features = n_features
    };

    fc_bench_config_t config = FC_BENCH_CONFIG_DEFAULT;
    config.name              = name;
    config.data_size         = (n_symbols * n_features + n_features * 3) * sizeof(double);
    config.min_iterations    = 1000;

    fc_bench_result_t result;
    fc_bench_run(&config, bench_update_fn, &data, &result);
    fc_bench_result_print(&result);

    fc_aligned_free(states);
    fc_aligned_free(features);
}

static void bench_normalize_state_update(void) {
    printf("\nState Update Performance (1000 symbols)\n");
    printf("------------------------------------------------------------\n");

    bench_update_states_impl(1000, 10, "Update/Features/N=10");
    bench_update_states_impl(1000, 50, "Update/Features/N=50");
    bench_update_states_impl(1000, 100, "Update/Features/N=100");
    bench_update_states_impl(1000, 200, "Update/Features/N=200");
}

static void bench_normalize_single_feature(void) {
    printf("\nSingle Feature Normalization (varying symbols)\n");
    printf("------------------------------------------------------------\n");

    bench_normalize_impl(100, 1, "Normalize/SingleFeature/N=100");
    bench_normalize_impl(1000, 1, "Normalize/SingleFeature/N=1000");
    bench_normalize_impl(5000, 1, "Normalize/SingleFeature/N=5000");
    bench_normalize_impl(10000, 1, "Normalize/SingleFeature/N=10000");
}

static void bench_normalize_throughput(void) {
    printf("\nThroughput Measurement (element processing rate)\n");
    printf("------------------------------------------------------------\n");

    const size_t n_symbols      = 10000;
    const int n_features        = 100;
    const size_t total_elements = n_symbols * n_features;

    fc_welford_state_t* states = fc_aligned_alloc(n_features * sizeof(fc_welford_state_t), 64);
    double* features           = fc_aligned_alloc(total_elements * sizeof(double), 64);
    double* z_out              = fc_aligned_alloc(total_elements * sizeof(double), 64);

    if (!states || !features || !z_out) {
        fprintf(stderr, "Memory allocation failed\n");
        return;
    }

    for (int f = 0; f < n_features; f++) {
        states[f].count = 1000;
        states[f].mean  = 100.0;
        states[f].m2    = 999.0 * 10.0;
    }

    for (size_t i = 0; i < total_elements; i++) {
        features[i] = 100.0 + (i % 100) / 10.0;
    }

    bench_normalize_data_t data = {
        .z_out      = z_out,
        .states     = states,
        .features   = features,
        .n_symbols  = n_symbols,
        .n_features = n_features
    };

    fc_bench_config_t config = FC_BENCH_CONFIG_DEFAULT;
    config.name              = "Normalize/Throughput/10000x100";
    config.data_size         = total_elements * 2 * sizeof(double);
    config.min_iterations    = 100;

    fc_bench_result_t result;
    fc_bench_run(&config, bench_normalize_fn, &data, &result);
    fc_bench_result_print(&result);

    double elements_per_sec = (double) total_elements / (result.mean_ns / 1e9);
    printf("  Throughput: %.2f million elements/second\n", elements_per_sec / 1e6);

    fc_aligned_free(states);
    fc_aligned_free(features);
    fc_aligned_free(z_out);
}

void bench_normalize_run(void) {
    printf("\nOnline Normalization (Welford Z-Score) Benchmarks\n");
    printf("============================================================\n");

    fc_simd_level_t level = fc_get_simd_level();
    printf("SIMD Level: %s\n", fc_simd_level_string(level));
    printf("\n");

    bench_normalize_ml_features();
    bench_normalize_vary_features();
    bench_normalize_vary_symbols();
    bench_normalize_state_update();
    bench_normalize_single_feature();
    bench_normalize_throughput();

    printf("\nPerformance Targets:\n");
    printf("  Online normalization (batch): ~100-200 ns\n");
    printf("  Feature extraction:           ~200 ns (50-200 dims, AVX-512)\n");
    printf("  Signal layer total budget:    ~0.5-2 μs\n");
    printf("\n");
    printf("Use Cases:\n");
    printf("  - ML feature normalization before inference\n");
    printf("  - Real-time z-score computation for signal processing\n");
    printf("  - Rolling window statistics for 1000-tick windows\n");
    printf("  - No look-ahead bias (strictly causal for backtesting)\n");
}
