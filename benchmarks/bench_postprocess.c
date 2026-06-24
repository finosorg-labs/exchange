/**
 * @file bench_postprocess.c
 * @brief Performance benchmarks for signal post-processing
 */

#include "signal/postprocess.h"
#include "bench_framework.h"
#include "platform.h"
#include "simd_detect.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

typedef struct {
    double* sig_out;
    double* ema_state;
    double* sig_in;
    fc_ex_sig_postproc_cfg_t* cfg;
    size_t n;
} bench_postprocess_data_t;

typedef struct {
    double* reversal_out;
    double* sig_prev;
    double* sig_cur;
    size_t n;
} bench_reversal_data_t;

static void bench_postprocess_fn(void* user_data) {
    bench_postprocess_data_t* data = (bench_postprocess_data_t*)user_data;
    fc_ex_sig_postprocess(data->sig_out, data->ema_state, data->sig_in, data->cfg, data->n);
}

static void bench_reversal_fn(void* user_data) {
    bench_reversal_data_t* data = (bench_reversal_data_t*)user_data;
    fc_ex_sig_detect_reversal(data->reversal_out, data->sig_prev, data->sig_cur, data->n);
}

static void bench_postprocess_impl(size_t n, const char* name) {
    double* sig_in = aligned_alloc(64, n * sizeof(double));
    double* sig_out = aligned_alloc(64, n * sizeof(double));
    double* ema_state = aligned_alloc(64, n * sizeof(double));

    if (!sig_in || !sig_out || !ema_state) {
        fprintf(stderr, "Memory allocation failed\n");
        return;
    }

    /* Initialize with realistic signal data */
    for (size_t i = 0; i < n; i++) {
        sig_in[i] = sin(i * 0.01) * 2.0 + (i % 10 - 5) * 0.1;
        ema_state[i] = 0.0;
    }

    fc_ex_sig_postproc_cfg_t cfg = {
        .threshold = 0.1,
        .ema_alpha = 0.3,
        .clip_lo = -2.0,
        .clip_hi = 2.0,
        .enable_reversal = 0
    };

    bench_postprocess_data_t data = {
        .sig_out = sig_out,
        .ema_state = ema_state,
        .sig_in = sig_in,
        .cfg = &cfg,
        .n = n
    };

    fc_bench_config_t config = FC_BENCH_CONFIG_DEFAULT;
    config.name = name;
    config.data_size = n * sizeof(double);
    config.min_iterations = 100;
    config.min_time_ms = 100.0;

    fc_bench_result_t result;
    fc_bench_run(&config, bench_postprocess_fn, &data, &result);
    fc_bench_result_print(&result);

    free(sig_in);
    free(sig_out);
    free(ema_state);
}

static void bench_reversal_impl(size_t n, const char* name) {
    double* sig_prev = aligned_alloc(64, n * sizeof(double));
    double* sig_cur = aligned_alloc(64, n * sizeof(double));
    double* reversal_out = aligned_alloc(64, n * sizeof(double));

    if (!sig_prev || !sig_cur || !reversal_out) {
        fprintf(stderr, "Memory allocation failed\n");
        return;
    }

    /* Initialize with signal transitions */
    for (size_t i = 0; i < n; i++) {
        sig_prev[i] = sin(i * 0.02) * 2.0;
        sig_cur[i] = sin((i + 1) * 0.02) * 2.0;
    }

    bench_reversal_data_t data = {
        .reversal_out = reversal_out,
        .sig_prev = sig_prev,
        .sig_cur = sig_cur,
        .n = n
    };

    fc_bench_config_t config = FC_BENCH_CONFIG_DEFAULT;
    config.name = name;
    config.data_size = n * sizeof(double);
    config.min_iterations = 100;
    config.min_time_ms = 100.0;

    fc_bench_result_t result;
    fc_bench_run(&config, bench_reversal_fn, &data, &result);
    fc_bench_result_print(&result);

    free(sig_prev);
    free(sig_cur);
    free(reversal_out);
}

