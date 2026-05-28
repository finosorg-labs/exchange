/**
 * @file test_orderbook.c
 * @brief Unit tests for order book snapshot generation
 */

#include "test_framework.h"
#include "orderbook.h"
#include "error.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define EPSILON 1e-10

TEST(test_aggregate_levels_basic) {
    fc_order_t orders[] = {
        {0, 100.0, 10.0, FC_ORDERBOOK_SIDE_BID, 0},
        {0, 100.0, 20.0, FC_ORDERBOOK_SIDE_BID, 0},
        {0, 99.5, 15.0, FC_ORDERBOOK_SIDE_BID, 0},
        {0, 99.5, 25.0, FC_ORDERBOOK_SIDE_BID, 0},
        {0, 99.0, 30.0, FC_ORDERBOOK_SIDE_BID, 0},
    };

    fc_price_level_t levels[10];
    uint32_t num_levels;

    fc_status_t status = fc_orderbook_aggregate_levels(
        levels, &num_levels, orders, 5, FC_ORDERBOOK_PRECISION_STANDARD
    );

    ASSERT_EQ(status, FC_OK);
    ASSERT_EQ(num_levels, 3);
    FC_TEST_ASSERT_DOUBLE_EQ(levels[0].price, 100.0, EPSILON);
    FC_TEST_ASSERT_DOUBLE_EQ(levels[0].volume, 30.0, EPSILON);
    FC_TEST_ASSERT_DOUBLE_EQ(levels[1].price, 99.5, EPSILON);
    FC_TEST_ASSERT_DOUBLE_EQ(levels[1].volume, 40.0, EPSILON);
    FC_TEST_ASSERT_DOUBLE_EQ(levels[2].price, 99.0, EPSILON);
    FC_TEST_ASSERT_DOUBLE_EQ(levels[2].volume, 30.0, EPSILON);
}

TEST(test_aggregate_levels_kahan) {
    fc_order_t orders[1000];
    for (int i = 0; i < 1000; i++) {
        orders[i].symbol_id = 0;
        orders[i].price = 100.0;
        orders[i].volume = 0.1;
        orders[i].side = FC_ORDERBOOK_SIDE_BID;
        orders[i].timestamp_ns = 0;
    }

    fc_price_level_t levels[10];
    uint32_t num_levels;

    fc_status_t status = fc_orderbook_aggregate_levels(
        levels, &num_levels, orders, 1000, FC_ORDERBOOK_PRECISION_KAHAN
    );

    ASSERT_EQ(status, FC_OK);
    ASSERT_EQ(num_levels, 1);
    FC_TEST_ASSERT_DOUBLE_EQ(levels[0].price, 100.0, EPSILON);
    FC_TEST_ASSERT_DOUBLE_EQ(levels[0].volume, 100.0, 1e-6);
}

TEST(test_calculate_metrics) {
    fc_price_level_t bids[5] = {
        {100.0, 50.0},
        {99.5, 30.0},
        {99.0, 20.0},
    };

    fc_price_level_t asks[5] = {
        {100.5, 40.0},
        {101.0, 25.0},
        {101.5, 15.0},
    };

    fc_orderbook_snapshot_t snapshot = {
        .bids = bids,
        .asks = asks,
        .num_bid_levels = 3,
        .num_ask_levels = 3,
        .symbol_id = 0,
        .timestamp_ns = 0,
    };

    fc_status_t status = fc_orderbook_calculate_metrics(&snapshot);

    ASSERT_EQ(status, FC_OK);
    FC_TEST_ASSERT_DOUBLE_EQ(snapshot.spread, 0.5, EPSILON);
    FC_TEST_ASSERT_DOUBLE_EQ(snapshot.mid_price, 100.25, EPSILON);

    double expected_weighted = (100.0 * 40.0 + 100.5 * 50.0) / (50.0 + 40.0);
    FC_TEST_ASSERT_DOUBLE_EQ(snapshot.weighted_mid_price, expected_weighted, EPSILON);
}

