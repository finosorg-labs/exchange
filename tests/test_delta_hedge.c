/**
 * @file test_delta_hedge.c
 * @brief Unit tests for delta hedging strategy
 */

#include "platform.h"
#include "strategy/delta_hedge.h"
#include "test_framework.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define EPSILON 1e-9

TEST(test_delta_aggregate_basic) {
    double leg_delta[]    = {0.5, 0.3};
    double leg_qty[]      = {100.0, -50.0};
    int n_legs[]          = {2};
    double delta_future[] = {1.0};
    double delta_net_out[1];
    double hedge_qty_out[1];

    fc_status_t status = fc_ex_strat_delta_aggregate(
        delta_net_out, hedge_qty_out, leg_delta, leg_qty, n_legs, delta_future, 1, 2
    );

    ASSERT_EQ(status, FC_OK);

    // Δ_net = 0.5*100 + 0.3*(-50) = 50 - 15 = 35
    double expected_delta_net = 35.0;
    FC_TEST_ASSERT_DOUBLE_EQ(delta_net_out[0], expected_delta_net, EPSILON);

    // N_hedge = -35 / 1.0 = -35
    double expected_hedge_qty = -35.0;
    FC_TEST_ASSERT_DOUBLE_EQ(hedge_qty_out[0], expected_hedge_qty, EPSILON);
}

TEST(test_delta_aggregate_multiple_books) {
    // 2 books with max 3 legs each
    double leg_delta[] = {
        0.6,
        0.4,
        0.2, // Book 0: 3 legs
        0.5,
        0.3,
        0.0 // Book 1: 2 legs (3rd unused)
    };
    double leg_qty[] = {
        100.0,
        -50.0,
        25.0, // Book 0
        200.0,
        -100.0,
        0.0 // Book 1
    };
    int n_legs[]          = {3, 2};
    double delta_future[] = {1.0, 0.5};
    double delta_net_out[2];
    double hedge_qty_out[2];

    fc_status_t status = fc_ex_strat_delta_aggregate(
        delta_net_out, hedge_qty_out, leg_delta, leg_qty, n_legs, delta_future, 2, 3
    );

    ASSERT_EQ(status, FC_OK);

    // Book 0: Δ_net = 0.6*100 + 0.4*(-50) + 0.2*25 = 60 - 20 + 5 = 45
    double expected_delta_net_0 = 45.0;
    FC_TEST_ASSERT_DOUBLE_EQ(delta_net_out[0], expected_delta_net_0, EPSILON);

    // Book 0: N_hedge = -45 / 1.0 = -45
    double expected_hedge_qty_0 = -45.0;
    FC_TEST_ASSERT_DOUBLE_EQ(hedge_qty_out[0], expected_hedge_qty_0, EPSILON);

    // Book 1: Δ_net = 0.5*200 + 0.3*(-100) = 100 - 30 = 70
    double expected_delta_net_1 = 70.0;
    FC_TEST_ASSERT_DOUBLE_EQ(delta_net_out[1], expected_delta_net_1, EPSILON);

    // Book 1: N_hedge = -70 / 0.5 = -140
    double expected_hedge_qty_1 = -140.0;
    FC_TEST_ASSERT_DOUBLE_EQ(hedge_qty_out[1], expected_hedge_qty_1, EPSILON);
}

TEST(test_delta_aggregate_zero_net_delta) {
    // Perfectly balanced portfolio
    double leg_delta[]    = {0.5, 0.5};
    double leg_qty[]      = {100.0, -100.0};
    int n_legs[]          = {2};
    double delta_future[] = {1.0};
    double delta_net_out[1];
    double hedge_qty_out[1];

    fc_status_t status = fc_ex_strat_delta_aggregate(
        delta_net_out, hedge_qty_out, leg_delta, leg_qty, n_legs, delta_future, 1, 2
    );

    ASSERT_EQ(status, FC_OK);

    // Δ_net = 0.5*100 + 0.5*(-100) = 0
    FC_TEST_ASSERT_DOUBLE_EQ(delta_net_out[0], 0.0, EPSILON);

    // N_hedge = 0
    FC_TEST_ASSERT_DOUBLE_EQ(hedge_qty_out[0], 0.0, EPSILON);
}

