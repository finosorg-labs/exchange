/**
 * @file test_stat_arb.c
 * @brief Unit tests for statistical arbitrage strategy
 */

#include "test_framework.h"
#include "strategy/stat_arb.h"
#include "platform.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define EPSILON 1e-9

TEST(test_zscore_state_init) {
    fc_ex_strat_zscore_state_t state;
    fc_status_t status = fc_ex_strat_zscore_state_init(&state, 50);
    ASSERT_EQ(status, FC_OK);
    ASSERT_TRUE(state.buffer != NULL);
    ASSERT_EQ(state.window_size, 50);
    ASSERT_EQ(state.count, 0);
    ASSERT_EQ(state.head, 0);
    FC_TEST_ASSERT_DOUBLE_EQ(state.sum, 0.0, EPSILON);
    FC_TEST_ASSERT_DOUBLE_EQ(state.sum_sq, 0.0, EPSILON);

    fc_ex_strat_zscore_state_free(&state);
}

TEST(test_zscore_state_init_invalid) {
    fc_ex_strat_zscore_state_t state;

    fc_status_t status = fc_ex_strat_zscore_state_init(NULL, 50);
    ASSERT_EQ(status, FC_ERR_INVALID_ARG);

    status = fc_ex_strat_zscore_state_init(&state, 0);
    ASSERT_EQ(status, FC_ERR_INVALID_ARG);

    status = fc_ex_strat_zscore_state_init(&state, 1);
    ASSERT_EQ(status, FC_ERR_INVALID_ARG);
}

TEST(test_zscore_state_reset) {
    fc_ex_strat_zscore_state_t state;
    fc_ex_strat_zscore_state_init(&state, 10);

    state.count = 5;
    state.head = 3;
    state.sum = 10.0;
    state.sum_sq = 25.0;

    fc_ex_strat_zscore_state_reset(&state);

    ASSERT_EQ(state.count, 0);
    ASSERT_EQ(state.head, 0);
    FC_TEST_ASSERT_DOUBLE_EQ(state.sum, 0.0, EPSILON);
    FC_TEST_ASSERT_DOUBLE_EQ(state.sum_sq, 0.0, EPSILON);

    fc_ex_strat_zscore_state_free(&state);
}

TEST(test_coint_beta_basic) {
    const size_t n_pairs = 2;
    const size_t window = 100;

    double* log_pa = (double*)malloc(n_pairs * window * sizeof(double));
    double* log_pb = (double*)malloc(n_pairs * window * sizeof(double));
    double beta_out[2];

    // Use simpler linear relationship: log_pa = beta * log_pb (no intercept)
    for (size_t i = 0; i < window; i++) {
        log_pb[0 * window + i] = 3.0 + 0.01 * i;
        log_pa[0 * window + i] = 1.5 * log_pb[0 * window + i];

        log_pb[1 * window + i] = 4.0 + 0.02 * i;
        log_pa[1 * window + i] = 2.0 * log_pb[1 * window + i];
    }

    fc_status_t status = fc_ex_strat_coint_beta(beta_out, log_pa, log_pb, n_pairs, window);
    ASSERT_EQ(status, FC_OK);

    // Should be very close since it's perfect cointegration
    FC_TEST_ASSERT_DOUBLE_EQ(beta_out[0], 1.5, 0.01);
    FC_TEST_ASSERT_DOUBLE_EQ(beta_out[1], 2.0, 0.01);

    free(log_pa);
    free(log_pb);
}

TEST(test_coint_beta_perfect_cointegration) {
    const size_t n_pairs = 1;
    const size_t window = 50;
    const double true_beta = 1.5;

    double* log_pa = (double*)malloc(window * sizeof(double));
    double* log_pb = (double*)malloc(window * sizeof(double));
    double beta_out[1];

    for (size_t i = 0; i < window; i++) {
        log_pb[i] = 3.0 + 0.01 * i;
        log_pa[i] = true_beta * log_pb[i];
    }

    fc_status_t status = fc_ex_strat_coint_beta(beta_out, log_pa, log_pb, n_pairs, window);
    ASSERT_EQ(status, FC_OK);

    FC_TEST_ASSERT_DOUBLE_EQ(beta_out[0], true_beta, 0.01);

    free(log_pa);
    free(log_pb);
}

TEST(test_coint_beta_null_args) {
    double log_pa[10] = {0};
    double log_pb[10] = {0};
    double beta_out[1];

    fc_status_t status = fc_ex_strat_coint_beta(NULL, log_pa, log_pb, 1, 10);
    ASSERT_EQ(status, FC_ERR_INVALID_ARG);

    status = fc_ex_strat_coint_beta(beta_out, NULL, log_pb, 1, 10);
    ASSERT_EQ(status, FC_ERR_INVALID_ARG);

    status = fc_ex_strat_coint_beta(beta_out, log_pa, NULL, 1, 10);
    ASSERT_EQ(status, FC_ERR_INVALID_ARG);
}

