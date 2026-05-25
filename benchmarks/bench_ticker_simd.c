/**
 * @file bench_ticker_simd.c
 * @brief Performance benchmarks for SIMD-optimized ticker batch processing
 */

#include "bench_framework.h"
#include "simd_detect.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define NSEC_PER_SEC 1000000000LL

// External function from ticker_simd.c
void fc_ticker_update_ohlcv_batch_simd(
    double* high,
    double* low,
    double* volume_sum,
    double* amount_sum,
    const double* prices,
    const double* volumes,
    const double* amounts,
    size_t count
);

static inline int64_t get_time_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (int64_t)ts.tv_sec * NSEC_PER_SEC + ts.tv_nsec;
}

static void generate_random_data(double* prices, double* volumes, double* amounts, size_t count) {
    for (size_t i = 0; i < count; i++) {
        prices[i]  = 100.0 + (rand() % 10000) / 100.0;
        volumes[i] = 100.0 + (rand() % 10000);
        amounts[i] = prices[i] * volumes[i];
    }
}

static void bench_simd_batch_small(void) {
    printf("\n=== Benchmark: SIMD Batch (Small - 10 elements) ===\n");

    const size_t count      = 10;
    const size_t iterations = 1000000;

    double* prices  = (double*)malloc(count * sizeof(double));
    double* volumes = (double*)malloc(count * sizeof(double));
    double* amounts = (double*)malloc(count * sizeof(double));

    generate_random_data(prices, volumes, amounts, count);

    double high       = 100.0;
    double low        = 100.0;
    double volume_sum = 0.0;
    double amount_sum = 0.0;

    int64_t start = get_time_ns();
    for (size_t i = 0; i < iterations; i++) {
        fc_ticker_update_ohlcv_batch_simd(
            &high, &low, &volume_sum, &amount_sum, prices, volumes, amounts, count
        );
    }
    int64_t end = get_time_ns();

    double elapsed_ms      = (end - start) / 1e6;
    double ops_per_sec     = iterations / (elapsed_ms / 1000.0);
    double ns_per_op       = (end - start) / (double)iterations;
    double elements_per_op = count;
    double ns_per_element  = ns_per_op / elements_per_op;

    printf("Iterations: %zu\n", iterations);
    printf("Elements per batch: %zu\n", count);
    printf("Total time: %.3f ms\n", elapsed_ms);
    printf("Throughput: %.0f ops/sec\n", ops_per_sec);
    printf("Latency per op: %.3f ns\n", ns_per_op);
    printf("Latency per element: %.3f ns\n", ns_per_element);

    free(prices);
    free(volumes);
    free(amounts);
}

static void bench_simd_batch_medium(void) {
    printf("\n=== Benchmark: SIMD Batch (Medium - 100 elements) ===\n");

    const size_t count      = 100;
    const size_t iterations = 100000;

    double* prices  = (double*)malloc(count * sizeof(double));
    double* volumes = (double*)malloc(count * sizeof(double));
    double* amounts = (double*)malloc(count * sizeof(double));

    generate_random_data(prices, volumes, amounts, count);

    double high       = 100.0;
    double low        = 100.0;
    double volume_sum = 0.0;
    double amount_sum = 0.0;

    int64_t start = get_time_ns();
    for (size_t i = 0; i < iterations; i++) {
        fc_ticker_update_ohlcv_batch_simd(
            &high, &low, &volume_sum, &amount_sum, prices, volumes, amounts, count
        );
    }
    int64_t end = get_time_ns();

    double elapsed_ms      = (end - start) / 1e6;
    double ops_per_sec     = iterations / (elapsed_ms / 1000.0);
    double ns_per_op       = (end - start) / (double)iterations;
    double elements_per_op = count;
    double ns_per_element  = ns_per_op / elements_per_op;

    printf("Iterations: %zu\n", iterations);
    printf("Elements per batch: %zu\n", count);
    printf("Total time: %.3f ms\n", elapsed_ms);
    printf("Throughput: %.0f ops/sec\n", ops_per_sec);
    printf("Latency per op: %.3f ns\n", ns_per_op);
    printf("Latency per element: %.3f ns\n", ns_per_element);

    free(prices);
    free(volumes);
    free(amounts);
}