TEST(test_delta_aggregate_negative_delta) {
    // Portfolio with net short delta
    double leg_delta[]    = {-0.6, -0.4};
    double leg_qty[]      = {100.0, 50.0};
    int n_legs[]          = {2};
    double delta_future[] = {1.0};
    double delta_net_out[1];
    double hedge_qty_out[1];

    fc_status_t status = fc_ex_strat_delta_aggregate(
        delta_net_out, hedge_qty_out, leg_delta, leg_qty, n_legs, delta_future, 1, 2
    );

    ASSERT_EQ(status, FC_OK);

    // Δ_net = -0.6*100 + -0.4*50 = -60 - 20 = -80
    double expected_delta_net = -80.0;
    FC_TEST_ASSERT_DOUBLE_EQ(delta_net_out[0], expected_delta_net, EPSILON);

    // N_hedge = -(-80) / 1.0 = 80
    double expected_hedge_qty = 80.0;
    FC_TEST_ASSERT_DOUBLE_EQ(hedge_qty_out[0], expected_hedge_qty, EPSILON);
}

TEST(test_delta_aggregate_non_unit_future_delta) {
    // Hedging instrument with delta != 1.0
    double leg_delta[]    = {0.5};
    double leg_qty[]      = {100.0};
    int n_legs[]          = {1};
    double delta_future[] = {0.8};
    double delta_net_out[1];
    double hedge_qty_out[1];

    fc_status_t status = fc_ex_strat_delta_aggregate(
        delta_net_out, hedge_qty_out, leg_delta, leg_qty, n_legs, delta_future, 1, 1
    );

    ASSERT_EQ(status, FC_OK);

    // Δ_net = 0.5*100 = 50
    double expected_delta_net = 50.0;
    FC_TEST_ASSERT_DOUBLE_EQ(delta_net_out[0], expected_delta_net, EPSILON);

    // N_hedge = -50 / 0.8 = -62.5
    double expected_hedge_qty = -62.5;
    FC_TEST_ASSERT_DOUBLE_EQ(hedge_qty_out[0], expected_hedge_qty, EPSILON);
}

TEST(test_delta_aggregate_single_leg) {
    double leg_delta[]    = {0.7};
    double leg_qty[]      = {200.0};
    int n_legs[]          = {1};
    double delta_future[] = {1.0};
    double delta_net_out[1];
    double hedge_qty_out[1];

    fc_status_t status = fc_ex_strat_delta_aggregate(
        delta_net_out, hedge_qty_out, leg_delta, leg_qty, n_legs, delta_future, 1, 1
    );

    ASSERT_EQ(status, FC_OK);

    // Δ_net = 0.7*200 = 140
    double expected_delta_net = 140.0;
    FC_TEST_ASSERT_DOUBLE_EQ(delta_net_out[0], expected_delta_net, EPSILON);

    // N_hedge = -140 / 1.0 = -140
    double expected_hedge_qty = -140.0;
    FC_TEST_ASSERT_DOUBLE_EQ(hedge_qty_out[0], expected_hedge_qty, EPSILON);
}

TEST(test_delta_aggregate_nan_handling) {
    // Book with NaN in leg delta
    double leg_delta[]    = {NAN, 0.5};
    double leg_qty[]      = {100.0, 50.0};
    int n_legs[]          = {2};
    double delta_future[] = {1.0};
    double delta_net_out[1];
    double hedge_qty_out[1];

    fc_status_t status = fc_ex_strat_delta_aggregate(
        delta_net_out, hedge_qty_out, leg_delta, leg_qty, n_legs, delta_future, 1, 2
    );

    ASSERT_EQ(status, FC_OK);

    ASSERT_TRUE(isnan(delta_net_out[0]));
    ASSERT_TRUE(isnan(hedge_qty_out[0]));
}

