/**
 * @file test_index_rebal.c
 * @brief Unit tests for index rebalancing strategy
 */

#include "platform.h"
#include "strategy/index_rebal.h"
#include "test_framework.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define EPSILON 1e-9

TEST(test_index_nav_single_index_basic) {
    double prices[]      = {100.0, 200.0, 50.0};
    double weights[]     = {0.5, 0.3, 0.2};
    int n_constituents[] = {3};
    double nav_out[1]    = {0.0};

    fc_status_t status =
        fc_ex_strat_index_nav(nav_out, NULL, prices, weights, NULL, NULL, n_constituents, 1, 3);

    ASSERT_EQ(status, FC_OK);

    double expected_nav = 100.0 * 0.5 + 200.0 * 0.3 + 50.0 * 0.2;
    FC_TEST_ASSERT_DOUBLE_EQ(nav_out[0], expected_nav, EPSILON);
}

TEST(test_index_nav_multiple_indices) {
    double prices[]      = {100.0, 200.0, 50.0, 150.0, 300.0, 0.0};
    double weights[]     = {0.5, 0.3, 0.2, 0.6, 0.4, 0.0};
    int n_constituents[] = {3, 2};
    double nav_out[2]    = {0.0};

    fc_status_t status =
        fc_ex_strat_index_nav(nav_out, NULL, prices, weights, NULL, NULL, n_constituents, 2, 3);

    ASSERT_EQ(status, FC_OK);

    double expected_nav0 = 100.0 * 0.5 + 200.0 * 0.3 + 50.0 * 0.2;
    double expected_nav1 = 150.0 * 0.6 + 300.0 * 0.4;

    FC_TEST_ASSERT_DOUBLE_EQ(nav_out[0], expected_nav0, EPSILON);
    FC_TEST_ASSERT_DOUBLE_EQ(nav_out[1], expected_nav1, EPSILON);
}

TEST(test_index_nav_with_rebalancing) {
    double prices[]         = {100.0, 200.0, 50.0};
    double weights[]        = {0.5, 0.3, 0.2};
    double current_qty[]    = {0.0, 0.0, 0.0};
    double tracking_aum[]   = {10000.0};
    int n_constituents[]    = {3};
    double nav_out[1]       = {0.0};
    double rebal_qty_out[3] = {0.0};

    fc_status_t status = fc_ex_strat_index_nav(
        nav_out, rebal_qty_out, prices, weights, current_qty, tracking_aum, n_constituents, 1, 3
    );

    ASSERT_EQ(status, FC_OK);

    double expected_target0 = (10000.0 * 0.5) / 100.0;
    double expected_target1 = (10000.0 * 0.3) / 200.0;
    double expected_target2 = (10000.0 * 0.2) / 50.0;

    FC_TEST_ASSERT_DOUBLE_EQ(rebal_qty_out[0], expected_target0, EPSILON);
    FC_TEST_ASSERT_DOUBLE_EQ(rebal_qty_out[1], expected_target1, EPSILON);
    FC_TEST_ASSERT_DOUBLE_EQ(rebal_qty_out[2], expected_target2, EPSILON);
}

TEST(test_index_nav_rebalancing_with_existing_positions) {
    double prices[]         = {100.0, 200.0};
    double weights[]        = {0.6, 0.4};
    double current_qty[]    = {30.0, 10.0};
    double tracking_aum[]   = {10000.0};
    int n_constituents[]    = {2};
    double nav_out[1]       = {0.0};
    double rebal_qty_out[2] = {0.0};

    fc_status_t status = fc_ex_strat_index_nav(
        nav_out, rebal_qty_out, prices, weights, current_qty, tracking_aum, n_constituents, 1, 2
    );

    ASSERT_EQ(status, FC_OK);

    double expected_target0 = (10000.0 * 0.6) / 100.0;
    double expected_target1 = (10000.0 * 0.4) / 200.0;
    double expected_rebal0  = expected_target0 - 30.0;
    double expected_rebal1  = expected_target1 - 10.0;

    FC_TEST_ASSERT_DOUBLE_EQ(rebal_qty_out[0], expected_rebal0, EPSILON);
    FC_TEST_ASSERT_DOUBLE_EQ(rebal_qty_out[1], expected_rebal1, EPSILON);
}

