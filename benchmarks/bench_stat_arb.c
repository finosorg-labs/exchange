/**
 * @file bench_stat_arb.c
 * @brief Performance benchmarks for statistical arbitrage strategy
 */

#include "bench_framework.h"
#include "strategy/stat_arb.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NUM_PAIRS_SMALL 10
#define NUM_PAIRS_MEDIUM 100
#define NUM_PAIRS_LARGE 1000
#define WINDOW_SIZE_SMALL 50
#define WINDOW_SIZE_MEDIUM 100
#define WINDOW_SIZE_LARGE 200

static double* generate_log_prices(size_t n, double base) {
    double* prices = (double*)malloc(n * sizeof(double));
    if (!prices)
        return NULL;

    srand(42);
    for (size_t i = 0; i < n; i++) {
        prices[i] = base + (rand() % 1000) * 0.001;
    }
    return prices;
}

static double* generate_prices(size_t n, double base) {
    double* prices = (double*)malloc(n * sizeof(double));
    if (!prices)
        return NULL;

    srand(43);
    for (size_t i = 0; i < n; i++) {
        prices[i] = base + (rand() % 1000) * 0.1;
    }
    return prices;
}

static double* generate_betas(size_t n) {
    double* betas = (double*)malloc(n * sizeof(double));
    if (!betas)
        return NULL;

    srand(44);
    for (size_t i = 0; i < n; i++) {
        betas[i] = 0.5 + (rand() % 300) * 0.01;
    }
    return betas;
}

typedef struct {
    double* beta_out;
    const double* log_pa;
    const double* log_pb;
    size_t n_pairs;
    size_t window;
} bench_beta_data_t;

typedef struct {
    double* spread_out;
    double* z_out;
    const double* pa;
    const double* pb;
    const double* beta;
    fc_ex_strat_zscore_state_t* states;
    size_t n_pairs;
} bench_spread_data_t;

static void bench_coint_beta_func(void* user_data) {
    bench_beta_data_t* data = (bench_beta_data_t*)user_data;

    fc_ex_strat_coint_beta(
        data->beta_out,
        data->log_pa,
        data->log_pb,
        data->n_pairs,
        data->window
    );
}

static void bench_coint_spread_z_func(void* user_data) {
    bench_spread_data_t* data = (bench_spread_data_t*)user_data;

    fc_ex_strat_coint_spread_z(
        data->spread_out,
        data->z_out,
        data->pa,
        data->pb,
        data->beta,
        data->states,
        data->n_pairs
    );
}

static void run_beta_benchmark(size_t n_pairs, size_t window, const char* label) {
    printf("\n=== Cointegration Beta Estimation: %s ===\n", label);
    printf("Pairs: %zu, Window: %zu\n", n_pairs, window);

    bench_beta_data_t data;
    data.n_pairs = n_pairs;
    data.window = window;
    data.beta_out = (double*)malloc(n_pairs * sizeof(double));
    data.log_pa = generate_log_prices(n_pairs * window, 4.6);
    data.log_pb = generate_log_prices(n_pairs * window, 4.0);

    if (!data.beta_out || !data.log_pa || !data.log_pb) {
        fprintf(stderr, "Memory allocation failed\n");
        goto cleanup;
    }

    fc_bench_config_t config = FC_BENCH_CONFIG_DEFAULT;
    config.name = "fc_ex_strat_coint_beta";
    config.data_size = n_pairs * window * sizeof(double) * 2;
    config.min_time_ms = 200.0;
    fc_bench_result_t result;
    fc_bench_run(&config, bench_coint_beta_func, &data, &result);
    fc_bench_result_print(&result);

    double throughput = (double)n_pairs / (result.mean_ns / 1e9);
    printf("Throughput: %.2f pairs/second\n", throughput);

cleanup:
    free(data.beta_out);
    free((void*)data.log_pa);
    free((void*)data.log_pb);
}

