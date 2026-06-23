/**
 * @file bench_kyle_lambda.c
 * @brief Benchmark for Kyle's Lambda computation
 */

#include "signal/kyle_lambda.h"
#include "bench_framework.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

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

static void bench_config(size_t n_symbols, size_t window, const char* desc) {
    const size_t total_elements = n_symbols * window;
    const size_t iterations = 1000;

    double* dprice = (double*)malloc(total_elements * sizeof(double));
    double* volume = (double*)malloc(total_elements * sizeof(double));
    double* lambda = (double*)malloc(n_symbols * sizeof(double));

    if (!dprice || !volume || !lambda) {
        printf("  FAILED: Memory allocation\n");
        goto cleanup;
    }

    generate_test_data(dprice, volume, n_symbols, window);

    /* Warmup */
    for (int i = 0; i < 10; i++) {
        fc_ex_sig_kyle_lambda_batch(lambda, dprice, volume, n_symbols, window);
    }

    /* Benchmark */
    fc_bench_time_t start = fc_bench_time_now();
    for (size_t i = 0; i < iterations; i++) {
        fc_ex_sig_kyle_lambda_batch(lambda, dprice, volume, n_symbols, window);
    }
    fc_bench_time_t end = fc_bench_time_now();

    double elapsed_ms = fc_bench_time_elapsed_ms(&start, &end);
    double per_iter_us = (elapsed_ms * 1000.0) / iterations;
    double per_symbol_ns = (elapsed_ms * 1e6) / (iterations * n_symbols);

    printf("  %s\n", desc);
    printf("    Time: %.3f μs/iter, %.2f ns/symbol\n", per_iter_us, per_symbol_ns);

cleanup:
    free(dprice);
    free(volume);
    free(lambda);
}

void bench_kyle_lambda_run(void) {
    printf("\nKyle's Lambda Benchmarks\n");
    printf("------------------------------------------------------------\n");

    bench_config(1, 100, "1 symbol × 100-tick window");
    bench_config(1, 1000, "1 symbol × 1000-tick window");
    bench_config(10, 100, "10 symbols × 100-tick window");
    bench_config(100, 100, "100 symbols × 100-tick window");
    bench_config(100, 1000, "100 symbols × 1000-tick window");
    bench_config(1000, 100, "1000 symbols × 100-tick window");
}