TEST(test_delta_aggregate_zero_delta_future) {
    // Delta future is zero (division by zero case)
    double leg_delta[]    = {0.5};
    double leg_qty[]      = {100.0};
    int n_legs[]          = {1};
    double delta_future[] = {0.0};
    double delta_net_out[1];
    double hedge_qty_out[1];

    fc_status_t status = fc_ex_strat_delta_aggregate(
        delta_net_out, hedge_qty_out, leg_delta, leg_qty, n_legs, delta_future, 1, 1
    );

    ASSERT_EQ(status, FC_OK);

    // DeltaNet should be computed normally
    double expected_delta_net = 50.0;
    FC_TEST_ASSERT_DOUBLE_EQ(delta_net_out[0], expected_delta_net, EPSILON);

    // HedgeQty should be NaN (division by zero)
    ASSERT_TRUE(isnan(hedge_qty_out[0]));
}

TEST(test_delta_aggregate_large_batch) {
    const size_t n_books = 100;
    const int max_legs   = 5;

    double* leg_delta     = (double*) malloc(n_books * max_legs * sizeof(double));
    double* leg_qty       = (double*) malloc(n_books * max_legs * sizeof(double));
    int* n_legs           = (int*) malloc(n_books * sizeof(int));
    double* delta_future  = (double*) malloc(n_books * sizeof(double));
    double* delta_net_out = (double*) malloc(n_books * sizeof(double));
    double* hedge_qty_out = (double*) malloc(n_books * sizeof(double));

    if (!leg_delta || !leg_qty || !n_legs || !delta_future || !delta_net_out || !hedge_qty_out) {
        free(leg_delta);
        free(leg_qty);
        free(n_legs);
        free(delta_future);
        free(delta_net_out);
        free(hedge_qty_out);
        ASSERT_NOT_NULL(NULL);
    }

    for (size_t i = 0; i < n_books; i++) {
        n_legs[i]       = 3;
        delta_future[i] = 1.0;
        for (int j = 0; j < max_legs; j++) {
            leg_delta[i * max_legs + j] = 0.5;
            leg_qty[i * max_legs + j]   = 100.0;
        }
    }

    fc_status_t status = fc_ex_strat_delta_aggregate(
        delta_net_out, hedge_qty_out, leg_delta, leg_qty, n_legs, delta_future, n_books, max_legs
    );

    ASSERT_EQ(status, FC_OK);

    // Each book: Δ_net = 0.5*100 + 0.5*100 + 0.5*100 = 150
    double expected_delta_net = 150.0;
    double expected_hedge_qty = -150.0;

    for (size_t i = 0; i < n_books; i++) {
        FC_TEST_ASSERT_DOUBLE_EQ(delta_net_out[i], expected_delta_net, EPSILON);
        FC_TEST_ASSERT_DOUBLE_EQ(hedge_qty_out[i], expected_hedge_qty, EPSILON);
    }

    free(leg_delta);
    free(leg_qty);
    free(n_legs);
    free(delta_future);
    free(delta_net_out);
    free(hedge_qty_out);
}

TEST(test_delta_aggregate_invalid_null_outputs) {
    double leg_delta[]    = {0.5};
    double leg_qty[]      = {100.0};
    int n_legs[]          = {1};
    double delta_future[] = {1.0};
    double delta_net_out[1];
    double hedge_qty_out[1];

    fc_status_t status = fc_ex_strat_delta_aggregate(
        NULL, hedge_qty_out, leg_delta, leg_qty, n_legs, delta_future, 1, 1
    );
    ASSERT_EQ(status, FC_ERR_INVALID_ARG);

    status = fc_ex_strat_delta_aggregate(
        delta_net_out, NULL, leg_delta, leg_qty, n_legs, delta_future, 1, 1
    );
    ASSERT_EQ(status, FC_ERR_INVALID_ARG);
}

