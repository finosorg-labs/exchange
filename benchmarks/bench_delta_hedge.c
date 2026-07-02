/**
 * @file bench_delta_hedge.c
 * @brief Performance benchmarks for delta hedging strategy
 */

#include "bench_framework.h"
#include "strategy/delta_hedge.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NUM_BOOKS_SMALL 10
#define NUM_BOOKS_MEDIUM 100
#define NUM_BOOKS_LARGE 1000
#define MAX_LEGS 10

static void setup_test_data(
    double* leg_delta,
    double* leg_qty,
    int* n_legs,
    double* delta_future,
    size_t n_books,
    int max_legs
) {
    srand(42);
    for (size_t i = 0; i < n_books; i++) {
        n_legs[i] = 3 + (rand() % (max_legs - 2));
        delta_future[i] = 0.8 + (rand() % 40) * 0.01;

        for (int j = 0; j < max_legs; j++) {
            leg_delta[i * max_legs + j] = -0.5 + (rand() % 100) * 0.01;
            leg_qty[i * max_legs + j] = -100.0 + (rand() % 200) * 1.0;
        }
    }
}

typedef struct {
    double* leg_delta;
    double* leg_qty;
    int* n_legs;
    double* delta_future;
    double* delta_net_out;
    double* hedge_qty_out;
    size_t n_books;
    int max_legs;
} bench_data_t;

static void bench_delta_aggregate_func(void* user_data) {
    bench_data_t* data = (bench_data_t*)user_data;

    fc_ex_strat_delta_aggregate(
        data->delta_net_out,
        data->hedge_qty_out,
        data->leg_delta,
        data->leg_qty,
        data->n_legs,
        data->delta_future,
        data->n_books,
        data->max_legs
    );
}

static void run_benchmark_for_size(size_t n_books, const char* size_label) {
    bench_data_t data;
    data.n_books = n_books;
    data.max_legs = MAX_LEGS;

    data.leg_delta = (double*)malloc(n_books * MAX_LEGS * sizeof(double));
    data.leg_qty = (double*)malloc(n_books * MAX_LEGS * sizeof(double));
    data.n_legs = (int*)malloc(n_books * sizeof(int));
    data.delta_future = (double*)malloc(n_books * sizeof(double));
    data.delta_net_out = (double*)malloc(n_books * sizeof(double));
    data.hedge_qty_out = (double*)malloc(n_books * sizeof(double));

    if (!data.leg_delta || !data.leg_qty || !data.n_legs ||
        !data.delta_future || !data.delta_net_out || !data.hedge_qty_out) {
        fprintf(stderr, "Failed to allocate memory for benchmark\n");
        goto cleanup;
    }

    setup_test_data(
        data.leg_delta,
        data.leg_qty,
        data.n_legs,
        data.delta_future,
        n_books,
        MAX_LEGS
    );

    char bench_name[256];
    snprintf(bench_name, sizeof(bench_name), "strat_delta_aggregate_%s", size_label);

    fc_bench_config_t config = FC_BENCH_CONFIG_DEFAULT;
    config.name = bench_name;
    config.data_size = n_books * MAX_LEGS * sizeof(double) * 2 +
                       n_books * sizeof(int) +
                       n_books * sizeof(double);
    config.min_time_ms = 200.0;

    fc_bench_result_t result;
    fc_bench_run(&config, bench_delta_aggregate_func, &data, &result);
    fc_bench_result_print(&result);

cleanup:
    free(data.leg_delta);
    free(data.leg_qty);
    free(data.n_legs);
    free(data.delta_future);
    free(data.delta_net_out);
    free(data.hedge_qty_out);
}

void bench_delta_hedge_run(void) {
    printf("=== Delta Hedging Strategy Benchmarks ===\n\n");

    fc_bench_print_header();

    printf("\n--- Small batch (10 books, max 10 legs) ---\n");
    run_benchmark_for_size(NUM_BOOKS_SMALL, "small");

    printf("\n--- Medium batch (100 books, max 10 legs) ---\n");
    run_benchmark_for_size(NUM_BOOKS_MEDIUM, "medium");

    printf("\n--- Large batch (1000 books, max 10 legs) ---\n");
    run_benchmark_for_size(NUM_BOOKS_LARGE, "large");
}
