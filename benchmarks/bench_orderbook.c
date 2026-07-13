/**
 * @file bench_orderbook.c
 * @brief Performance benchmarks for order book snapshot generation
 */

#include "bench_framework.h"
#include "orderbook.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NUM_SYMBOLS       5000
#define ORDERS_PER_SYMBOL 100
#define MAX_LEVELS        10

static fc_order_t* generate_test_orders(size_t num_orders, uint32_t num_symbols) {
    fc_order_t* orders = (fc_order_t*) malloc(num_orders * sizeof(fc_order_t));
    if (!orders)
        return NULL;

    srand(42);

    for (size_t i = 0; i < num_orders; i++) {
        orders[i].symbol_id = rand() % num_symbols;
        orders[i].side      = (rand() % 2) ? FC_ORDERBOOK_SIDE_BID : FC_ORDERBOOK_SIDE_ASK;

        double base_price = 100.0 + (orders[i].symbol_id % 100);
        if (orders[i].side == FC_ORDERBOOK_SIDE_BID) {
            orders[i].price = base_price - (rand() % 100) * 0.01;
        } else {
            orders[i].price = base_price + (rand() % 100) * 0.01;
        }

        orders[i].volume       = 10.0 + (rand() % 1000) * 0.1;
        orders[i].timestamp_ns = i * 1000;
    }

    return orders;
}

static fc_order_t* generate_sorted_orders(size_t num_orders) {
    fc_order_t* orders = (fc_order_t*) malloc(num_orders * sizeof(fc_order_t));
    if (!orders)
        return NULL;

    size_t half = num_orders >> 1;

    for (size_t i = 0; i < half; i++) {
        orders[i].symbol_id    = 0;
        orders[i].price        = 100.0 - i * 0.01;
        orders[i].volume       = 10.0 + i;
        orders[i].side         = FC_ORDERBOOK_SIDE_BID;
        orders[i].timestamp_ns = i;
    }

    for (size_t i = 0; i < half; i++) {
        orders[half + i].symbol_id    = 0;
        orders[half + i].price        = 100.5 + i * 0.01;
        orders[half + i].volume       = 10.0 + i;
        orders[half + i].side         = FC_ORDERBOOK_SIDE_ASK;
        orders[half + i].timestamp_ns = half + i;
    }

    return orders;
}

static fc_order_t* generate_unsorted_orders(size_t num_orders) {
    fc_order_t* orders = generate_sorted_orders(num_orders);
    if (!orders)
        return NULL;

    srand(42);
    for (size_t i = num_orders - 1; i > 0; i--) {
        size_t j        = rand() % (i + 1);
        fc_order_t temp = orders[i];
        orders[i]       = orders[j];
        orders[j]       = temp;
    }

    return orders;
}

static fc_order_t* generate_nearly_sorted_orders(size_t num_orders) {
    fc_order_t* orders = generate_sorted_orders(num_orders);
    if (!orders)
        return NULL;

    srand(42);
    size_t swaps = num_orders / 20;
    for (size_t i = 0; i < swaps; i++) {
        size_t idx1     = rand() % num_orders;
        size_t idx2     = rand() % num_orders;
        fc_order_t temp = orders[idx1];
        orders[idx1]    = orders[idx2];
        orders[idx2]    = temp;
    }

    return orders;
}

typedef struct {
    fc_order_t* orders;
    size_t num_orders;
    fc_price_level_t* levels;
    uint32_t* num_levels;
} bench_aggregate_data_t;

static void bench_aggregate_fn(void* user_data) {
    bench_aggregate_data_t* data = (bench_aggregate_data_t*) user_data;
    fc_orderbook_aggregate_levels(
        data->levels, data->num_levels, data->orders, data->num_orders, FC_ORDERBOOK_PRECISION_KAHAN
    );
}

static void bench_aggregate_levels(void) {
    fc_order_t orders[100];
    for (int i = 0; i < 100; i++) {
        orders[i].symbol_id    = 0;
        orders[i].price        = 100.0 - (i / 10) * 0.5;
        orders[i].volume       = 10.0;
        orders[i].side         = FC_ORDERBOOK_SIDE_BID;
        orders[i].timestamp_ns = 0;
    }

    fc_price_level_t levels[100];
    uint32_t num_levels;

    bench_aggregate_data_t data = {
        .orders = orders, .num_orders = 100, .levels = levels, .num_levels = &num_levels
    };

    fc_bench_config_t config = FC_BENCH_CONFIG_DEFAULT;
    config.name              = "aggregate_levels_100";
    config.data_size         = 100 * sizeof(fc_order_t);
    config.min_time_ms       = 100.0;

    fc_bench_result_t result;
    fc_bench_run(&config, bench_aggregate_fn, &data, &result);
    fc_bench_result_print(&result);
}