TEST(test_index_nav_multiple_indices_with_rebalancing) {
    double prices[]         = {100.0, 200.0, 150.0, 300.0};
    double weights[]        = {0.6, 0.4, 0.5, 0.5};
    double current_qty[]    = {0.0, 0.0, 0.0, 0.0};
    double tracking_aum[]   = {10000.0, 20000.0};
    int n_constituents[]    = {2, 2};
    double nav_out[2]       = {0.0};
    double rebal_qty_out[4] = {0.0};

    fc_status_t status = fc_ex_strat_index_nav(
        nav_out, rebal_qty_out, prices, weights, current_qty, tracking_aum, n_constituents, 2, 2
    );

    ASSERT_EQ(status, FC_OK);

    double expected_target0_0 = (10000.0 * 0.6) / 100.0;
    double expected_target0_1 = (10000.0 * 0.4) / 200.0;
    double expected_target1_0 = (20000.0 * 0.5) / 150.0;
    double expected_target1_1 = (20000.0 * 0.5) / 300.0;

    FC_TEST_ASSERT_DOUBLE_EQ(rebal_qty_out[0], expected_target0_0, EPSILON);
    FC_TEST_ASSERT_DOUBLE_EQ(rebal_qty_out[1], expected_target0_1, EPSILON);
    FC_TEST_ASSERT_DOUBLE_EQ(rebal_qty_out[2], expected_target1_0, EPSILON);
    FC_TEST_ASSERT_DOUBLE_EQ(rebal_qty_out[3], expected_target1_1, EPSILON);
}

TEST(test_index_nav_variable_constituents) {
    double prices[]      = {100.0, 200.0, 0.0, 150.0, 300.0, 450.0};
    double weights[]     = {0.7, 0.3, 0.0, 0.4, 0.35, 0.25};
    int n_constituents[] = {2, 3};
    double nav_out[2]    = {0.0};

    fc_status_t status =
        fc_ex_strat_index_nav(nav_out, NULL, prices, weights, NULL, NULL, n_constituents, 2, 3);

    ASSERT_EQ(status, FC_OK);

    double expected_nav0 = 100.0 * 0.7 + 200.0 * 0.3;
    double expected_nav1 = 150.0 * 0.4 + 300.0 * 0.35 + 450.0 * 0.25;

    FC_TEST_ASSERT_DOUBLE_EQ(nav_out[0], expected_nav0, EPSILON);
    FC_TEST_ASSERT_DOUBLE_EQ(nav_out[1], expected_nav1, EPSILON);
}

TEST(test_index_nav_kahan_summation_precision) {
    double prices[100];
    double weights[100];
    int n_constituents[] = {100};
    double nav_out[1]    = {0.0};

    for (int i = 0; i < 100; i++) {
        prices[i]  = 0.01;
        weights[i] = 0.01;
    }

    fc_status_t status =
        fc_ex_strat_index_nav(nav_out, NULL, prices, weights, NULL, NULL, n_constituents, 1, 100);

    ASSERT_EQ(status, FC_OK);

    double expected_nav = 0.01 * 0.01 * 100.0;
    FC_TEST_ASSERT_DOUBLE_EQ(nav_out[0], expected_nav, 1e-12);
}