TEST(test_snapshot_generate_single_symbol) {
    fc_order_t orders[] = {
        {0, 100.0, 10.0, FC_ORDERBOOK_SIDE_BID, 0},
        {0, 100.0, 20.0, FC_ORDERBOOK_SIDE_BID, 0},
        {0, 99.5, 15.0, FC_ORDERBOOK_SIDE_BID, 0},
        {0, 100.5, 12.0, FC_ORDERBOOK_SIDE_ASK, 0},
        {0, 100.5, 18.0, FC_ORDERBOOK_SIDE_ASK, 0},
        {0, 101.0, 25.0, FC_ORDERBOOK_SIDE_ASK, 0},
    };

    fc_price_level_t bids[10];
    fc_price_level_t asks[10];

    fc_orderbook_snapshot_t snapshot = {
        .bids = bids,
        .asks = asks,
    };

    fc_status_t status = fc_orderbook_snapshot_generate(
        &snapshot, orders, 6, 10, FC_ORDERBOOK_PRECISION_KAHAN, 1000000000LL
    );

    ASSERT_EQ(status, FC_OK);
    ASSERT_EQ(snapshot.symbol_id, 0);
    ASSERT_EQ(snapshot.timestamp_ns, 1000000000LL);
    ASSERT_EQ(snapshot.num_bid_levels, 2);
    ASSERT_EQ(snapshot.num_ask_levels, 2);

    FC_TEST_ASSERT_DOUBLE_EQ(snapshot.bids[0].price, 100.0, EPSILON);
    FC_TEST_ASSERT_DOUBLE_EQ(snapshot.bids[0].volume, 30.0, EPSILON);
    FC_TEST_ASSERT_DOUBLE_EQ(snapshot.asks[0].price, 100.5, EPSILON);
    FC_TEST_ASSERT_DOUBLE_EQ(snapshot.asks[0].volume, 30.0, EPSILON);

    FC_TEST_ASSERT_DOUBLE_EQ(snapshot.spread, 0.5, EPSILON);
}

TEST(test_snapshot_generate_max_levels) {
    fc_order_t orders[20];
    for (int i = 0; i < 10; i++) {
        orders[i].symbol_id = 0;
        orders[i].price = 100.0 - i * 0.5;
        orders[i].volume = 10.0;
        orders[i].side = FC_ORDERBOOK_SIDE_BID;
        orders[i].timestamp_ns = 0;
    }
    for (int i = 0; i < 10; i++) {
        orders[10 + i].symbol_id = 0;
        orders[10 + i].price = 100.5 + i * 0.5;
        orders[10 + i].volume = 10.0;
        orders[10 + i].side = FC_ORDERBOOK_SIDE_ASK;
        orders[10 + i].timestamp_ns = 0;
    }

    fc_price_level_t bids[5];
    fc_price_level_t asks[5];

    fc_orderbook_snapshot_t snapshot = {
        .bids = bids,
        .asks = asks,
    };

    fc_status_t status =
        fc_orderbook_snapshot_generate(&snapshot, orders, 20, 5, FC_ORDERBOOK_PRECISION_STANDARD, 0);

    ASSERT_EQ(status, FC_OK);
    ASSERT_EQ(snapshot.num_bid_levels, 5);
    ASSERT_EQ(snapshot.num_ask_levels, 5);
}

TEST(test_snapshot_generate_empty) {
    fc_price_level_t bids[10];
    fc_price_level_t asks[10];

    fc_orderbook_snapshot_t snapshot = {
        .bids = bids,
        .asks = asks,
    };

    fc_status_t status =
        fc_orderbook_snapshot_generate(&snapshot, NULL, 0, 10, FC_ORDERBOOK_PRECISION_STANDARD, 0);

    ASSERT_EQ(status, FC_OK);
    ASSERT_EQ(snapshot.num_bid_levels, 0);
    ASSERT_EQ(snapshot.num_ask_levels, 0);
    FC_TEST_ASSERT_DOUBLE_EQ(snapshot.spread, 0.0, EPSILON);
}

TEST(test_snapshot_generate_batch) {
    fc_order_t orders[] = {
        {0, 100.0, 10.0, FC_ORDERBOOK_SIDE_BID, 0},
        {0, 100.5, 12.0, FC_ORDERBOOK_SIDE_ASK, 0},
        {1, 200.0, 20.0, FC_ORDERBOOK_SIDE_BID, 0},
        {1, 200.5, 22.0, FC_ORDERBOOK_SIDE_ASK, 0},
        {2, 300.0, 30.0, FC_ORDERBOOK_SIDE_BID, 0},
        {2, 300.5, 32.0, FC_ORDERBOOK_SIDE_ASK, 0},
    };

    fc_price_level_t bids0[10], asks0[10];
    fc_price_level_t bids1[10], asks1[10];
    fc_price_level_t bids2[10], asks2[10];

    fc_orderbook_snapshot_t snapshots[3] = {
        {.bids = bids0, .asks = asks0},
        {.bids = bids1, .asks = asks1},
        {.bids = bids2, .asks = asks2},
    };

    fc_status_t status = fc_orderbook_snapshot_generate_batch(
        snapshots, orders, 6, 3, 10, FC_ORDERBOOK_PRECISION_STANDARD, 1000000000LL
    );

    ASSERT_EQ(status, FC_OK);

    ASSERT_EQ(snapshots[0].symbol_id, 0);
    ASSERT_EQ(snapshots[0].num_bid_levels, 1);
    ASSERT_EQ(snapshots[0].num_ask_levels, 1);
    FC_TEST_ASSERT_DOUBLE_EQ(snapshots[0].bids[0].price, 100.0, EPSILON);

    ASSERT_EQ(snapshots[1].symbol_id, 1);
    ASSERT_EQ(snapshots[1].num_bid_levels, 1);
    FC_TEST_ASSERT_DOUBLE_EQ(snapshots[1].bids[0].price, 200.0, EPSILON);

    ASSERT_EQ(snapshots[2].symbol_id, 2);
    ASSERT_EQ(snapshots[2].num_bid_levels, 1);
    FC_TEST_ASSERT_DOUBLE_EQ(snapshots[2].bids[0].price, 300.0, EPSILON);
}

