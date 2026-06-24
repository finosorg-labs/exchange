/**
 * @file bench_alpha.c
 * @brief Performance benchmarks for Alpha factor aggregation
 */

#include "signal/alpha.h"
#include "bench_framework.h"
#include "platform.h"
#include "simd_detect.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* Helper macro to align size to 64-byte boundary */
#define ALIGN_SIZE(size) (((size) + 63) / 64 * 64)

typedef struct {
    double* alpha_out;
    double* confidence_out;
    const double* signals;
    const double* weights;
    fc_ex_alpha_cfg_t* cfg;
    size_t n_symbols;
    int n_signals;
} bench_alpha_data_t;

typedef struct {
    double* weights_out;
    const double* weights_in;
    size_t n_symbols;
    int n_signals;
} bench_normalize_data_t;

typedef struct {
    double* agreement_out;
    const double* signals;
    size_t n_symbols;
    int n_signals;
} bench_agreement_data_t;

typedef struct {
    double* weights_out;
    const double* signals_hist;
    size_t window_size;
    int n_signals;
} bench_invvol_data_t;

static void bench_alpha_aggregate_fn(void* user_data) {
    bench_alpha_data_t* data = (bench_alpha_data_t*)user_data;
    fc_ex_sig_alpha_aggregate(
        data->alpha_out,
        data->confidence_out,
        data->signals,
        data->weights,
        data->cfg,
        data->n_symbols,
        data->n_signals
    );
}

static void bench_normalize_weights_fn(void* user_data) {
    bench_normalize_data_t* data = (bench_normalize_data_t*)user_data;
    fc_ex_sig_normalize_weights(data->weights_out, data->weights_in, data->n_symbols, data->n_signals);
}

static void bench_compute_agreement_fn(void* user_data) {
    bench_agreement_data_t* data = (bench_agreement_data_t*)user_data;
    fc_ex_sig_compute_agreement(data->agreement_out, data->signals, data->n_symbols, data->n_signals);
}

static void bench_inverse_vol_weights_fn(void* user_data) {
    bench_invvol_data_t* data = (bench_invvol_data_t*)user_data;
    fc_ex_sig_inverse_vol_weights(data->weights_out, data->signals_hist, data->window_size, data->n_signals);
}

static void bench_alpha_aggregate_impl(size_t n_symbols, int n_signals, const char* name) {
    size_t signals_size = n_symbols * n_signals;

    double* signals = aligned_alloc(64, ALIGN_SIZE(signals_size * sizeof(double)));
    double* weights = aligned_alloc(64, ALIGN_SIZE(n_signals * sizeof(double)));
    double* alpha_out = aligned_alloc(64, ALIGN_SIZE(n_symbols * sizeof(double)));
    double* confidence_out = aligned_alloc(64, ALIGN_SIZE(n_symbols * sizeof(double)));

    if (!signals || !weights || !alpha_out || !confidence_out) {
        fprintf(stderr, "Memory allocation failed\n");
        return;
    }

    /* Initialize with realistic signal patterns */
    for (size_t i = 0; i < n_symbols; i++) {
        for (int j = 0; j < n_signals; j++) {
            /* Mix of positive, negative, and varying magnitudes */
            double phase = (double)(i + j) * 0.1;
            signals[i * n_signals + j] = sin(phase) * 2.0 + cos(phase * 0.5);
        }
    }

    /* Initialize weights */
    for (int j = 0; j < n_signals; j++) {
        weights[j] = 1.0 / (double)n_signals;
    }

    fc_ex_alpha_cfg_t cfg = {
        .normalize_weights = 0,
        .per_symbol_weights = 0,
        .min_confidence = 0.3
    };

    bench_alpha_data_t data = {
        .alpha_out = alpha_out,
        .confidence_out = confidence_out,
        .signals = signals,
        .weights = weights,
        .cfg = &cfg,
        .n_symbols = n_symbols,
        .n_signals = n_signals
    };

    fc_bench_config_t config = FC_BENCH_CONFIG_DEFAULT;
    config.name = name;
    config.data_size = signals_size * sizeof(double);
    config.min_iterations = 100;
    config.min_time_ms = 100.0;

    fc_bench_result_t result;
    fc_bench_run(&config, bench_alpha_aggregate_fn, &data, &result);
    fc_bench_result_print(&result);

    free(signals);
    free(weights);
    free(alpha_out);
    free(confidence_out);
}

