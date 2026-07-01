/**
 * @file bench_market_maker.c
 * @brief Performance benchmarks for market maker strategy
 */

#include "bench_framework.h"
#include "strategy/market_maker.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NUM_SYMBOLS_SMALL 100
#define NUM_SYMBOLS_MEDIUM 1000
#define NUM_SYMBOLS_LARGE 5000

static double* generate_mid_prices(size_t n) {
    double* prices = (double*)malloc(n * sizeof(double));
    if (!prices)
        return NULL;

    for (size_t i = 0; i < n; i++) {
        prices[i] = 100.0 + (i % 100) * 0.5;
    }
    return prices;
}

static double* generate_inventories(size_t n) {
    double* inventories = (double*)malloc(n * sizeof(double));
    if (!inventories)
        return NULL;

    srand(42);
    for (size_t i = 0; i < n; i++) {
        inventories[i] = -50.0 + (rand() % 1000) * 0.1;
    }
    return inventories;
}

static double* generate_volatilities(size_t n) {
    double* volatilities = (double*)malloc(n * sizeof(double));
    if (!volatilities)
        return NULL;

    srand(43);
    for (size_t i = 0; i < n; i++) {
        volatilities[i] = 0.01 + (rand() % 100) * 0.0001;
    }
    return volatilities;
}

static double* generate_arrival_rates(size_t n) {
    double* arrival_rates = (double*)malloc(n * sizeof(double));
    if (!arrival_rates)
        return NULL;

    srand(44);
    for (size_t i = 0; i < n; i++) {
        arrival_rates[i] = 1.0 + (rand() % 100) * 0.5;
    }
    return arrival_rates;
}

typedef struct {
    fc_ex_strat_mm_params_t params;
    double* bid_out;
    double* ask_out;
    double* reserve_out;
} bench_data_t;

static void bench_strat_mm_quotes_func(void* user_data) {
    bench_data_t* data = (bench_data_t*)user_data;

    fc_ex_strat_mm_quotes(
        data->bid_out,
        data->ask_out,
        data->reserve_out,
        &data->params
    );
}

static void bench_strat_mm_reservation_price_func(void* user_data) {
    bench_data_t* data = (bench_data_t*)user_data;

    fc_ex_strat_mm_reservation_price(
        data->reserve_out,
        &data->params
    );
}

static void bench_strat_mm_optimal_spread_func(void* user_data) {
    bench_data_t* data = (bench_data_t*)user_data;

    fc_ex_strat_mm_optimal_spread(
        data->reserve_out,
        &data->params
    );
}

static void run_benchmark_for_size(size_t n, const char* size_label) {
    bench_data_t data;
    data.params.mid = generate_mid_prices(n);
    data.params.inventory = generate_inventories(n);
    data.params.sigma = generate_volatilities(n);
    data.params.kappa = generate_arrival_rates(n);
    data.params.gamma = 0.1;
    data.params.t_minus_t = 1.0;
    data.params.n = n;
    data.bid_out = (double*)malloc(n * sizeof(double));
    data.ask_out = (double*)malloc(n * sizeof(double));
    data.reserve_out = (double*)malloc(n * sizeof(double));

    if (!data.params.mid || !data.params.inventory || !data.params.sigma ||
        !data.params.kappa || !data.bid_out || !data.ask_out || !data.reserve_out) {
        fprintf(stderr, "Failed to allocate memory for benchmark\n");
        goto cleanup;
    }

    char bench_name[256];

    snprintf(bench_name, sizeof(bench_name), "strat_mm_quotes_%s", size_label);
    fc_bench_config_t config = FC_BENCH_CONFIG_DEFAULT;
    config.name = bench_name;
    config.data_size = n * sizeof(double) * 6;
    config.min_time_ms = 200.0;
    fc_bench_result_t result;
    fc_bench_run(&config, bench_strat_mm_quotes_func, &data, &result);
    fc_bench_result_print(&result);

    snprintf(bench_name, sizeof(bench_name), "strat_mm_reservation_price_%s", size_label);
    config.name = bench_name;
    config.data_size = n * sizeof(double) * 4;
    fc_bench_run(&config, bench_strat_mm_reservation_price_func, &data, &result);
    fc_bench_result_print(&result);

    snprintf(bench_name, sizeof(bench_name), "strat_mm_optimal_spread_%s", size_label);
    config.name = bench_name;
    config.data_size = n * sizeof(double) * 3;
    fc_bench_run(&config, bench_strat_mm_optimal_spread_func, &data, &result);
    fc_bench_result_print(&result);

cleanup:
    free((void*)data.params.mid);
    free((void*)data.params.inventory);
    free((void*)data.params.sigma);
    free((void*)data.params.kappa);
    free(data.bid_out);
    free(data.ask_out);
    free(data.reserve_out);
}

void bench_market_maker_run(void) {
    printf("=== Market Maker Strategy Benchmarks ===\n\n");

    fc_bench_print_header();

    printf("\n--- Small batch (100 symbols) ---\n");
    run_benchmark_for_size(NUM_SYMBOLS_SMALL, "small");

    printf("\n--- Medium batch (1000 symbols) ---\n");
    run_benchmark_for_size(NUM_SYMBOLS_MEDIUM, "medium");

    printf("\n--- Large batch (5000 symbols) ---\n");
    run_benchmark_for_size(NUM_SYMBOLS_LARGE, "large");
}