TEST(test_delta_aggregate_invalid_null_inputs) {
    double delta_net_out[1];
    double hedge_qty_out[1];
    double leg_qty[]      = {100.0};
    int n_legs[]          = {1};
    double delta_future[] = {1.0};

    fc_status_t status = fc_ex_strat_delta_aggregate(
        delta_net_out, hedge_qty_out, NULL, leg_qty, n_legs, delta_future, 1, 1
    );
    ASSERT_EQ(status, FC_ERR_INVALID_ARG);

    double leg_delta[] = {0.5};
    status             = fc_ex_strat_delta_aggregate(
        delta_net_out, hedge_qty_out, leg_delta, NULL, n_legs, delta_future, 1, 1
    );
    ASSERT_EQ(status, FC_ERR_INVALID_ARG);

    status = fc_ex_strat_delta_aggregate(
        delta_net_out, hedge_qty_out, leg_delta, leg_qty, NULL, delta_future, 1, 1
    );
    ASSERT_EQ(status, FC_ERR_INVALID_ARG);

    status = fc_ex_strat_delta_aggregate(
        delta_net_out, hedge_qty_out, leg_delta, leg_qty, n_legs, NULL, 1, 1
    );
    ASSERT_EQ(status, FC_ERR_INVALID_ARG);
}

TEST(test_delta_aggregate_invalid_zero_n_books) {
    double leg_delta[]    = {0.5};
    double leg_qty[]      = {100.0};
    int n_legs[]          = {1};
    double delta_future[] = {1.0};
    double delta_net_out[1];
    double hedge_qty_out[1];

    fc_status_t status = fc_ex_strat_delta_aggregate(
        delta_net_out, hedge_qty_out, leg_delta, leg_qty, n_legs, delta_future, 0, 1
    );
    ASSERT_EQ(status, FC_ERR_INVALID_ARG);
}

TEST(test_delta_aggregate_invalid_zero_max_legs) {
    double leg_delta[]    = {0.5};
    double leg_qty[]      = {100.0};
    int n_legs[]          = {1};
    double delta_future[] = {1.0};
    double delta_net_out[1];
    double hedge_qty_out[1];

    fc_status_t status = fc_ex_strat_delta_aggregate(
        delta_net_out, hedge_qty_out, leg_delta, leg_qty, n_legs, delta_future, 1, 0
    );
    ASSERT_EQ(status, FC_ERR_INVALID_ARG);
}

TEST(test_delta_aggregate_invalid_n_legs_out_of_range) {
    double leg_delta[]    = {0.5, 0.3};
    double leg_qty[]      = {100.0, 50.0};
    int n_legs[]          = {0}; // Invalid: n_legs < 1
    double delta_future[] = {1.0};
    double delta_net_out[1];
    double hedge_qty_out[1];

    fc_status_t status = fc_ex_strat_delta_aggregate(
        delta_net_out, hedge_qty_out, leg_delta, leg_qty, n_legs, delta_future, 1, 2
    );
    ASSERT_EQ(status, FC_ERR_INVALID_ARG);

    n_legs[0] = 3; // Invalid: n_legs > max_legs
    status    = fc_ex_strat_delta_aggregate(
        delta_net_out, hedge_qty_out, leg_delta, leg_qty, n_legs, delta_future, 1, 2
    );
    ASSERT_EQ(status, FC_ERR_INVALID_ARG);
}

void register_delta_hedge_tests(void) {
    RUN_TEST(test_delta_aggregate_basic);
    RUN_TEST(test_delta_aggregate_multiple_books);
    RUN_TEST(test_delta_aggregate_zero_net_delta);
    RUN_TEST(test_delta_aggregate_negative_delta);
    RUN_TEST(test_delta_aggregate_non_unit_future_delta);
    RUN_TEST(test_delta_aggregate_single_leg);
    RUN_TEST(test_delta_aggregate_nan_handling);
    RUN_TEST(test_delta_aggregate_zero_delta_future);
    RUN_TEST(test_delta_aggregate_large_batch);
    RUN_TEST(test_delta_aggregate_invalid_null_outputs);
    RUN_TEST(test_delta_aggregate_invalid_null_inputs);
    RUN_TEST(test_delta_aggregate_invalid_zero_n_books);
    RUN_TEST(test_delta_aggregate_invalid_zero_max_legs);
    RUN_TEST(test_delta_aggregate_invalid_n_legs_out_of_range);
}