static void run_spread_z_benchmark(size_t n_pairs, size_t window_size, const char* label) {
    printf("\n=== Spread and Z-Score Computation: %s ===\n", label);
    printf("Pairs: %zu, Window Size: %zu\n", n_pairs, window_size);

    bench_spread_data_t data;
    data.n_pairs = n_pairs;
    data.spread_out = (double*)malloc(n_pairs * sizeof(double));
    data.z_out = (double*)malloc(n_pairs * sizeof(double));
    data.pa = generate_prices(n_pairs, 100.0);
    data.pb = generate_prices(n_pairs, 50.0);
    data.beta = generate_betas(n_pairs);
    data.states =
        (fc_ex_strat_zscore_state_t*)malloc(n_pairs * sizeof(fc_ex_strat_zscore_state_t));

    if (!data.spread_out || !data.z_out || !data.pa || !data.pb || !data.beta || !data.states) {
        fprintf(stderr, "Memory allocation failed\n");
        goto cleanup;
    }

    for (size_t i = 0; i < n_pairs; i++) {
        fc_ex_strat_zscore_state_init(&data.states[i], window_size);
    }

    for (int warmup = 0; warmup < 10; warmup++) {
        fc_ex_strat_coint_spread_z(
            data.spread_out, data.z_out, data.pa, data.pb, data.beta, data.states, n_pairs
        );
    }

    fc_bench_config_t config = FC_BENCH_CONFIG_DEFAULT;
    config.name = "fc_ex_strat_coint_spread_z";
    config.data_size = n_pairs * sizeof(double) * 5;
    config.min_time_ms = 200.0;
    fc_bench_result_t result;
    fc_bench_run(&config, bench_coint_spread_z_func, &data, &result);
    fc_bench_result_print(&result);

    double throughput = (double)n_pairs / (result.mean_ns / 1e9);
    printf("Throughput: %.2f pairs/second\n", throughput);
    printf("Time per pair: %.2f ns\n", result.mean_ns / (double)n_pairs);

    for (size_t i = 0; i < n_pairs; i++) {
        fc_ex_strat_zscore_state_free(&data.states[i]);
    }

cleanup:
    free(data.spread_out);
    free(data.z_out);
    free((void*)data.pa);
    free((void*)data.pb);
    free((void*)data.beta);
    free(data.states);
}

static void bench_zscore_state_operations(void) {
    printf("\n=== Z-Score State Operations ===\n");

    const size_t window_size = 100;
    fc_ex_strat_zscore_state_t state;

    fc_bench_time_t start = fc_bench_time_now();
    fc_ex_strat_zscore_state_init(&state, window_size);
    fc_bench_time_t end = fc_bench_time_now();

    printf("Init (window=%zu): %.2f ns\n", window_size,
           (double)fc_bench_time_elapsed_ns(&start, &end));

    start = fc_bench_time_now();
    for (int i = 0; i < 1000; i++) {
        fc_ex_strat_zscore_state_reset(&state);
    }
    end = fc_bench_time_now();

    printf("Reset (avg of 1000): %.2f ns\n",
           (double)fc_bench_time_elapsed_ns(&start, &end) / 1000.0);

    start = fc_bench_time_now();
    fc_ex_strat_zscore_state_free(&state);
    end = fc_bench_time_now();

    printf("Free: %.2f ns\n", (double)fc_bench_time_elapsed_ns(&start, &end));
}

static void bench_coint_beta_varying_window(void) {
    printf("\n=== Beta Estimation: Varying Window Size (100 pairs) ===\n");

    size_t windows[] = {50, 100, 200, 500, 1000};
    const size_t n_pairs = 100;

    for (size_t i = 0; i < sizeof(windows) / sizeof(windows[0]); i++) {
        size_t window = windows[i];

        bench_beta_data_t data;
        data.n_pairs = n_pairs;
        data.window = window;
        data.beta_out = (double*)malloc(n_pairs * sizeof(double));
        data.log_pa = generate_log_prices(n_pairs * window, 4.6);
        data.log_pb = generate_log_prices(n_pairs * window, 4.0);

        if (!data.beta_out || !data.log_pa || !data.log_pb) {
            fprintf(stderr, "Memory allocation failed\n");
            continue;
        }

        fc_bench_config_t config = FC_BENCH_CONFIG_DEFAULT;
        config.name = "fc_ex_strat_coint_beta";
        config.data_size = n_pairs * window * sizeof(double) * 2;
        config.min_time_ms = 100.0;
        fc_bench_result_t result;
        fc_bench_run(&config, bench_coint_beta_func, &data, &result);

        printf("Window=%zu: %.2f ms (%.2f us/pair)\n",
               window,
               result.mean_ns / 1e6,
               result.mean_ns / 1e3 / (double)n_pairs);

        free(data.beta_out);
        free((void*)data.log_pa);
        free((void*)data.log_pb);
    }
}