static void bench_simd_batch_large(void) {
    printf("\n=== Benchmark: SIMD Batch (Large - 1000 elements) ===\n");

    const size_t count      = 1000;
    const size_t iterations = 10000;

    double* prices  = (double*)malloc(count * sizeof(double));
    double* volumes = (double*)malloc(count * sizeof(double));
    double* amounts = (double*)malloc(count * sizeof(double));

    generate_random_data(prices, volumes, amounts, count);

    double high       = 100.0;
    double low        = 100.0;
    double volume_sum = 0.0;
    double amount_sum = 0.0;

    int64_t start = get_time_ns();
    for (size_t i = 0; i < iterations; i++) {
        fc_ticker_update_ohlcv_batch_simd(
            &high, &low, &volume_sum, &amount_sum, prices, volumes, amounts, count
        );
    }
    int64_t end = get_time_ns();

    double elapsed_ms      = (end - start) / 1e6;
    double ops_per_sec     = iterations / (elapsed_ms / 1000.0);
    double ns_per_op       = (end - start) / (double)iterations;
    double elements_per_op = count;
    double ns_per_element  = ns_per_op / elements_per_op;

    printf("Iterations: %zu\n", iterations);
    printf("Elements per batch: %zu\n", count);
    printf("Total time: %.3f ms\n", elapsed_ms);
    printf("Throughput: %.0f ops/sec\n", ops_per_sec);
    printf("Latency per op: %.3f ns\n", ns_per_op);
    printf("Latency per element: %.3f ns\n", ns_per_element);

    free(prices);
    free(volumes);
    free(amounts);
}

static void bench_simd_batch_very_large(void) {
    printf("\n=== Benchmark: SIMD Batch (Very Large - 10000 elements) ===\n");

    const size_t count      = 10000;
    const size_t iterations = 1000;

    double* prices  = (double*)malloc(count * sizeof(double));
    double* volumes = (double*)malloc(count * sizeof(double));
    double* amounts = (double*)malloc(count * sizeof(double));

    generate_random_data(prices, volumes, amounts, count);

    double high       = 100.0;
    double low        = 100.0;
    double volume_sum = 0.0;
    double amount_sum = 0.0;

    int64_t start = get_time_ns();
    for (size_t i = 0; i < iterations; i++) {
        fc_ticker_update_ohlcv_batch_simd(
            &high, &low, &volume_sum, &amount_sum, prices, volumes, amounts, count
        );
    }
    int64_t end = get_time_ns();

    double elapsed_ms      = (end - start) / 1e6;
    double ops_per_sec     = iterations / (elapsed_ms / 1000.0);
    double ns_per_op       = (end - start) / (double)iterations;
    double elements_per_op = count;
    double ns_per_element  = ns_per_op / elements_per_op;

    printf("Iterations: %zu\n", iterations);
    printf("Elements per batch: %zu\n", count);
    printf("Total time: %.3f ms\n", elapsed_ms);
    printf("Throughput: %.0f ops/sec\n", ops_per_sec);
    printf("Latency per op: %.3f ns\n", ns_per_op);
    printf("Latency per element: %.3f ns\n", ns_per_element);

    free(prices);
    free(volumes);
    free(amounts);
}

