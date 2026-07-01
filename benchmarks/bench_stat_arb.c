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

static void bench_incremental_vs_batch(void);

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
    config.name = label;
    config.data_size = n_pairs * window * sizeof(double) * 2;
    config.min_time_ms = 200.0;
    fc_bench_result_t result;
    fc_bench_run(&config, bench_coint_beta_func, &data, &result);
    fc_bench_result_print(&result);

cleanup:
    free(data.beta_out);
    free((void*)data.log_pa);
    free((void*)data.log_pb);
}

static void run_spread_z_benchmark(size_t n_pairs, size_t window_size, const char* label) {
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
    config.name = label;
    config.data_size = n_pairs * sizeof(double) * 5;
    config.min_time_ms = 200.0;
    fc_bench_result_t result;
    fc_bench_run(&config, bench_coint_spread_z_func, &data, &result);
    fc_bench_result_print(&result);

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
    printf("\n--- Z-Score State Operations ---\n");
    const size_t window_size = 100;
    fc_ex_strat_zscore_state_t state;

    fc_bench_time_t start = fc_bench_time_now();
    fc_ex_strat_zscore_state_init(&state, window_size);
    fc_bench_time_t end = fc_bench_time_now();
    double init_ns = (double)fc_bench_time_elapsed_ns(&start, &end);

    printf("StatArb/ZScoreInit                               %6d          %8.2f ns/op\n",
           1, init_ns);

    start = fc_bench_time_now();
    for (int i = 0; i < 1000; i++) {
        fc_ex_strat_zscore_state_reset(&state);
    }
    end = fc_bench_time_now();
    double reset_ns = (double)fc_bench_time_elapsed_ns(&start, &end) / 1000.0;

    printf("StatArb/ZScoreReset                               %6d          %8.2f ns/op\n",
           1000, reset_ns);

    start = fc_bench_time_now();
    fc_ex_strat_zscore_state_free(&state);
    end = fc_bench_time_now();
    double free_ns = (double)fc_bench_time_elapsed_ns(&start, &end);

    printf("StatArb/ZScoreFree                               %6d          %8.2f ns/op\n",
           1, free_ns);
}

static void bench_coint_beta_varying_window(void) {
    size_t windows[] = {50, 100, 200, 500, 1000};
    const size_t n_pairs = 100;

    printf("\n--- Beta Estimation: Varying Window Size (100 pairs) ---\n");

    for (size_t i = 0; i < sizeof(windows) / sizeof(windows[0]); i++) {
        size_t window = windows[i];
        char name[64];
        snprintf(name, sizeof(name), "StatArb/CointBeta/Window=%zu", window);

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
        config.name = name;
        config.data_size = n_pairs * window * sizeof(double) * 2;
        config.min_time_ms = 100.0;
        fc_bench_result_t result;
        fc_bench_run(&config, bench_coint_beta_func, &data, &result);
        fc_bench_result_print(&result);

        free(data.beta_out);
        free((void*)data.log_pa);
        free((void*)data.log_pb);
    }
}