TEST(test_index_nav_large_batch) {
    const size_t n_index    = 100;
    const int max_const     = 50;
    const size_t total_size = n_index * max_const;

    double* prices      = (double*) malloc(total_size * sizeof(double));
    double* weights     = (double*) malloc(total_size * sizeof(double));
    int* n_constituents = (int*) malloc(n_index * sizeof(int));
    double* nav_out     = (double*) calloc(n_index, sizeof(double));

    if (!prices || !weights || !n_constituents || !nav_out) {
        free(prices);
        free(weights);
        free(n_constituents);
        free(nav_out);
        ASSERT_NOT_NULL(NULL);
    }

    for (size_t i = 0; i < n_index; i++) {
        n_constituents[i] = 10;
        for (int j = 0; j < max_const; j++) {
            size_t idx = i * max_const + j;
            if (j < 10) {
                prices[idx]  = 100.0 + j * 10.0;
                weights[idx] = 0.1;
            } else {
                prices[idx]  = 0.0;
                weights[idx] = 0.0;
            }
        }
    }

    fc_status_t status = fc_ex_strat_index_nav(
        nav_out, NULL, prices, weights, NULL, NULL, n_constituents, n_index, max_const
    );

    ASSERT_EQ(status, FC_OK);

    for (size_t i = 0; i < n_index; i++) {
        ASSERT_TRUE(nav_out[i] > 0.0);
    }

    free(prices);
    free(weights);
    free(n_constituents);
    free(nav_out);
}

TEST(test_index_nav_invalid_null_nav_out) {
    double prices[]      = {100.0};
    double weights[]     = {1.0};
    int n_constituents[] = {1};

    fc_status_t status =
        fc_ex_strat_index_nav(NULL, NULL, prices, weights, NULL, NULL, n_constituents, 1, 1);

    ASSERT_EQ(status, FC_ERR_INVALID_ARG);
}

TEST(test_index_nav_invalid_null_prices) {
    double weights[]     = {1.0};
    int n_constituents[] = {1};
    double nav_out[1]    = {0.0};

    fc_status_t status =
        fc_ex_strat_index_nav(nav_out, NULL, NULL, weights, NULL, NULL, n_constituents, 1, 1);

    ASSERT_EQ(status, FC_ERR_INVALID_ARG);
}

TEST(test_index_nav_invalid_null_weights) {
    double prices[]      = {100.0};
    int n_constituents[] = {1};
    double nav_out[1]    = {0.0};

    fc_status_t status =
        fc_ex_strat_index_nav(nav_out, NULL, prices, NULL, NULL, NULL, n_constituents, 1, 1);

    ASSERT_EQ(status, FC_ERR_INVALID_ARG);
}

TEST(test_index_nav_invalid_null_n_constituents) {
    double prices[]   = {100.0};
    double weights[]  = {1.0};
    double nav_out[1] = {0.0};

    fc_status_t status =
        fc_ex_strat_index_nav(nav_out, NULL, prices, weights, NULL, NULL, NULL, 1, 1);

    ASSERT_EQ(status, FC_ERR_INVALID_ARG);
}

TEST(test_index_nav_invalid_zero_n_index) {
    double prices[]      = {100.0};
    double weights[]     = {1.0};
    int n_constituents[] = {1};
    double nav_out[1]    = {0.0};

    fc_status_t status =
        fc_ex_strat_index_nav(nav_out, NULL, prices, weights, NULL, NULL, n_constituents, 0, 1);

    ASSERT_EQ(status, FC_ERR_INVALID_ARG);
}

TEST(test_index_nav_invalid_zero_max_constituents) {
    double prices[]      = {100.0};
    double weights[]     = {1.0};
    int n_constituents[] = {1};
    double nav_out[1]    = {0.0};

    fc_status_t status =
        fc_ex_strat_index_nav(nav_out, NULL, prices, weights, NULL, NULL, n_constituents, 1, 0);

    ASSERT_EQ(status, FC_ERR_INVALID_ARG);
}

TEST(test_index_nav_invalid_negative_n_constituents) {
    double prices[]      = {100.0};
    double weights[]     = {1.0};
    int n_constituents[] = {-1};
    double nav_out[1]    = {0.0};

    fc_status_t status =
        fc_ex_strat_index_nav(nav_out, NULL, prices, weights, NULL, NULL, n_constituents, 1, 1);

    ASSERT_EQ(status, FC_ERR_INVALID_ARG);
}

TEST(test_index_nav_invalid_n_constituents_exceeds_max) {
    double prices[]      = {100.0, 200.0};
    double weights[]     = {0.5, 0.5};
    int n_constituents[] = {3};
    double nav_out[1]    = {0.0};

    fc_status_t status =
        fc_ex_strat_index_nav(nav_out, NULL, prices, weights, NULL, NULL, n_constituents, 1, 2);

    ASSERT_EQ(status, FC_ERR_INVALID_ARG);
}