void bench_postprocess_run(void) {
    printf("========================================\n");
    printf("Signal Post-processing Benchmarks\n");
    printf("========================================\n\n");

    /* Print system info */
    fc_simd_level_t simd_level = fc_get_simd_level();
    const char* simd_str = "Unknown";
    switch (simd_level) {
        case FC_SIMD_SCALAR: simd_str = "Scalar"; break;
        case FC_SIMD_SSE42:  simd_str = "SSE4.2"; break;
        case FC_SIMD_AVX2:   simd_str = "AVX2"; break;
        case FC_SIMD_AVX512: simd_str = "AVX-512"; break;
        case FC_SIMD_NEON:   simd_str = "NEON"; break;
    }
    printf("SIMD Level: %s\n\n", simd_str);

    /* Post-processing benchmarks */
    printf("Post-processing (threshold + EMA + clip):\n");
    printf("------------------------------------------\n");
    bench_postprocess_impl(1000, "postprocess_1K");
    bench_postprocess_impl(10000, "postprocess_10K");
    bench_postprocess_impl(100000, "postprocess_100K");
    bench_postprocess_impl(1000000, "postprocess_1M");

    printf("\n");

    /* Reversal detection benchmarks */
    printf("Reversal Detection:\n");
    printf("------------------------------------------\n");
    bench_reversal_impl(1000, "reversal_1K");
    bench_reversal_impl(10000, "reversal_10K");
    bench_reversal_impl(100000, "reversal_100K");
    bench_reversal_impl(1000000, "reversal_1M");

    printf("\n");

    /* Component benchmarks */
    printf("Component Analysis (1M elements):\n");
    printf("------------------------------------------\n");

    const size_t n = 1000000;
    double* sig_in = aligned_alloc(64, n * sizeof(double));
    double* sig_out = aligned_alloc(64, n * sizeof(double));
    double* ema_state = aligned_alloc(64, n * sizeof(double));

    for (size_t i = 0; i < n; i++) {
        sig_in[i] = sin(i * 0.01) * 2.0;
        ema_state[i] = 0.0;
    }

    /* Benchmark with different configurations */

    /* Threshold only */
    fc_ex_sig_postproc_cfg_t cfg_threshold = {
        .threshold = 0.1,
        .ema_alpha = 1.0,     /* No EMA */
        .clip_lo = -INFINITY,
        .clip_hi = INFINITY,
        .enable_reversal = 0
    };
    bench_postprocess_data_t data_threshold = {sig_out, ema_state, sig_in, &cfg_threshold, n};

    fc_bench_config_t config_threshold = FC_BENCH_CONFIG_DEFAULT;
    config_threshold.name = "threshold_only";
    config_threshold.data_size = n * sizeof(double);
    config_threshold.min_iterations = 100;
    config_threshold.min_time_ms = 100.0;

    fc_bench_result_t result_threshold;
    fc_bench_run(&config_threshold, bench_postprocess_fn, &data_threshold, &result_threshold);
    fc_bench_result_print(&result_threshold);

    /* Reset state */
    memset(ema_state, 0, n * sizeof(double));

    /* EMA only */
    fc_ex_sig_postproc_cfg_t cfg_ema = {
        .threshold = 0.0,
        .ema_alpha = 0.3,
        .clip_lo = -INFINITY,
        .clip_hi = INFINITY,
        .enable_reversal = 0
    };
    bench_postprocess_data_t data_ema = {sig_out, ema_state, sig_in, &cfg_ema, n};

    fc_bench_config_t config_ema = FC_BENCH_CONFIG_DEFAULT;
    config_ema.name = "ema_only";
    config_ema.data_size = n * sizeof(double);
    config_ema.min_iterations = 100;
    config_ema.min_time_ms = 100.0;

    fc_bench_result_t result_ema;
    fc_bench_run(&config_ema, bench_postprocess_fn, &data_ema, &result_ema);
    fc_bench_result_print(&result_ema);

    /* Reset state */
    memset(ema_state, 0, n * sizeof(double));

    /* Clip only */
    fc_ex_sig_postproc_cfg_t cfg_clip = {
        .threshold = 0.0,
        .ema_alpha = 1.0,
        .clip_lo = -2.0,
        .clip_hi = 2.0,
        .enable_reversal = 0
    };
    bench_postprocess_data_t data_clip = {sig_out, ema_state, sig_in, &cfg_clip, n};

    fc_bench_config_t config_clip = FC_BENCH_CONFIG_DEFAULT;
    config_clip.name = "clip_only";
    config_clip.data_size = n * sizeof(double);
    config_clip.min_iterations = 100;
    config_clip.min_time_ms = 100.0;

    fc_bench_result_t result_clip;
    fc_bench_run(&config_clip, bench_postprocess_fn, &data_clip, &result_clip);
    fc_bench_result_print(&result_clip);

    free(sig_in);
    free(sig_out);
    free(ema_state);

    printf("\n========================================\n");
    printf("Benchmark completed\n");
    printf("========================================\n");
}
