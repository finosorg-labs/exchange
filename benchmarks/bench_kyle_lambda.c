/**
 * @file bench_kyle_lambda.c
 * @brief Benchmark for Kyle's Lambda computation
 */

#include "signal/kyle_lambda.h"
#include "bench_framework.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

typedef struct {
    double* lambda_out;
    double* dprice;
    double* volume;
    size_t n_symbols;
    size_t window;
} bench_kyle_lambda_data_t;

typedef struct {
    double* lambda_out;
    bool* valid_flags;
    double* dprice;
    double* volume;
    double* workspace;
    size_t workspace_size;
    size_t n_symbols;
    size_t window;
} bench_kyle_lambda_ext_data_t;

typedef struct {
    double* lambda_out;
    double* r_squared;
    double* std_error;
    double* dprice;
    double* volume;
    size_t n_symbols;
    size_t window;
} bench_kyle_lambda_ols_data_t;

/* Test data generation */
static void generate_test_data(
    double* dprice,
    double* volume,
    size_t n_symbols,
    size_t window
) {
    for (size_t i = 0; i < n_symbols; i++) {
        for (size_t j = 0; j < window; j++) {
            size_t idx = i * window + j;
            double t = (double)j / (double)window;
            dprice[idx] = sin(t * 6.28318 + i * 0.1) * 0.01;
            volume[idx] = 1000.0 + 500.0 * cos(t * 3.14159 + i * 0.2) + 100.0 * dprice[idx];
        }
    }
}

static void bench_kyle_lambda_fn(void* user_data) {
    bench_kyle_lambda_data_t* data = (bench_kyle_lambda_data_t*)user_data;
    fc_ex_sig_kyle_lambda_batch(
        data->lambda_out,
        data->dprice,
        data->volume,
        data->n_symbols,
        data->window
    );
}

static void bench_kyle_lambda_ext_fn(void* user_data) {
    bench_kyle_lambda_ext_data_t* data = (bench_kyle_lambda_ext_data_t*)user_data;
    fc_ex_sig_kyle_lambda_batch_ext(
        data->lambda_out,
        data->valid_flags,
        data->dprice,
        data->volume,
        data->n_symbols,
        data->window,
        data->workspace,
        data->workspace_size
    );
}

static void bench_kyle_lambda_ols_fn(void* user_data) {
    bench_kyle_lambda_ols_data_t* data = (bench_kyle_lambda_ols_data_t*)user_data;
    fc_ex_sig_kyle_lambda_ols(
        data->lambda_out,
        data->r_squared,
        data->std_error,
        NULL,
        data->dprice,
        data->volume,
        data->n_symbols,
        data->window
    );
}

static void bench_kyle_lambda_batch_impl(size_t n_symbols, size_t window, const char* name) {
    const size_t total_elements = n_symbols * window;

    double* dprice = aligned_alloc(64, total_elements * sizeof(double));
    double* volume = aligned_alloc(64, total_elements * sizeof(double));
    double* lambda = aligned_alloc(64, n_symbols * sizeof(double));

    if (!dprice || !volume || !lambda) {
        fprintf(stderr, "Memory allocation failed\n");
        return;
    }

    generate_test_data(dprice, volume, n_symbols, window);

    bench_kyle_lambda_data_t data = {
        .lambda_out = lambda,
        .dprice = dprice,
        .volume = volume,
        .n_symbols = n_symbols,
        .window = window
    };

    fc_bench_config_t config = FC_BENCH_CONFIG_DEFAULT;
    config.name = name;
    config.data_size = total_elements * sizeof(double) * 2;
    config.min_iterations = 100;
    config.min_time_ms = 100.0;

    fc_bench_result_t result;
    fc_bench_run(&config, bench_kyle_lambda_fn, &data, &result);
    fc_bench_result_print(&result);

    free(dprice);
    free(volume);
    free(lambda);
}

static void bench_kyle_lambda_ext_impl(size_t n_symbols, size_t window, const char* name) {
    const size_t total_elements = n_symbols * window;

    double* dprice = aligned_alloc(64, total_elements * sizeof(double));
    double* volume = aligned_alloc(64, total_elements * sizeof(double));
    double* lambda = aligned_alloc(64, n_symbols * sizeof(double));
    bool* valid = aligned_alloc(64, n_symbols * sizeof(bool));

    size_t ws_size = fc_ex_sig_kyle_lambda_workspace_size(window);
    double* workspace = aligned_alloc(64, ws_size);

    if (!dprice || !volume || !lambda || !valid || !workspace) {
        fprintf(stderr, "Memory allocation failed\n");
        return;
    }

    generate_test_data(dprice, volume, n_symbols, window);

    bench_kyle_lambda_ext_data_t data = {
        .lambda_out = lambda,
        .valid_flags = valid,
        .dprice = dprice,
        .volume = volume,
        .workspace = workspace,
        .workspace_size = ws_size,
        .n_symbols = n_symbols,
        .window = window
    };

    fc_bench_config_t config = FC_BENCH_CONFIG_DEFAULT;
    config.name = name;
    config.data_size = total_elements * sizeof(double) * 2;
    config.min_iterations = 100;
    config.min_time_ms = 100.0;

    fc_bench_result_t result;
    fc_bench_run(&config, bench_kyle_lambda_ext_fn, &data, &result);
    fc_bench_result_print(&result);

    free(dprice);
    free(volume);
    free(lambda);
    free(valid);
    free(workspace);
}