TEST(test_index_nav_invalid_rebal_missing_current_qty) {
    double prices[]         = {100.0};
    double weights[]        = {1.0};
    double tracking_aum[]   = {10000.0};
    int n_constituents[]    = {1};
    double nav_out[1]       = {0.0};
    double rebal_qty_out[1] = {0.0};

    fc_status_t status = fc_ex_strat_index_nav(
        nav_out, rebal_qty_out, prices, weights, NULL, tracking_aum, n_constituents, 1, 1
    );

    ASSERT_EQ(status, FC_ERR_INVALID_ARG);
}

TEST(test_index_nav_invalid_rebal_missing_tracking_aum) {
    double prices[]         = {100.0};
    double weights[]        = {1.0};
    double current_qty[]    = {0.0};
    int n_constituents[]    = {1};
    double nav_out[1]       = {0.0};
    double rebal_qty_out[1] = {0.0};

    fc_status_t status = fc_ex_strat_index_nav(
        nav_out, rebal_qty_out, prices, weights, current_qty, NULL, n_constituents, 1, 1
    );

    ASSERT_EQ(status, FC_ERR_INVALID_ARG);
}

TEST(test_index_nav_invalid_zero_price_with_rebalancing) {
    double prices[]         = {0.0};
    double weights[]        = {1.0};
    double current_qty[]    = {0.0};
    double tracking_aum[]   = {10000.0};
    int n_constituents[]    = {1};
    double nav_out[1]       = {0.0};
    double rebal_qty_out[1] = {0.0};

    fc_status_t status = fc_ex_strat_index_nav(
        nav_out, rebal_qty_out, prices, weights, current_qty, tracking_aum, n_constituents, 1, 1
    );

    ASSERT_EQ(status, FC_ERR_INVALID_ARG);
}

TEST(test_index_nav_invalid_negative_price_with_rebalancing) {
    double prices[]         = {-100.0};
    double weights[]        = {1.0};
    double current_qty[]    = {0.0};
    double tracking_aum[]   = {10000.0};
    int n_constituents[]    = {1};
    double nav_out[1]       = {0.0};
    double rebal_qty_out[1] = {0.0};

    fc_status_t status = fc_ex_strat_index_nav(
        nav_out, rebal_qty_out, prices, weights, current_qty, tracking_aum, n_constituents, 1, 1
    );

    ASSERT_EQ(status, FC_ERR_INVALID_ARG);
}

void register_index_rebal_tests(void) {
    RUN_TEST(test_index_nav_single_index_basic);
    RUN_TEST(test_index_nav_multiple_indices);
    RUN_TEST(test_index_nav_with_rebalancing);
    RUN_TEST(test_index_nav_rebalancing_with_existing_positions);
    RUN_TEST(test_index_nav_multiple_indices_with_rebalancing);
    RUN_TEST(test_index_nav_variable_constituents);
    RUN_TEST(test_index_nav_kahan_summation_precision);
    RUN_TEST(test_index_nav_large_batch);
    RUN_TEST(test_index_nav_invalid_null_nav_out);
    RUN_TEST(test_index_nav_invalid_null_prices);
    RUN_TEST(test_index_nav_invalid_null_weights);
    RUN_TEST(test_index_nav_invalid_null_n_constituents);
    RUN_TEST(test_index_nav_invalid_zero_n_index);
    RUN_TEST(test_index_nav_invalid_zero_max_constituents);
    RUN_TEST(test_index_nav_invalid_negative_n_constituents);
    RUN_TEST(test_index_nav_invalid_n_constituents_exceeds_max);
    RUN_TEST(test_index_nav_invalid_rebal_missing_current_qty);
    RUN_TEST(test_index_nav_invalid_rebal_missing_tracking_aum);
    RUN_TEST(test_index_nav_invalid_zero_price_with_rebalancing);
    RUN_TEST(test_index_nav_invalid_negative_price_with_rebalancing);
}