TEST(test_coint_beta_invalid_dimensions) {
    double log_pa[10] = {0};
    double log_pb[10] = {0};
    double beta_out[1];

    fc_status_t status = fc_ex_strat_coint_beta(beta_out, log_pa, log_pb, 0, 10);
    ASSERT_EQ(status, FC_ERR_INVALID_ARG);

    status = fc_ex_strat_coint_beta(beta_out, log_pa, log_pb, 1, 0);
    ASSERT_EQ(status, FC_ERR_INVALID_ARG);

    status = fc_ex_strat_coint_beta(beta_out, log_pa, log_pb, 1, 1);
    ASSERT_EQ(status, FC_ERR_INVALID_ARG);
}

TEST(test_coint_spread_z_basic) {
    const size_t n_pairs = 2;

    double pa[] = {100.0, 200.0};
    double pb[] = {50.0, 100.0};
    double beta[] = {1.5, 2.0};
    double spread_out[2] = {0.0, 0.0};
    double z_out[2] = {0.0, 0.0};

    fc_ex_strat_zscore_state_t states[2];
    fc_ex_strat_zscore_state_init(&states[0], 10);
    fc_ex_strat_zscore_state_init(&states[1], 10);

    fc_status_t status = fc_ex_strat_coint_spread_z(
        spread_out, z_out, pa, pb, beta, states, n_pairs
    );
    ASSERT_EQ(status, FC_OK);

    ASSERT_TRUE(!isnan(spread_out[0]));
    ASSERT_TRUE(!isnan(spread_out[1]));

    fc_ex_strat_zscore_state_free(&states[0]);
    fc_ex_strat_zscore_state_free(&states[1]);
}

TEST(test_coint_spread_z_rolling_window) {
    const size_t n_pairs = 1;
    const size_t window_size = 5;

    double pa[1];
    double pb[1];
    double beta[] = {1.0};
    double spread_out[1];
    double z_out[1];

    fc_ex_strat_zscore_state_t state;
    fc_ex_strat_zscore_state_init(&state, window_size);

    double test_prices_a[] = {100.0, 101.0, 99.0, 100.5, 98.0, 102.0, 100.0};
    double test_prices_b[] = {100.0, 101.0, 99.0, 100.5, 98.0, 102.0, 100.0};

    for (size_t i = 0; i < 7; i++) {
        pa[0] = test_prices_a[i];
        pb[0] = test_prices_b[i];

        fc_status_t status = fc_ex_strat_coint_spread_z(
            spread_out, z_out, pa, pb, beta, &state, n_pairs
        );
        ASSERT_EQ(status, FC_OK);

        if (i >= window_size) {
            ASSERT_TRUE(!isnan(z_out[0]));
        }
    }

    fc_ex_strat_zscore_state_free(&state);
}

TEST(test_coint_spread_z_mean_reversion) {
    const size_t n_pairs = 1;
    const size_t window_size = 10;

    double pa[1];
    double pb[1];
    double beta[] = {1.0};
    double spread_out[1];
    double z_out[1];

    fc_ex_strat_zscore_state_t state;
    fc_ex_strat_zscore_state_init(&state, window_size);

    for (int i = 0; i < 10; i++) {
        pa[0] = 100.0;
        pb[0] = 100.0;
        fc_ex_strat_coint_spread_z(spread_out, z_out, pa, pb, beta, &state, n_pairs);
    }

    double first_z = z_out[0];
    FC_TEST_ASSERT_DOUBLE_EQ(first_z, 0.0, 0.1);

    pa[0] = 110.0;
    pb[0] = 100.0;
    fc_ex_strat_coint_spread_z(spread_out, z_out, pa, pb, beta, &state, n_pairs);

    ASSERT_TRUE(z_out[0] > 1.0);

    fc_ex_strat_zscore_state_free(&state);
}

TEST(test_coint_spread_z_null_args) {
    double pa[1] = {100.0};
    double pb[1] = {50.0};
    double beta[1] = {1.5};
    double spread_out[1];
    double z_out[1];
    fc_ex_strat_zscore_state_t state;
    fc_ex_strat_zscore_state_init(&state, 10);

    fc_status_t status = fc_ex_strat_coint_spread_z(NULL, z_out, pa, pb, beta, &state, 1);
    ASSERT_EQ(status, FC_ERR_INVALID_ARG);

    status = fc_ex_strat_coint_spread_z(spread_out, NULL, pa, pb, beta, &state, 1);
    ASSERT_EQ(status, FC_ERR_INVALID_ARG);

    status = fc_ex_strat_coint_spread_z(spread_out, z_out, NULL, pb, beta, &state, 1);
    ASSERT_EQ(status, FC_ERR_INVALID_ARG);

    status = fc_ex_strat_coint_spread_z(spread_out, z_out, pa, NULL, beta, &state, 1);
    ASSERT_EQ(status, FC_ERR_INVALID_ARG);

    status = fc_ex_strat_coint_spread_z(spread_out, z_out, pa, pb, NULL, &state, 1);
    ASSERT_EQ(status, FC_ERR_INVALID_ARG);

    status = fc_ex_strat_coint_spread_z(spread_out, z_out, pa, pb, beta, NULL, 1);
    ASSERT_EQ(status, FC_ERR_INVALID_ARG);

    fc_ex_strat_zscore_state_free(&state);
}