TEST(test_invalid_arguments) {
    fc_price_level_t levels[10];
    uint32_t num_levels;

    fc_status_t status =
        fc_orderbook_aggregate_levels(NULL, &num_levels, NULL, 0, FC_ORDERBOOK_PRECISION_STANDARD);
    ASSERT_EQ(status, FC_ERR_INVALID_ARG);

    status = fc_orderbook_aggregate_levels(levels, NULL, NULL, 0, FC_ORDERBOOK_PRECISION_STANDARD);
    ASSERT_EQ(status, FC_ERR_INVALID_ARG);

    status = fc_orderbook_calculate_metrics(NULL);
    ASSERT_EQ(status, FC_ERR_INVALID_ARG);
}

TEST(test_snapshot_generate_unsorted) {
    // Create unsorted orders (mixed bids and asks, random price order)
    fc_order_t orders[] = {
        {0, 100.5, 40.0, FC_ORDERBOOK_SIDE_ASK, 1000},
        {0, 100.0, 10.0, FC_ORDERBOOK_SIDE_BID, 2000},
        {0, 101.0, 25.0, FC_ORDERBOOK_SIDE_ASK, 3000},
        {0, 99.5, 30.0, FC_ORDERBOOK_SIDE_BID, 4000},
        {0, 100.0, 20.0, FC_ORDERBOOK_SIDE_BID, 5000},
        {0, 99.0, 20.0, FC_ORDERBOOK_SIDE_BID, 6000},
        {0, 101.5, 15.0, FC_ORDERBOOK_SIDE_ASK, 7000},
    };

    fc_price_level_t bids[10];
    fc_price_level_t asks[10];

    fc_orderbook_snapshot_t snapshot = {
        .bids = bids,
        .asks = asks,
        .symbol_id = 0,
        .timestamp_ns = 0,
    };

    // Call unsorted version (will sort in-place)
    fc_status_t status = fc_orderbook_snapshot_generate_unsorted(
        &snapshot, orders, 7, 10, FC_ORDERBOOK_PRECISION_STANDARD, 123456789
    );

    ASSERT_EQ(status, FC_OK);
    ASSERT_EQ(snapshot.symbol_id, 0);
    ASSERT_EQ(snapshot.timestamp_ns, 123456789);

    // Verify bid levels (should be sorted descending by price)
    ASSERT_EQ(snapshot.num_bid_levels, 3);
    FC_TEST_ASSERT_DOUBLE_EQ(snapshot.bids[0].price, 100.0, EPSILON);
    FC_TEST_ASSERT_DOUBLE_EQ(snapshot.bids[0].volume, 30.0, EPSILON);  // 10 + 20
    FC_TEST_ASSERT_DOUBLE_EQ(snapshot.bids[1].price, 99.5, EPSILON);
    FC_TEST_ASSERT_DOUBLE_EQ(snapshot.bids[1].volume, 30.0, EPSILON);
    FC_TEST_ASSERT_DOUBLE_EQ(snapshot.bids[2].price, 99.0, EPSILON);
    FC_TEST_ASSERT_DOUBLE_EQ(snapshot.bids[2].volume, 20.0, EPSILON);

    // Verify ask levels (should be sorted ascending by price)
    ASSERT_EQ(snapshot.num_ask_levels, 3);
    FC_TEST_ASSERT_DOUBLE_EQ(snapshot.asks[0].price, 100.5, EPSILON);
    FC_TEST_ASSERT_DOUBLE_EQ(snapshot.asks[0].volume, 40.0, EPSILON);
    FC_TEST_ASSERT_DOUBLE_EQ(snapshot.asks[1].price, 101.0, EPSILON);
    FC_TEST_ASSERT_DOUBLE_EQ(snapshot.asks[1].volume, 25.0, EPSILON);
    FC_TEST_ASSERT_DOUBLE_EQ(snapshot.asks[2].price, 101.5, EPSILON);
    FC_TEST_ASSERT_DOUBLE_EQ(snapshot.asks[2].volume, 15.0, EPSILON);

    // Verify metrics
    FC_TEST_ASSERT_DOUBLE_EQ(snapshot.spread, 0.5, EPSILON);
    FC_TEST_ASSERT_DOUBLE_EQ(snapshot.mid_price, 100.25, EPSILON);

    double expected_weighted = (100.0 * 40.0 + 100.5 * 30.0) / (30.0 + 40.0);
    FC_TEST_ASSERT_DOUBLE_EQ(snapshot.weighted_mid_price, expected_weighted, EPSILON);
}

