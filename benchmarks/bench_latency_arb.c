/**
 * @file bench_latency_arb.c
 * @brief Performance benchmarks for latency arbitrage strategy
 */

#include "bench_framework.h"
#include "strategy/latency_arb.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NUM_PAIRS_SMALL 10
#define NUM_PAIRS_MEDIUM 100
#define NUM_PAIRS_LARGE 1000
#define WINDOW_SIZE_SMALL 50
#define WINDOW_SIZE_MEDIUM 100
#define WINDOW_SIZE_LARGE 200

static double* generate_prices(size_t n, double base, int seed) {
    double* prices = (double*)malloc(n * sizeof(double));
    if (!prices)
        return NULL;

    srand(seed);
    for (size_t i = 0; i < n; i++) {
        prices[i] = base + (rand() % 1000) * 0.1 + i * 0.05;
    }
    return prices;
}

static double* generate_coef(size_t n, double base, int seed) {
    double* coef = (double*)malloc(n * sizeof(double));
    if (!coef)
        return NULL;

    srand(seed);
    for (size_t i = 0; i < n; i++) {
        coef[i] = base + (rand() % 100) * 0.01;
    }
    return coef;
}

static double* generate_theta(size_t n, double base) {
    double* theta = (double*)malloc(n * sizeof(double));
    if (!theta)
        return NULL;

    for (size_t i = 0; i < n; i++) {
        theta[i] = base;
    }
    return theta;
}

typedef struct {
    double* coef_a_out;
    double* coef_b_out;
    const double* hist_fast;
    const double* hist_slow;
    size_t n_pairs;
    size_t window;
} bench_calibrate_data_t;

typedef struct {
    double* dev_out;
    int* hit_out;
    const double* price_fast;
    const double* price_slow;
    const double* coef_a;
    const double* coef_b;
    const double* theta;
    size_t n;
} bench_signal_data_t;

static void bench_latarb_calibrate_func(void* user_data) {
    bench_calibrate_data_t* data = (bench_calibrate_data_t*)user_data;

    fc_ex_strat_latarb_calibrate(
        data->coef_a_out,
        data->coef_b_out,
        data->hist_fast,
        data->hist_slow,
        data->n_pairs,
        data->window
    );
}

static void bench_latarb_signal_func(void* user_data) {
    bench_signal_data_t* data = (bench_signal_data_t*)user_data;

    fc_ex_strat_latarb_signal(
        data->dev_out,
        data->hit_out,
        data->price_fast,
        data->price_slow,
        data->coef_a,
        data->coef_b,
        data->theta,
        data->n
    );
}

static void run_calibrate_benchmark(size_t n_pairs, size_t window, const char* label) {
    bench_calibrate_data_t data;
    data.n_pairs = n_pairs;
    data.window = window;
    data.coef_a_out = (double*)malloc(n_pairs * sizeof(double));
    data.coef_b_out = (double*)malloc(n_pairs * sizeof(double));
    data.hist_fast = generate_prices(n_pairs * window, 100.0, 42);
    data.hist_slow = generate_prices(n_pairs * window, 120.0, 43);

    if (!data.coef_a_out || !data.coef_b_out || !data.hist_fast || !data.hist_slow) {
        fprintf(stderr, "Memory allocation failed\n");
        goto cleanup;
    }

    fc_bench_config_t config = FC_BENCH_CONFIG_DEFAULT;
    config.name = label;
    config.data_size = n_pairs * window * sizeof(double) * 2;
    config.min_time_ms = 200.0;
    fc_bench_result_t result;
    fc_bench_run(&config, bench_latarb_calibrate_func, &data, &result);
    fc_bench_result_print(&result);

cleanup:
    free(data.coef_a_out);
    free(data.coef_b_out);
    free((void*)data.hist_fast);
    free((void*)data.hist_slow);
}

static void run_signal_benchmark(size_t n, const char* label) {
    bench_signal_data_t data;
    data.n = n;
    data.dev_out = (double*)malloc(n * sizeof(double));
    data.hit_out = (int*)malloc(n * sizeof(int));
    data.price_fast = generate_prices(n, 100.0, 44);
    data.price_slow = generate_prices(n, 120.0, 45);
    data.coef_a = generate_coef(n, 1.2, 46);
    data.coef_b = generate_coef(n, 5.0, 47);
    data.theta = generate_theta(n, 2.0);

    if (!data.dev_out || !data.hit_out || !data.price_fast || !data.price_slow ||
        !data.coef_a || !data.coef_b || !data.theta) {
        fprintf(stderr, "Memory allocation failed\n");
        goto cleanup;
    }

    fc_bench_config_t config = FC_BENCH_CONFIG_DEFAULT;
    config.name = label;
    config.data_size = n * sizeof(double) * 5 + n * sizeof(int);
    config.min_time_ms = 200.0;
    fc_bench_result_t result;
    fc_bench_run(&config, bench_latarb_signal_func, &data, &result);
    fc_bench_result_print(&result);

cleanup:
    free(data.dev_out);
    free(data.hit_out);
    free((void*)data.price_fast);
    free((void*)data.price_slow);
    free((void*)data.coef_a);
    free((void*)data.coef_b);
    free((void*)data.theta);
}