TEST(test_coint_spread_z_invalid_prices) {
    const size_t n_pairs = 3;

    double pa[] = {100.0, -50.0, 0.0};
    double pb[] = {50.0, 100.0, 50.0};
    double beta[] = {1.5, 2.0, 1.0};
    double spread_out[3];
    double z_out[3];

    fc_ex_strat_zscore_state_t states[3];
    for (size_t i = 0; i < n_pairs; i++) {
        fc_ex_strat_zscore_state_init(&states[i], 10);
    }

    fc_status_t status = fc_ex_strat_coint_spread_z(
        spread_out, z_out, pa, pb, beta, states, n_pairs
    );
    ASSERT_EQ(status, FC_OK);

    ASSERT_TRUE(!isnan(spread_out[0]));
    ASSERT_TRUE(isnan(spread_out[1]));
    ASSERT_TRUE(isnan(spread_out[2]));

    ASSERT_TRUE(!isnan(z_out[0]));
    ASSERT_TRUE(isnan(z_out[1]));
    ASSERT_TRUE(isnan(z_out[2]));

    for (size_t i = 0; i < n_pairs; i++) {
        fc_ex_strat_zscore_state_free(&states[i]);
    }
}

TEST(test_coint_spread_z_batch) {
    const size_t n_pairs = 100;
    const size_t window_size = 20;

    double* pa = (double*)malloc(n_pairs * sizeof(double));
    double* pb = (double*)malloc(n_pairs * sizeof(double));
    double* beta = (double*)malloc(n_pairs * sizeof(double));
    double* spread_out = (double*)malloc(n_pairs * sizeof(double));
    double* z_out = (double*)malloc(n_pairs * sizeof(double));

    fc_ex_strat_zscore_state_t* states =
        (fc_ex_strat_zscore_state_t*)malloc(n_pairs * sizeof(fc_ex_strat_zscore_state_t));

    for (size_t i = 0; i < n_pairs; i++) {
        pa[i] = 100.0 + i * 0.5;
        pb[i] = 50.0 + i * 0.25;
        beta[i] = 1.0 + i * 0.01;
        fc_ex_strat_zscore_state_init(&states[i], window_size);
    }

    fc_status_t status = fc_ex_strat_coint_spread_z(
        spread_out, z_out, pa, pb, beta, states, n_pairs
    );
    ASSERT_EQ(status, FC_OK);

    for (size_t i = 0; i < n_pairs; i++) {
        ASSERT_TRUE(!isnan(spread_out[i]));
        fc_ex_strat_zscore_state_free(&states[i]);
    }

    free(pa);
    free(pb);
    free(beta);
    free(spread_out);
    free(z_out);
    free(states);
}

TEST(test_zscore_window_full) {
    const size_t n_pairs = 1;
    const size_t window_size = 3;

    double pa[1];
    double pb[1];
    double beta[] = {1.0};
    double spread_out[1];
    double z_out[1];

    fc_ex_strat_zscore_state_t state;
    fc_ex_strat_zscore_state_init(&state, window_size);

    double test_values[] = {100.0, 101.0, 102.0, 103.0, 104.0};

    for (size_t i = 0; i < 5; i++) {
        pa[0] = test_values[i];
        pb[0] = test_values[i];

        fc_ex_strat_coint_spread_z(spread_out, z_out, pa, pb, beta, &state, n_pairs);

        if (i >= window_size) {
            ASSERT_EQ(state.count, window_size);
        } else {
            ASSERT_EQ(state.count, i + 1);
        }
    }

    fc_ex_strat_zscore_state_free(&state);
}

void register_stat_arb_tests(void) {
    RUN_TEST(test_zscore_state_init);
    RUN_TEST(test_zscore_state_init_invalid);
    RUN_TEST(test_zscore_state_reset);
    RUN_TEST(test_coint_beta_basic);
    RUN_TEST(test_coint_beta_perfect_cointegration);
    RUN_TEST(test_coint_beta_null_args);
    RUN_TEST(test_coint_beta_invalid_dimensions);
    RUN_TEST(test_coint_spread_z_basic);
    RUN_TEST(test_coint_spread_z_rolling_window);
    RUN_TEST(test_coint_spread_z_mean_reversion);
    RUN_TEST(test_coint_spread_z_null_args);
    RUN_TEST(test_coint_spread_z_invalid_prices);
    RUN_TEST(test_coint_spread_z_batch);
    RUN_TEST(test_zscore_window_full);
}