static void bench_kyle_lambda_ols_impl(size_t n_symbols, size_t window, const char* name) {
    const size_t total_elements = n_symbols * window;

    double* dprice = aligned_alloc(64, total_elements * sizeof(double));
    double* volume = aligned_alloc(64, total_elements * sizeof(double));
    double* lambda = aligned_alloc(64, n_symbols * sizeof(double));
    double* r_squared = aligned_alloc(64, n_symbols * sizeof(double));
    double* std_error = aligned_alloc(64, n_symbols * sizeof(double));

    if (!dprice || !volume || !lambda || !r_squared || !std_error) {
        fprintf(stderr, "Memory allocation failed\n");
        return;
    }

    generate_test_data(dprice, volume, n_symbols, window);

    bench_kyle_lambda_ols_data_t data = {
        .lambda_out = lambda,
        .r_squared = r_squared,
        .std_error = std_error,
        .dprice = dprice,
        .volume = volume,
        .n_symbols = n_symbols,
        .window = window
    };

    fc_bench_config_t config = FC_BENCH_CONFIG_DEFAULT;
    config.name = name;
    config.data_size = total_elements * sizeof(double) * 2;
    config.min_iterations = 50;  /* Fewer iterations for slower OLS */
    config.min_time_ms = 100.0;

    fc_bench_result_t result;
    fc_bench_run(&config, bench_kyle_lambda_ols_fn, &data, &result);
    fc_bench_result_print(&result);

    free(dprice);
    free(volume);
    free(lambda);
    free(r_squared);
    free(std_error);
}

void bench_kyle_lambda_run(void) {
    printf("\nKyle's Lambda Benchmarks\n");
    printf("------------------------------------------------------------\n");

    printf("\nCovariance Method (Original API):\n");
    bench_kyle_lambda_batch_impl(1, 100, "KyleLambda/Cov/Symbols=1/Window=100");
    bench_kyle_lambda_batch_impl(10, 100, "KyleLambda/Cov/Symbols=10/Window=100");
    bench_kyle_lambda_batch_impl(100, 100, "KyleLambda/Cov/Symbols=100/Window=100");
    bench_kyle_lambda_batch_impl(1000, 100, "KyleLambda/Cov/Symbols=1000/Window=100");
    bench_kyle_lambda_batch_impl(100, 1000, "KyleLambda/Cov/Symbols=100/Window=1000");

    printf("\nCovariance Method (Extended API with Zero-Allocation):\n");
    bench_kyle_lambda_ext_impl(1, 100, "KyleLambda/CovExt/Symbols=1/Window=100");
    bench_kyle_lambda_ext_impl(10, 100, "KyleLambda/CovExt/Symbols=10/Window=100");
    bench_kyle_lambda_ext_impl(100, 100, "KyleLambda/CovExt/Symbols=100/Window=100");
    bench_kyle_lambda_ext_impl(1000, 100, "KyleLambda/CovExt/Symbols=1000/Window=100");
    bench_kyle_lambda_ext_impl(100, 1000, "KyleLambda/CovExt/Symbols=100/Window=1000");

    printf("\nOLS Regression Method (Full Statistics):\n");
    bench_kyle_lambda_ols_impl(1, 100, "KyleLambda/OLS/Symbols=1/Window=100");
    bench_kyle_lambda_ols_impl(10, 100, "KyleLambda/OLS/Symbols=10/Window=100");
    bench_kyle_lambda_ols_impl(100, 100, "KyleLambda/OLS/Symbols=100/Window=100");
    bench_kyle_lambda_ols_impl(100, 1000, "KyleLambda/OLS/Symbols=100/Window=1000");

    printf("\nPerformance Targets:\n");
    printf("  Covariance method:               ~1-5 μs per symbol\n");
    printf("  Extended API (zero-alloc):       ~0.5-3 μs per symbol\n");
    printf("  OLS (with diagnostics):          ~10-50 μs per symbol\n");
    printf("  Signal layer total budget:       ~0.5-2 μs\n");
}