static void bench_simd_batch_varying_sizes(void) {
    printf("\n=== Benchmark: SIMD Batch (Varying Sizes) ===\n");

    const size_t sizes[]      = {1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024};
    const size_t num_sizes    = sizeof(sizes) / sizeof(sizes[0]);
    const size_t iterations   = 100000;
    const size_t max_size     = 1024;

    double* prices  = (double*)malloc(max_size * sizeof(double));
    double* volumes = (double*)malloc(max_size * sizeof(double));
    double* amounts = (double*)malloc(max_size * sizeof(double));

    generate_random_data(prices, volumes, amounts, max_size);

    printf("\n%-10s %-15s %-15s %-15s\n", "Size", "Time (ms)", "ns/op", "ns/element");
    printf("---------------------------------------------------------------\n");

    for (size_t s = 0; s < num_sizes; s++) {
        size_t count = sizes[s];

        double high       = 100.0;
        double low        = 100.0;
        double volume_sum = 0.0;
        double amount_sum = 0.0;

        int64_t start = get_time_ns();
        for (size_t i = 0; i < iterations; i++) {
            fc_ticker_update_ohlcv_batch_simd(
                &high, &low, &volume_sum, &amount_sum, prices, volumes, amounts, count
            );
        }
        int64_t end = get_time_ns();

        double elapsed_ms     = (end - start) / 1e6;
        double ns_per_op      = (end - start) / (double)iterations;
        double ns_per_element = ns_per_op / count;

        printf("%-10zu %-15.3f %-15.3f %-15.3f\n", count, elapsed_ms, ns_per_op, ns_per_element);
    }

    free(prices);
    free(volumes);
    free(amounts);
}

static void bench_simd_throughput(void) {
    printf("\n=== Benchmark: SIMD Throughput (1 second test) ===\n");

    const size_t count = 100;
    const int64_t test_duration_ns = 1 * NSEC_PER_SEC;

    double* prices  = (double*)malloc(count * sizeof(double));
    double* volumes = (double*)malloc(count * sizeof(double));
    double* amounts = (double*)malloc(count * sizeof(double));

    generate_random_data(prices, volumes, amounts, count);

    double high       = 100.0;
    double low        = 100.0;
    double volume_sum = 0.0;
    double amount_sum = 0.0;

    size_t iterations = 0;
    int64_t start     = get_time_ns();
    int64_t end       = start;

    while ((end - start) < test_duration_ns) {
        fc_ticker_update_ohlcv_batch_simd(
            &high, &low, &volume_sum, &amount_sum, prices, volumes, amounts, count
        );
        iterations++;
        end = get_time_ns();
    }

    double elapsed_sec       = (end - start) / 1e9;
    double ops_per_sec       = iterations / elapsed_sec;
    double elements_per_sec  = (iterations * count) / elapsed_sec;
    double ns_per_op         = (end - start) / (double)iterations;

    printf("Test duration: %.3f seconds\n", elapsed_sec);
    printf("Iterations: %zu\n", iterations);
    printf("Elements per batch: %zu\n", count);
    printf("Throughput: %.0f ops/sec\n", ops_per_sec);
    printf("Throughput: %.0f elements/sec\n", elements_per_sec);
    printf("Latency per op: %.3f ns\n", ns_per_op);

    free(prices);
    free(volumes);
    free(amounts);
}

void bench_ticker_simd_suite(void) {
    printf("\n");
    printf("========================================\n");
    printf("  SIMD Batch Processing Benchmarks\n");
    printf("========================================\n");

    // Print SIMD level
    fc_simd_level_t level = fc_get_simd_level();
    printf("\nSIMD Level: ");
    switch (level) {
    case FC_SIMD_SCALAR:
        printf("None (Scalar)\n");
        break;
    case FC_SIMD_SSE42:
        printf("SSE4.2\n");
        break;
    case FC_SIMD_AVX2:
        printf("AVX2\n");
        break;
    case FC_SIMD_AVX512:
        printf("AVX-512\n");
        break;
    default:
        printf("Unknown\n");
        break;
    }

    bench_simd_batch_small();
    bench_simd_batch_medium();
    bench_simd_batch_large();
    bench_simd_batch_very_large();
    bench_simd_batch_varying_sizes();
    bench_simd_throughput();

    printf("\n");
    printf("========================================\n");
}