typedef struct {
    fc_order_t* orders;
    size_t num_orders;
    fc_orderbook_snapshot_t* snapshot;
} bench_snapshot_data_t;

static void bench_snapshot_fn(void* user_data) {
    bench_snapshot_data_t* data = (bench_snapshot_data_t*) user_data;
    fc_orderbook_snapshot_generate(
        data->snapshot, data->orders, data->num_orders, MAX_LEVELS, FC_ORDERBOOK_PRECISION_KAHAN, 0
    );
}

static void bench_snapshot_unsorted_fn(void* user_data) {
    bench_snapshot_data_t* data = (bench_snapshot_data_t*) user_data;
    fc_orderbook_snapshot_generate_unsorted(
        data->snapshot, data->orders, data->num_orders, MAX_LEVELS, FC_ORDERBOOK_PRECISION_KAHAN, 0
    );
}

static void bench_snapshot_single(void) {
    fc_order_t orders[100];
    for (int i = 0; i < 50; i++) {
        orders[i].symbol_id    = 0;
        orders[i].price        = 100.0 - i * 0.1;
        orders[i].volume       = 10.0 + i;
        orders[i].side         = FC_ORDERBOOK_SIDE_BID;
        orders[i].timestamp_ns = i;
    }
    for (int i = 0; i < 50; i++) {
        orders[50 + i].symbol_id    = 0;
        orders[50 + i].price        = 100.5 + i * 0.1;
        orders[50 + i].volume       = 10.0 + i;
        orders[50 + i].side         = FC_ORDERBOOK_SIDE_ASK;
        orders[50 + i].timestamp_ns = 50 + i;
    }

    fc_price_level_t bids[MAX_LEVELS];
    fc_price_level_t asks[MAX_LEVELS];
    fc_orderbook_snapshot_t snapshot = {.bids = bids, .asks = asks};

    bench_snapshot_data_t data = {.orders = orders, .num_orders = 100, .snapshot = &snapshot};

    fc_bench_config_t config = FC_BENCH_CONFIG_DEFAULT;
    config.name              = "snapshot_single_100";
    config.data_size         = 100 * sizeof(fc_order_t);
    config.min_time_ms       = 100.0;

    fc_bench_result_t result;
    fc_bench_run(&config, bench_snapshot_fn, &data, &result);
    fc_bench_result_print(&result);
}

static void bench_snapshot_comparison(size_t num_orders) {
    fc_order_t* sorted_orders        = generate_sorted_orders(num_orders);
    fc_order_t* unsorted_orders      = generate_unsorted_orders(num_orders);
    fc_order_t* nearly_sorted_orders = generate_nearly_sorted_orders(num_orders);

    if (!sorted_orders || !unsorted_orders || !nearly_sorted_orders) {
        printf("  Failed to allocate orders\n");
        free(sorted_orders);
        free(unsorted_orders);
        free(nearly_sorted_orders);
        return;
    }

    fc_price_level_t bids[MAX_LEVELS];
    fc_price_level_t asks[MAX_LEVELS];
    fc_orderbook_snapshot_t snapshot = {.bids = bids, .asks = asks};

    char name[64];
    fc_bench_config_t config = FC_BENCH_CONFIG_DEFAULT;
    config.data_size         = num_orders * sizeof(fc_order_t);
    config.min_time_ms       = 100.0;
    fc_bench_result_t result;

    snprintf(name, sizeof(name), "snapshot_sorted_%zu", num_orders);
    config.name                       = name;
    bench_snapshot_data_t sorted_data = {
        .orders = sorted_orders, .num_orders = num_orders, .snapshot = &snapshot
    };
    fc_bench_run(&config, bench_snapshot_fn, &sorted_data, &result);
    fc_bench_result_print(&result);

    snprintf(name, sizeof(name), "snapshot_unsorted_%zu", num_orders);
    config.name                         = name;
    bench_snapshot_data_t unsorted_data = {
        .orders = unsorted_orders, .num_orders = num_orders, .snapshot = &snapshot
    };
    fc_bench_run(&config, bench_snapshot_unsorted_fn, &unsorted_data, &result);
    fc_bench_result_print(&result);

    snprintf(name, sizeof(name), "snapshot_nearly_sorted_%zu", num_orders);
    config.name                              = name;
    bench_snapshot_data_t nearly_sorted_data = {
        .orders = nearly_sorted_orders, .num_orders = num_orders, .snapshot = &snapshot
    };
    fc_bench_run(&config, bench_snapshot_unsorted_fn, &nearly_sorted_data, &result);
    fc_bench_result_print(&result);

    free(sorted_orders);
    free(unsorted_orders);
    free(nearly_sorted_orders);
}