static void bench_alpha_aggregate_per_symbol_weights(size_t n_symbols, int n_signals, const char* name) {
    size_t signals_size = n_symbols * n_signals;
    size_t weights_size = n_symbols * n_signals;

    double* signals = aligned_alloc(64, ALIGN_SIZE(signals_size * sizeof(double)));
    double* weights = aligned_alloc(64, ALIGN_SIZE(weights_size * sizeof(double)));
    double* alpha_out = aligned_alloc(64, ALIGN_SIZE(n_symbols * sizeof(double)));
    double* confidence_out = aligned_alloc(64, ALIGN_SIZE(n_symbols * sizeof(double)));

    if (!signals || !weights || !alpha_out || !confidence_out) {
        fprintf(stderr, "Memory allocation failed\n");
        return;
    }

    /* Initialize data */
    for (size_t i = 0; i < n_symbols; i++) {
        for (int j = 0; j < n_signals; j++) {
            double phase = (double)(i + j) * 0.1;
            signals[i * n_signals + j] = sin(phase) * 2.0;
            weights[i * n_signals + j] = 1.0 / (double)n_signals;
        }
    }

    fc_ex_alpha_cfg_t cfg = {
        .normalize_weights = 0,
        .per_symbol_weights = 1,  /* Per-symbol weights */
        .min_confidence = 0.3
    };

    bench_alpha_data_t data = {
        .alpha_out = alpha_out,
        .confidence_out = confidence_out,
        .signals = signals,
        .weights = weights,
        .cfg = &cfg,
        .n_symbols = n_symbols,
        .n_signals = n_signals
    };

    fc_bench_config_t config = FC_BENCH_CONFIG_DEFAULT;
    config.name = name;
    config.data_size = signals_size * sizeof(double);
    config.min_iterations = 100;
    config.min_time_ms = 100.0;

    fc_bench_result_t result;
    fc_bench_run(&config, bench_alpha_aggregate_fn, &data, &result);
    fc_bench_result_print(&result);

    free(signals);
    free(weights);
    free(alpha_out);
    free(confidence_out);
}

static void bench_normalize_weights_impl(size_t n_symbols, int n_signals, const char* name) {
    size_t weights_size = n_symbols * n_signals;

    double* weights_in = aligned_alloc(64, ALIGN_SIZE(weights_size * sizeof(double)));
    double* weights_out = aligned_alloc(64, ALIGN_SIZE(weights_size * sizeof(double)));

    if (!weights_in || !weights_out) {
        fprintf(stderr, "Memory allocation failed\n");
        return;
    }

    /* Initialize with non-normalized weights */
    for (size_t i = 0; i < weights_size; i++) {
        weights_in[i] = (double)(i % 10 + 1);
    }

    bench_normalize_data_t data = {
        .weights_out = weights_out,
        .weights_in = weights_in,
        .n_symbols = n_symbols,
        .n_signals = n_signals
    };

    fc_bench_config_t config = FC_BENCH_CONFIG_DEFAULT;
    config.name = name;
    config.data_size = weights_size * sizeof(double);
    config.min_iterations = 1000;
    config.min_time_ms = 100.0;

    fc_bench_result_t result;
    fc_bench_run(&config, bench_normalize_weights_fn, &data, &result);
    fc_bench_result_print(&result);

    free(weights_in);
    free(weights_out);
}

static void bench_compute_agreement_impl(size_t n_symbols, int n_signals, const char* name) {
    size_t signals_size = n_symbols * n_signals;

    double* signals = aligned_alloc(64, ALIGN_SIZE(signals_size * sizeof(double)));
    double* agreement_out = aligned_alloc(64, ALIGN_SIZE(n_symbols * sizeof(double)));

    if (!signals || !agreement_out) {
        fprintf(stderr, "Memory allocation failed\n");
        return;
    }

    /* Initialize signals */
    for (size_t i = 0; i < signals_size; i++) {
        signals[i] = sin((double)i * 0.1) * 2.0;
    }

    bench_agreement_data_t data = {
        .agreement_out = agreement_out,
        .signals = signals,
        .n_symbols = n_symbols,
        .n_signals = n_signals
    };

    fc_bench_config_t config = FC_BENCH_CONFIG_DEFAULT;
    config.name = name;
    config.data_size = signals_size * sizeof(double);
    config.min_iterations = 100;
    config.min_time_ms = 100.0;

    fc_bench_result_t result;
    fc_bench_run(&config, bench_compute_agreement_fn, &data, &result);
    fc_bench_result_print(&result);

    free(signals);
    free(agreement_out);
}