TEST(test_snapshot_generate_comparison) {
    // Compare sorted vs unsorted versions with same input
    fc_order_t sorted_orders[] = {
        // Bids (descending by price)
        {0, 100.0, 30.0, FC_ORDERBOOK_SIDE_BID, 1000},
        {0, 99.5, 30.0, FC_ORDERBOOK_SIDE_BID, 2000},
        {0, 99.0, 20.0, FC_ORDERBOOK_SIDE_BID, 3000},
        // Asks (ascending by price)
        {0, 100.5, 40.0, FC_ORDERBOOK_SIDE_ASK, 4000},
        {0, 101.0, 25.0, FC_ORDERBOOK_SIDE_ASK, 5000},
        {0, 101.5, 15.0, FC_ORDERBOOK_SIDE_ASK, 6000},
    };

    fc_order_t unsorted_orders[] = {
        {0, 100.5, 40.0, FC_ORDERBOOK_SIDE_ASK, 4000},
        {0, 100.0, 30.0, FC_ORDERBOOK_SIDE_BID, 1000},
        {0, 101.0, 25.0, FC_ORDERBOOK_SIDE_ASK, 5000},
        {0, 99.5, 30.0, FC_ORDERBOOK_SIDE_BID, 2000},
        {0, 99.0, 20.0, FC_ORDERBOOK_SIDE_BID, 3000},
        {0, 101.5, 15.0, FC_ORDERBOOK_SIDE_ASK, 6000},
    };

    fc_price_level_t bids1[10], asks1[10];
    fc_price_level_t bids2[10], asks2[10];

    fc_orderbook_snapshot_t snapshot1 = {.bids = bids1, .asks = asks1};
    fc_orderbook_snapshot_t snapshot2 = {.bids = bids2, .asks = asks2};

    // Generate using sorted version
    fc_status_t status1 = fc_orderbook_snapshot_generate(
        &snapshot1, sorted_orders, 6, 10, FC_ORDERBOOK_PRECISION_STANDARD, 0
    );

    // Generate using unsorted version
    fc_status_t status2 = fc_orderbook_snapshot_generate_unsorted(
        &snapshot2, unsorted_orders, 6, 10, FC_ORDERBOOK_PRECISION_STANDARD, 0
    );

    ASSERT_EQ(status1, FC_OK);
    ASSERT_EQ(status2, FC_OK);

    // Results should be identical
    ASSERT_EQ(snapshot1.num_bid_levels, snapshot2.num_bid_levels);
    ASSERT_EQ(snapshot1.num_ask_levels, snapshot2.num_ask_levels);

    for (uint32_t i = 0; i < snapshot1.num_bid_levels; i++) {
        FC_TEST_ASSERT_DOUBLE_EQ(snapshot1.bids[i].price, snapshot2.bids[i].price, EPSILON);
        FC_TEST_ASSERT_DOUBLE_EQ(snapshot1.bids[i].volume, snapshot2.bids[i].volume, EPSILON);
    }

    for (uint32_t i = 0; i < snapshot1.num_ask_levels; i++) {
        FC_TEST_ASSERT_DOUBLE_EQ(snapshot1.asks[i].price, snapshot2.asks[i].price, EPSILON);
        FC_TEST_ASSERT_DOUBLE_EQ(snapshot1.asks[i].volume, snapshot2.asks[i].volume, EPSILON);
    }

    FC_TEST_ASSERT_DOUBLE_EQ(snapshot1.spread, snapshot2.spread, EPSILON);
    FC_TEST_ASSERT_DOUBLE_EQ(snapshot1.mid_price, snapshot2.mid_price, EPSILON);
    FC_TEST_ASSERT_DOUBLE_EQ(snapshot1.weighted_mid_price, snapshot2.weighted_mid_price, EPSILON);
}

void register_order_book_tests(void) {
    RUN_TEST(test_aggregate_levels_basic);
    RUN_TEST(test_aggregate_levels_kahan);
    RUN_TEST(test_calculate_metrics);
    RUN_TEST(test_snapshot_generate_single_symbol);
    RUN_TEST(test_snapshot_generate_max_levels);
    RUN_TEST(test_snapshot_generate_empty);
    RUN_TEST(test_snapshot_generate_batch);
    RUN_TEST(test_invalid_arguments);
    RUN_TEST(test_snapshot_generate_unsorted);
    RUN_TEST(test_snapshot_generate_comparison);
}