static void bench_calibrate_small(void) {
    run_calibrate_benchmark(NUM_PAIRS_SMALL, WINDOW_SIZE_SMALL, "latarb_calibrate_10pairs_50window");
}

static void bench_calibrate_medium(void) {
    run_calibrate_benchmark(NUM_PAIRS_MEDIUM, WINDOW_SIZE_MEDIUM, "latarb_calibrate_100pairs_100window");
}

static void bench_calibrate_large(void) {
    run_calibrate_benchmark(NUM_PAIRS_LARGE, WINDOW_SIZE_SMALL, "latarb_calibrate_1000pairs_50window");
}

static void bench_signal_small(void) {
    run_signal_benchmark(NUM_PAIRS_SMALL, "latarb_signal_10pairs");
}

static void bench_signal_medium(void) {
    run_signal_benchmark(NUM_PAIRS_MEDIUM, "latarb_signal_100pairs");
}

static void bench_signal_large(void) {
    run_signal_benchmark(NUM_PAIRS_LARGE, "latarb_signal_1000pairs");
}

static void bench_signal_xlarge(void) {
    run_signal_benchmark(10000, "latarb_signal_10000pairs");
}

static void bench_throughput_analysis(void) {
    printf("\n=== Latency Arbitrage Throughput Analysis ===\n\n");

    const size_t sizes[] = {10, 100, 1000, 10000};
    const size_t n_sizes = sizeof(sizes) / sizeof(sizes[0]);

    printf("Signal generation (hot path simulation):\n");
    printf("%-10s %-15s %-15s %-15s\n", "Size", "Time (ns)", "Throughput", "Per-pair (ns)");
    printf("%-10s %-15s %-15s %-15s\n", "----", "---------", "----------", "-------------");

    for (size_t i = 0; i < n_sizes; i++) {
        bench_signal_data_t data;
        data.n = sizes[i];
        data.dev_out = (double*)malloc(sizes[i] * sizeof(double));
        data.hit_out = (int*)malloc(sizes[i] * sizeof(int));
        data.price_fast = generate_prices(sizes[i], 100.0, 44);
        data.price_slow = generate_prices(sizes[i], 120.0, 45);
        data.coef_a = generate_coef(sizes[i], 1.2, 46);
        data.coef_b = generate_coef(sizes[i], 5.0, 47);
        data.theta = generate_theta(sizes[i], 2.0);

        if (!data.dev_out || !data.hit_out || !data.price_fast || !data.price_slow ||
            !data.coef_a || !data.coef_b || !data.theta) {
            free(data.dev_out);
            free(data.hit_out);
            free((void*)data.price_fast);
            free((void*)data.price_slow);
            free((void*)data.coef_a);
            free((void*)data.coef_b);
            free((void*)data.theta);
            continue;
        }

        fc_bench_config_t config = FC_BENCH_CONFIG_DEFAULT;
        config.name = "latarb_signal_throughput";
        config.min_time_ms = 100.0;
        fc_bench_result_t result;
        fc_bench_run(&config, bench_latarb_signal_func, &data, &result);

        double time_ns = result.mean_ns;
        double throughput = (sizes[i] * 1e9) / time_ns;
        double per_pair_ns = time_ns / sizes[i];

        printf("%-10zu %-15.2f %-15.2f %-15.2f\n",
            sizes[i], time_ns, throughput, per_pair_ns);

        free(data.dev_out);
        free(data.hit_out);
        free((void*)data.price_fast);
        free((void*)data.price_slow);
        free((void*)data.coef_a);
        free((void*)data.coef_b);
        free((void*)data.theta);
    }

    printf("\nNote: Real-time hot path runs in FPGA (<200ns), not software\n");
    printf("These benchmarks are for offline backtesting and parameter tuning\n");
}

void bench_latency_arb_run(void) {
    printf("\n=== Latency Arbitrage Strategy Benchmarks ===\n\n");

    printf("--- Calibration (Offline/Periodic) ---\n");
    bench_calibrate_small();
    bench_calibrate_medium();
    bench_calibrate_large();

    printf("\n--- Signal Generation (Backtesting) ---\n");
    bench_signal_small();
    bench_signal_medium();
    bench_signal_large();
    bench_signal_xlarge();

    bench_throughput_analysis();
}
