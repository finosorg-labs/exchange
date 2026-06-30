/**
 * @file bench_market_maker_quotes.c
 * @brief Performance benchmarks for market maker quote calculation
 */

#include "bench_framework.h"
#include "market_maker_quotes.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NUM_SYMBOLS_SMALL 100
#define NUM_SYMBOLS_MEDIUM 1000
#define NUM_SYMBOLS_LARGE 5000

static double* generate_mid_prices(size_t n) {
    double* prices = (double*) malloc(n * sizeof(double));
    if (!prices)
        return NULL;

    for (size_t i = 0; i < n; i++) {
        prices[i] = 100.0 + (i % 100) * 0.5;
    }
    return prices;
}

static double* generate_inventories(size_t n) {
    double* inventories = (double*) malloc(n * sizeof(double));
    if (!inventories)
        return NULL;

    srand(42);
    for (size_t i = 0; i < n; i++) {
        inventories[i] = -50.0 + (rand() % 1000) * 0.1;
    }
    return inventories;
}

static double* generate_volatilities(size_t n) {
    double* volatilities = (double*) malloc(n * sizeof(double));
    if (!volatilities)
        return NULL;

    srand(43);
    for (size_t i = 0; i < n; i++) {
        volatilities[i] = 0.01 + (rand() % 100) * 0.0001;
    }
    return volatilities;
}

static double* generate_arrival_rates(size_t n) {
    double* arrival_rates = (double*) malloc(n * sizeof(double));
    if (!arrival_rates)
        return NULL;

    srand(44);
    for (size_t i = 0; i < n; i++) {
        arrival_rates[i] = 1.0 + (rand() % 100) * 0.5;
    }
    return arrival_rates;
}

typedef struct {
    double* mid_prices;
    double* inventories;
    double* volatilities;
    double* arrival_rates;
    double* bid_prices;
    double* ask_prices;
    double risk_aversion;
    double time_horizon;
    size_t n;
} bench_data_t;

static void bench_market_maker_quotes_func(void* user_data) {
    bench_data_t* data = (bench_data_t*) user_data;

    fc_market_maker_quotes(
        data->bid_prices,
        data->ask_prices,
        data->mid_prices,
        data->inventories,
        data->volatilities,
        data->arrival_rates,
        data->risk_aversion,
        data->time_horizon,
        data->n
    );
}

static void bench_reservation_price_func(void* user_data) {
    bench_data_t* data = (bench_data_t*) user_data;

    fc_market_maker_reservation_price(
        data->bid_prices,
        data->mid_prices,
        data->inventories,
        data->volatilities,
        data->risk_aversion,
        data->time_horizon,
        data->n
    );
}

static void bench_optimal_spread_func(void* user_data) {
    bench_data_t* data = (bench_data_t*) user_data;

    fc_market_maker_optimal_spread(
        data->bid_prices,
        data->volatilities,
        data->arrival_rates,
        data->risk_aversion,
        data->time_horizon,
        data->n
    );
}

static void run_benchmark_for_size(size_t n, const char* size_label) {
    bench_data_t data;
    data.n = n;
    data.mid_prices = generate_mid_prices(n);
    data.inventories = generate_inventories(n);
    data.volatilities = generate_volatilities(n);
    data.arrival_rates = generate_arrival_rates(n);
    data.bid_prices = (double*) malloc(n * sizeof(double));
    data.ask_prices = (double*) malloc(n * sizeof(double));
    data.risk_aversion = 0.1;
    data.time_horizon = 1.0;

    if (!data.mid_prices || !data.inventories || !data.volatilities || !data.arrival_rates ||
        !data.bid_prices || !data.ask_prices) {
        fprintf(stderr, "Failed to allocate memory for benchmark\n");
        goto cleanup;
    }

    char bench_name[256];

    snprintf(bench_name, sizeof(bench_name), "market_maker_quotes_%s", size_label);
    fc_bench_config_t config = FC_BENCH_CONFIG_DEFAULT;
    config.name = bench_name;
    config.data_size = n * sizeof(double) * 6;
    config.min_time_ms = 200.0;
    fc_bench_result_t result;
    fc_bench_run(&config, bench_market_maker_quotes_func, &data, &result);
    fc_bench_result_print(&result);

    snprintf(bench_name, sizeof(bench_name), "reservation_price_%s", size_label);
    config.name = bench_name;
    config.data_size = n * sizeof(double) * 4;
    fc_bench_run(&config, bench_reservation_price_func, &data, &result);
    fc_bench_result_print(&result);

    snprintf(bench_name, sizeof(bench_name), "optimal_spread_%s", size_label);
    config.name = bench_name;
    config.data_size = n * sizeof(double) * 3;
    fc_bench_run(&config, bench_optimal_spread_func, &data, &result);
    fc_bench_result_print(&result);

cleanup:
    free(data.mid_prices);
    free(data.inventories);
    free(data.volatilities);
    free(data.arrival_rates);
    free(data.bid_prices);
    free(data.ask_prices);
}

static void bench_market_maker_quotes_small(void) {
    run_benchmark_for_size(NUM_SYMBOLS_SMALL, "100_symbols");
}

static void bench_market_maker_quotes_medium(void) {
    run_benchmark_for_size(NUM_SYMBOLS_MEDIUM, "1000_symbols");
}

static void bench_market_maker_quotes_large(void) {
    run_benchmark_for_size(NUM_SYMBOLS_LARGE, "5000_symbols");
}

static void bench_market_maker_quotes_single(void) {
    bench_data_t data;
    data.n = 1;
    double mid_price = 100.0;
    double inventory = 10.0;
    double volatility = 0.02;
    double arrival_rate = 10.0;
    double bid_price, ask_price;

    data.mid_prices = &mid_price;
    data.inventories = &inventory;
    data.volatilities = &volatility;
    data.arrival_rates = &arrival_rate;
    data.bid_prices = &bid_price;
    data.ask_prices = &ask_price;
    data.risk_aversion = 0.1;
    data.time_horizon = 1.0;

    fc_bench_config_t config = FC_BENCH_CONFIG_DEFAULT;
    config.name = "market_maker_quotes_single_symbol";
    config.data_size = sizeof(double) * 6;
    config.min_time_ms = 200.0;
    config.min_iterations = 100;
    fc_bench_result_t result;
    fc_bench_run(&config, bench_market_maker_quotes_func, &data, &result);
    fc_bench_result_print(&result);
}

void bench_market_maker_quotes_run(void) {
    printf("\n");
    printf("========================================\n");
    printf("Market Maker Quotes Benchmarks\n");
    printf("========================================\n");
    printf("\n");

    fc_bench_print_header();

    printf("\n--- Single Symbol Benchmark ---\n");
    bench_market_maker_quotes_single();

    printf("\n--- Batch Size Benchmarks ---\n");
    bench_market_maker_quotes_small();
    bench_market_maker_quotes_medium();
    bench_market_maker_quotes_large();

    printf("\n");
    printf("========================================\n");
    printf("Market Maker Quotes Benchmarks Complete\n");
    printf("========================================\n");
}