static void bench_spread_z_varying_pairs(void) {
    size_t pair_counts[] = {10, 50, 100, 500, 1000, 5000};
    const size_t window_size = 50;

    printf("\n--- Spread/Z-Score: Varying Pair Count (window=50) ---\n");

    for (size_t i = 0; i < sizeof(pair_counts) / sizeof(pair_counts[0]); i++) {
        size_t n_pairs = pair_counts[i];
        char name[64];
        snprintf(name, sizeof(name), "StatArb/SpreadZ/Pairs=%zu", n_pairs);

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
        config.name = name;
        config.data_size = n_pairs * sizeof(double) * 5;
        config.min_time_ms = 100.0;
        fc_bench_result_t result;
        fc_bench_run(&config, bench_coint_spread_z_func, &data, &result);
        fc_bench_result_print(&result);

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
    printf("\nStatistical Arbitrage Strategy Benchmarks\n");
    printf("------------------------------------------------------------\n");

    bench_zscore_state_operations();

    run_beta_benchmark(NUM_PAIRS_SMALL, WINDOW_SIZE_SMALL, "StatArb/CointBeta/10×50");
    run_beta_benchmark(NUM_PAIRS_MEDIUM, WINDOW_SIZE_MEDIUM, "StatArb/CointBeta/100×100");
    run_beta_benchmark(NUM_PAIRS_LARGE, WINDOW_SIZE_LARGE, "StatArb/CointBeta/1000×200");

    run_spread_z_benchmark(NUM_PAIRS_SMALL, WINDOW_SIZE_SMALL, "StatArb/SpreadZ/10×50");
    run_spread_z_benchmark(NUM_PAIRS_MEDIUM, WINDOW_SIZE_MEDIUM, "StatArb/SpreadZ/100×100");
    run_spread_z_benchmark(NUM_PAIRS_LARGE, WINDOW_SIZE_LARGE, "StatArb/SpreadZ/1000×200");

    bench_coint_beta_varying_window();
    bench_spread_z_varying_pairs();

    bench_incremental_vs_batch();

    printf("\nPerformance Targets:\n");
    printf("  Beta estimation:                ~10-100 μs per 100 pairs\n");
    printf("  Spread/Z-Score update:          ~0.5-5 μs per 100 pairs\n");
    printf("  Strategy decision budget:       ~2-10 μs total\n");
}

void bench_stat_arb_run(void) {
    bench_stat_arb();
}

static void bench_incremental_vs_batch(void) {
    const size_t n_pairs = 100;
    const size_t window_size = 50;
    const size_t n_updates = 100;

    printf("\n--- Incremental vs Batch Z-Score Comparison ---\n");

    double* pa = generate_prices(n_pairs, 100.0);
    double* pb = generate_prices(n_pairs, 50.0);
    double* beta = generate_betas(n_pairs);
    double* spread_out = (double*)malloc(n_pairs * sizeof(double));
    double* z_out = (double*)malloc(n_pairs * sizeof(double));

    fc_ex_strat_zscore_state_t* states =
        (fc_ex_strat_zscore_state_t*)malloc(n_pairs * sizeof(fc_ex_strat_zscore_state_t));

    for (size_t i = 0; i < n_pairs; i++) {
        fc_ex_strat_zscore_state_init(&states[i], window_size);
    }

    fc_bench_time_t start = fc_bench_time_now();
    for (size_t update = 0; update < n_updates; update++) {
        for (size_t i = 0; i < n_pairs; i++) {
            pa[i] += (rand() % 100 - 50) / 1000.0;
            pb[i] += (rand() % 100 - 50) / 1000.0;
        }
        fc_ex_strat_coint_spread_z(spread_out, z_out, pa, pb, beta, states, n_pairs);
    }
    fc_bench_time_t end = fc_bench_time_now();

    uint64_t incremental_ns = fc_bench_time_elapsed_ns(&start, &end);
    double incremental_us = (double)incremental_ns / n_updates / 1000.0;
    printf("  Incremental (%zu updates): %.2f μs/op (%.2f ns per pair per update)\n",
           n_updates, incremental_us, (double)incremental_ns / n_updates / n_pairs);

    for (size_t i = 0; i < n_pairs; i++) {
        fc_ex_strat_zscore_state_free(&states[i]);
    }

    double* spread_history = (double*)malloc(n_pairs * window_size * sizeof(double));
    double* z_batch = (double*)malloc(n_pairs * window_size * sizeof(double));

    for (size_t i = 0; i < n_pairs * window_size; i++) {
        spread_history[i] = (rand() % 1000) / 100.0;
    }

    start = fc_bench_time_now();
    fc_ex_strat_coint_spread_z_batch(z_batch, spread_history, n_pairs, window_size);
    end = fc_bench_time_now();

    uint64_t batch_ns = fc_bench_time_elapsed_ns(&start, &end);
    double batch_us = (double)batch_ns / 1000.0;
    size_t batch_data_size = n_pairs * window_size * sizeof(double) * 2;
    double throughput_mb_s = (double)batch_data_size / (batch_ns / 1e9) / (1024 * 1024);
    printf("  Batch recalculation: %.2f μs (%.2f MB/s, %.2f ns per element)\n",
           batch_us, throughput_mb_s, (double)batch_ns / (n_pairs * window_size));

    free(pa);
    free(pb);
    free(beta);
    free(spread_out);
    free(z_out);
    free(states);
    free(spread_history);
    free(z_batch);
}