static void bench_inverse_vol_weights_impl(size_t window_size, int n_signals, const char* name) {
    size_t hist_size = window_size * n_signals;

    double* signals_hist = aligned_alloc(64, ALIGN_SIZE(hist_size * sizeof(double)));
    double* weights_out = aligned_alloc(64, ALIGN_SIZE(n_signals * sizeof(double)));

    if (!signals_hist || !weights_out) {
        fprintf(stderr, "Memory allocation failed\n");
        return;
    }

    /* Initialize historical signals with varying volatility */
    for (size_t t = 0; t < window_size; t++) {
        for (int j = 0; j < n_signals; j++) {
            /* Each signal has different volatility */
            double vol = 0.1 + (double)j * 0.05;
            signals_hist[t * n_signals + j] = sin((double)t * 0.1) * vol;
        }
    }

    bench_invvol_data_t data = {
        .weights_out = weights_out,
        .signals_hist = signals_hist,
        .window_size = window_size,
        .n_signals = n_signals
    };

    fc_bench_config_t config = FC_BENCH_CONFIG_DEFAULT;
    config.name = name;
    config.data_size = hist_size * sizeof(double);
    config.min_iterations = 100;
    config.min_time_ms = 100.0;

    fc_bench_result_t result;
    fc_bench_run(&config, bench_inverse_vol_weights_fn, &data, &result);
    fc_bench_result_print(&result);

    free(signals_hist);
    free(weights_out);
}

void bench_alpha_run(void) {
    printf("\nAlpha Factor Aggregation Benchmarks\n");
    printf("------------------------------------------------------------\n");

    /* Alpha aggregation with uniform weights - various sizes */
    bench_alpha_aggregate_impl(100, 10, "Alpha/Aggregate/Uniform/100×10");
    bench_alpha_aggregate_impl(1000, 10, "Alpha/Aggregate/Uniform/1000×10");
    bench_alpha_aggregate_impl(1000, 50, "Alpha/Aggregate/Uniform/1000×50");
    bench_alpha_aggregate_impl(5000, 20, "Alpha/Aggregate/Uniform/5000×20");
    bench_alpha_aggregate_impl(10000, 10, "Alpha/Aggregate/Uniform/10000×10");

    /* Alpha aggregation with per-symbol weights */
    bench_alpha_aggregate_per_symbol_weights(1000, 10, "Alpha/Aggregate/PerSymbol/1000×10");
    bench_alpha_aggregate_per_symbol_weights(5000, 20, "Alpha/Aggregate/PerSymbol/5000×20");

    /* Weight normalization */
    bench_normalize_weights_impl(1000, 10, "Alpha/Normalize/1000×10");
    bench_normalize_weights_impl(10000, 10, "Alpha/Normalize/10000×10");
    bench_normalize_weights_impl(5000, 50, "Alpha/Normalize/5000×50");

    /* Signal agreement computation */
    bench_compute_agreement_impl(1000, 10, "Alpha/Agreement/1000×10");
    bench_compute_agreement_impl(5000, 20, "Alpha/Agreement/5000×20");
    bench_compute_agreement_impl(10000, 50, "Alpha/Agreement/10000×50");

    /* Inverse volatility weighting */
    bench_inverse_vol_weights_impl(100, 10, "Alpha/InverseVol/100×10");
    bench_inverse_vol_weights_impl(1000, 10, "Alpha/InverseVol/1000×10");
    bench_inverse_vol_weights_impl(1000, 50, "Alpha/InverseVol/1000×50");

    /* Real-world scenarios */
    bench_alpha_aggregate_impl(3000, 15, "Alpha/RealWorld/3000×15");
    bench_alpha_aggregate_impl(5000, 20, "Alpha/RealWorld/5000×20");

    /* HFT scenarios */
    bench_alpha_aggregate_impl(1, 20, "Alpha/HFT/1×20");
    bench_alpha_aggregate_impl(10, 30, "Alpha/HFT/10×30");

    /* ML Alpha scenarios */
    bench_alpha_aggregate_impl(1000, 100, "Alpha/ML/1000×100");
    bench_alpha_aggregate_impl(1000, 200, "Alpha/ML/1000×200");

    printf("\nPerformance Targets:\n");
    printf("  Alpha aggregation:              ~0.5-2 μs per symbol\n");
    printf("  Weight normalization:           ~50-100 ns per weight\n");
    printf("  Signal agreement:               ~100-200 ns per symbol\n");
    printf("  Full signal layer budget:       ~0.5-2 μs total\n");
}