static void bench_spread_z_varying_pairs(void) {
    printf("\n=== Spread/Z-Score: Varying Pair Count (window=50) ===\n");

    size_t pair_counts[] = {10, 50, 100, 500, 1000, 5000};
    const size_t window_size = 50;

    for (size_t i = 0; i < sizeof(pair_counts) / sizeof(pair_counts[0]); i++) {
        size_t n_pairs = pair_counts[i];

        bench_spread_data_t data;
        data.n_pairs = n_pairs;
        data.spread_out = (double*)malloc(n_pairs * sizeof(double));
        data.z_out = (double*)malloc(n_pairs * sizeof(double));
        data.pa = generate_prices(n_pairs, 100.0);
        data.pb = generate_prices(n_pairs, 50.0);
        data.beta = generate_betas(n_pairs);
        data.states =
            (fc_ex_strat_zscore_state_t*)malloc(n_pairs * sizeof(fc_ex_strat_zscore_state_t));

        if (!data.spread_out || !data.z_out || !data.pa || !data.pb || !data.beta ||
            !data.states) {
            fprintf(stderr, "Memory allocation failed\n");
            continue;
        }

        for (size_t j = 0; j < n_pairs; j++) {
            fc_ex_strat_zscore_state_init(&data.states[j], window_size);
        }

        for (int warmup = 0; warmup < 5; warmup++) {
            fc_ex_strat_coint_spread_z(
                data.spread_out, data.z_out, data.pa, data.pb, data.beta, data.states, n_pairs
            );
        }

        fc_bench_config_t config = FC_BENCH_CONFIG_DEFAULT;
        config.name = "fc_ex_strat_coint_spread_z";
        config.data_size = n_pairs * sizeof(double) * 5;
        config.min_time_ms = 100.0;
        fc_bench_result_t result;
        fc_bench_run(&config, bench_coint_spread_z_func, &data, &result);

        printf("Pairs=%zu: %.2f us (%.2f ns/pair, %.2f Mpairs/s)\n",
               n_pairs,
               result.mean_ns / 1e3,
               result.mean_ns / (double)n_pairs,
               (double)n_pairs / (result.mean_ns / 1e3));

        for (size_t j = 0; j < n_pairs; j++) {
            fc_ex_strat_zscore_state_free(&data.states[j]);
        }

        free(data.spread_out);
        free(data.z_out);
        free((void*)data.pa);
        free((void*)data.pb);
        free((void*)data.beta);
        free(data.states);
    }
}

void bench_stat_arb(void) {
    printf("\n");
    printf("╔════════════════════════════════════════════════════════════════╗\n");
    printf("║         Statistical Arbitrage Strategy Benchmarks             ║\n");
    printf("╚════════════════════════════════════════════════════════════════╝\n");

    bench_zscore_state_operations();

    run_beta_benchmark(NUM_PAIRS_SMALL, WINDOW_SIZE_SMALL, "Small (10 pairs, 50 window)");
    run_beta_benchmark(NUM_PAIRS_MEDIUM, WINDOW_SIZE_MEDIUM, "Medium (100 pairs, 100 window)");
    run_beta_benchmark(NUM_PAIRS_LARGE, WINDOW_SIZE_LARGE, "Large (1000 pairs, 200 window)");

    run_spread_z_benchmark(NUM_PAIRS_SMALL, WINDOW_SIZE_SMALL, "Small (10 pairs, 50 window)");
    run_spread_z_benchmark(
        NUM_PAIRS_MEDIUM, WINDOW_SIZE_MEDIUM, "Medium (100 pairs, 100 window)"
    );
    run_spread_z_benchmark(NUM_PAIRS_LARGE, WINDOW_SIZE_LARGE, "Large (1000 pairs, 200 window)");

    bench_coint_beta_varying_window();
    bench_spread_z_varying_pairs();

    printf("\n");
}

void bench_stat_arb_run(void) {
    bench_stat_arb();
}
