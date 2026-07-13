/**
 * @file bench_index_rebal.c
 * @brief Performance benchmarks for index rebalancing strategy
 */

#include "bench_framework.h"
#include "strategy/index_rebal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NUM_INDICES_SMALL  10
#define NUM_INDICES_MEDIUM 50
#define NUM_INDICES_LARGE  100
#define MAX_CONSTITUENTS   50

static double* generate_prices(size_t n_index, int max_constituents) {
    double* prices = (double*) malloc(n_index * max_constituents * sizeof(double));
    if (!prices)
        return NULL;

    srand(42);
    for (size_t i = 0; i < n_index * max_constituents; i++) {
        prices[i] = 50.0 + (rand() % 500) * 1.0;
    }
    return prices;
}

static double* generate_weights(size_t n_index, int max_constituents, const int* n_constituents) {
    double* weights = (double*) malloc(n_index * max_constituents * sizeof(double));
    if (!weights)
        return NULL;

    srand(43);
    for (size_t j = 0; j < n_index; j++) {
        double sum = 0.0;
        for (int i = 0; i < n_constituents[j]; i++) {
            weights[j * max_constituents + i] = (rand() % 100 + 1) / 100.0;
            sum += weights[j * max_constituents + i];
        }

        for (int i = 0; i < n_constituents[j]; i++) {
            weights[j * max_constituents + i] /= sum;
        }

        for (int i = n_constituents[j]; i < max_constituents; i++) {
            weights[j * max_constituents + i] = 0.0;
        }
    }
    return weights;
}

static double* generate_quantities(size_t n_index, int max_constituents) {
    double* quantities = (double*) malloc(n_index * max_constituents * sizeof(double));
    if (!quantities)
        return NULL;

    srand(44);
    for (size_t i = 0; i < n_index * max_constituents; i++) {
        quantities[i] = (rand() % 1000) * 1.0;
    }
    return quantities;
}

static double* generate_tracking_aum(size_t n_index) {
    double* aum = (double*) malloc(n_index * sizeof(double));
    if (!aum)
        return NULL;

    srand(45);
    for (size_t i = 0; i < n_index; i++) {
        aum[i] = 1000000.0 + (rand() % 10000000);
    }
    return aum;
}

static int* generate_n_constituents(size_t n_index, int max_constituents) {
    int* n_const = (int*) malloc(n_index * sizeof(int));
    if (!n_const)
        return NULL;

    srand(46);
    for (size_t i = 0; i < n_index; i++) {
        n_const[i] = 10 + (rand() % (max_constituents - 10));
    }
    return n_const;
}

typedef struct {
    double* nav_out;
    double* rebal_qty_out;
    const double* prices;
    const double* weights;
    const double* current_qty;
    const double* tracking_aum;
    const int* n_constituents;
    size_t n_index;
    int max_constituents;
} bench_data_t;

static void bench_index_nav_only_func(void* user_data) {
    bench_data_t* data = (bench_data_t*) user_data;

    fc_ex_strat_index_nav(
        data->nav_out,
        NULL,
        data->prices,
        data->weights,
        NULL,
        NULL,
        data->n_constituents,
        data->n_index,
        data->max_constituents
    );
}

static void bench_index_nav_with_rebalancing_func(void* user_data) {
    bench_data_t* data = (bench_data_t*) user_data;

    fc_ex_strat_index_nav(
        data->nav_out,
        data->rebal_qty_out,
        data->prices,
        data->weights,
        data->current_qty,
        data->tracking_aum,
        data->n_constituents,
        data->n_index,
        data->max_constituents
    );
}

static void run_benchmark_for_size(size_t n_index, const char* size_label) {
    bench_data_t data;
    data.n_index          = n_index;
    data.max_constituents = MAX_CONSTITUENTS;
    data.n_constituents   = generate_n_constituents(n_index, MAX_CONSTITUENTS);
    data.prices           = generate_prices(n_index, MAX_CONSTITUENTS);
    data.weights          = generate_weights(n_index, MAX_CONSTITUENTS, data.n_constituents);
    data.current_qty      = generate_quantities(n_index, MAX_CONSTITUENTS);
    data.tracking_aum     = generate_tracking_aum(n_index);
    data.nav_out          = (double*) malloc(n_index * sizeof(double));
    data.rebal_qty_out    = (double*) malloc(n_index * MAX_CONSTITUENTS * sizeof(double));

    if (!data.n_constituents || !data.prices || !data.weights || !data.current_qty ||
        !data.tracking_aum || !data.nav_out || !data.rebal_qty_out) {
        fprintf(stderr, "Failed to allocate memory for benchmark\n");
        goto cleanup;
    }

    char bench_name[256];

    snprintf(bench_name, sizeof(bench_name), "index_nav_only_%s", size_label);
    fc_bench_config_t config = FC_BENCH_CONFIG_DEFAULT;
    config.name              = bench_name;
    config.data_size         = n_index * MAX_CONSTITUENTS * sizeof(double) * 2;
    config.min_time_ms       = 200.0;
    fc_bench_result_t result;
    fc_bench_run(&config, bench_index_nav_only_func, &data, &result);
    fc_bench_result_print(&result);

    snprintf(bench_name, sizeof(bench_name), "index_nav_with_rebalancing_%s", size_label);
    config.name      = bench_name;
    config.data_size = n_index * MAX_CONSTITUENTS * sizeof(double) * 5;
    fc_bench_run(&config, bench_index_nav_with_rebalancing_func, &data, &result);
    fc_bench_result_print(&result);

cleanup:
    free((void*) data.n_constituents);
    free((void*) data.prices);
    free((void*) data.weights);
    free((void*) data.current_qty);
    free((void*) data.tracking_aum);
    free(data.nav_out);
    free(data.rebal_qty_out);
}

void bench_index_rebal_run(void) {
    printf("=== Index Rebalancing Strategy Benchmarks ===\n\n");

    fc_bench_print_header();

    printf("\n--- Small batch (10 indices x ~30 constituents) ---\n");
    run_benchmark_for_size(NUM_INDICES_SMALL, "small");

    printf("\n--- Medium batch (50 indices x ~30 constituents) ---\n");
    run_benchmark_for_size(NUM_INDICES_MEDIUM, "medium");

    printf("\n--- Large batch (100 indices x ~30 constituents) ---\n");
    run_benchmark_for_size(NUM_INDICES_LARGE, "large");
}