typedef struct {
    fc_order_t* orders;
    size_t num_orders;
    fc_orderbook_snapshot_t* snapshots;
    uint32_t num_symbols;
} bench_batch_data_t;

static void bench_batch_fn(void* user_data) {
    bench_batch_data_t* data = (bench_batch_data_t*) user_data;
    fc_orderbook_snapshot_generate_batch(
        data->snapshots,
        data->orders,
        data->num_orders,
        data->num_symbols,
        MAX_LEVELS,
        FC_ORDERBOOK_PRECISION_KAHAN,
        0
    );
}

static void bench_snapshot_batch(uint32_t num_symbols, size_t orders_per_symbol) {
    size_t total_orders = num_symbols * orders_per_symbol;
    fc_order_t* orders  = generate_test_orders(total_orders, num_symbols);
    if (!orders) {
        printf("  Failed to allocate orders\n");
        return;
    }

    fc_orderbook_snapshot_t* snapshots =
        (fc_orderbook_snapshot_t*) malloc(num_symbols * sizeof(fc_orderbook_snapshot_t));
    if (!snapshots) {
        free(orders);
        return;
    }

    for (uint32_t i = 0; i < num_symbols; i++) {
        snapshots[i].bids = (fc_price_level_t*) malloc(MAX_LEVELS * sizeof(fc_price_level_t));
        snapshots[i].asks = (fc_price_level_t*) malloc(MAX_LEVELS * sizeof(fc_price_level_t));
    }

    bench_batch_data_t data = {
        .orders      = orders,
        .num_orders  = total_orders,
        .snapshots   = snapshots,
        .num_symbols = num_symbols
    };

    char name[64];
    snprintf(name, sizeof(name), "snapshot_batch_%u_symbols", num_symbols);

    fc_bench_config_t config = FC_BENCH_CONFIG_DEFAULT;
    config.name              = name;
    config.data_size         = total_orders * sizeof(fc_order_t);
    config.min_time_ms       = (num_symbols >= 1000) ? 100.0 : 200.0;
    config.min_iterations    = (num_symbols >= 1000) ? 10 : 100;

    fc_bench_result_t result;
    fc_bench_run(&config, bench_batch_fn, &data, &result);
    fc_bench_result_print(&result);

    for (uint32_t i = 0; i < num_symbols; i++) {
        free(snapshots[i].bids);
        free(snapshots[i].asks);
    }
    free(snapshots);
    free(orders);
}

void bench_order_book_run(void) {
    printf("\n");
    printf("============================================================\n");
    printf("  Order Book Snapshot Generation Benchmarks\n");
    printf("============================================================\n");
    printf("\n");

    fc_bench_print_header();

    printf("\n--- Basic Operations ---\n");
    bench_aggregate_levels();
    bench_snapshot_single();

    printf("\n--- Sorted vs Unsorted Comparison ---\n");
    bench_snapshot_comparison(100);
    bench_snapshot_comparison(500);
    bench_snapshot_comparison(1000);
    bench_snapshot_comparison(5000);

    printf("\n--- Batch Processing ---\n");
    bench_snapshot_batch(100, 20);
    bench_snapshot_batch(1000, 50);
    bench_snapshot_batch(5000, 100);

    printf("\n");
    printf("Performance targets:\n");
    printf("  Single symbol snapshot: < 10 μs\n");
    printf("  Full market (5000 symbols): < 50 ms\n");
    printf("\n");
    printf("Notes:\n");
    printf("  - sorted: Pre-sorted orders (baseline)\n");
    printf("  - unsorted: Completely random order (worst case)\n");
    printf("  - nearly_sorted: 95%% sorted (Timsort advantage)\n");
    printf("\n");
}
