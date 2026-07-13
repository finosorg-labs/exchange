/**
 * @file test_spread.c
 * @brief Unit tests for effective spread and Amihud illiquidity signal computation
 */

#include "arena.h"
#include "error.h"
#include "platform.h"
#include "signal/spread.h"
#include "test_framework.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

TEST(test_eff_spread_basic) {
    const size_t n        = 5;
    double trade_price[5] = {100.5, 99.8, 101.2, 100.0, 99.5};
    double micro_price[5] = {100.0, 100.0, 101.0, 100.0, 100.0};
    double eff_out[5];

    fc_status_t status = fc_ex_sig_eff_spread_batch(eff_out, trade_price, micro_price, n);

    FC_TEST_ASSERT_EQ(status, FC_OK);
    FC_TEST_ASSERT_DOUBLE_EQ(eff_out[0], 1.0, 1e-10); // 2 * |100.5 - 100.0| = 1.0
    FC_TEST_ASSERT_DOUBLE_EQ(eff_out[1], 0.4, 1e-10); // 2 * |99.8 - 100.0| = 0.4
    FC_TEST_ASSERT_DOUBLE_EQ(eff_out[2], 0.4, 1e-10); // 2 * |101.2 - 101.0| = 0.4
    FC_TEST_ASSERT_DOUBLE_EQ(eff_out[3], 0.0, 1e-10); // 2 * |100.0 - 100.0| = 0.0
    FC_TEST_ASSERT_DOUBLE_EQ(eff_out[4], 1.0, 1e-10); // 2 * |99.5 - 100.0| = 1.0
}

TEST(test_eff_spread_negative_diff) {
    const size_t n        = 3;
    double trade_price[3] = {99.0, 101.0, 100.0};
    double micro_price[3] = {100.0, 100.0, 100.0};
    double eff_out[3];

    fc_status_t status = fc_ex_sig_eff_spread_batch(eff_out, trade_price, micro_price, n);

    FC_TEST_ASSERT_EQ(status, FC_OK);
    FC_TEST_ASSERT_DOUBLE_EQ(eff_out[0], 2.0, 1e-10); // 2 * |99.0 - 100.0| = 2.0
    FC_TEST_ASSERT_DOUBLE_EQ(eff_out[1], 2.0, 1e-10); // 2 * |101.0 - 100.0| = 2.0
    FC_TEST_ASSERT_DOUBLE_EQ(eff_out[2], 0.0, 1e-10); // 2 * |100.0 - 100.0| = 0.0
}

TEST(test_eff_spread_large_batch) {
    const size_t n = 1000;
    double trade_price[1000];
    double micro_price[1000];
    double eff_out[1000];

    for (size_t i = 0; i < n; i++) {
        trade_price[i] = 100.0 + (double) i * 0.1;
        micro_price[i] = 100.0;
    }

    fc_status_t status = fc_ex_sig_eff_spread_batch(eff_out, trade_price, micro_price, n);

    FC_TEST_ASSERT_EQ(status, FC_OK);
    for (size_t i = 0; i < n; i++) {
        double expected = 2.0 * fabs(trade_price[i] - micro_price[i]);
        FC_TEST_ASSERT_DOUBLE_EQ(eff_out[i], expected, 1e-10);
    }
}

TEST(test_eff_spread_null_input) {
    double eff_out[10]     = {0};
    double trade_price[10] = {0};
    double micro_price[10] = {0};

    FC_TEST_ASSERT_EQ(
        fc_ex_sig_eff_spread_batch(NULL, trade_price, micro_price, 10), FC_ERR_INVALID_ARG
    );
    FC_TEST_ASSERT_EQ(
        fc_ex_sig_eff_spread_batch(eff_out, NULL, micro_price, 10), FC_ERR_INVALID_ARG
    );
    FC_TEST_ASSERT_EQ(
        fc_ex_sig_eff_spread_batch(eff_out, trade_price, NULL, 10), FC_ERR_INVALID_ARG
    );
    FC_TEST_ASSERT_EQ(
        fc_ex_sig_eff_spread_batch(eff_out, trade_price, micro_price, 0), FC_ERR_INVALID_ARG
    );
}

TEST(test_amihud_basic) {
    const size_t n    = 5;
    double returns[5] = {0.01, -0.02, 0.005, 0.0, -0.015};
    double volume[5]  = {1000.0, 2000.0, 500.0, 1000.0, 1500.0};
    double illiq_out[5];

    fc_status_t status = fc_ex_sig_amihud_batch(illiq_out, returns, volume, n);

    FC_TEST_ASSERT_EQ(status, FC_OK);
    FC_TEST_ASSERT_DOUBLE_EQ(illiq_out[0], 0.01 / 1000.0, 1e-12);  // |0.01| / 1000
    FC_TEST_ASSERT_DOUBLE_EQ(illiq_out[1], 0.02 / 2000.0, 1e-12);  // |-0.02| / 2000
    FC_TEST_ASSERT_DOUBLE_EQ(illiq_out[2], 0.005 / 500.0, 1e-12);  // |0.005| / 500
    FC_TEST_ASSERT_DOUBLE_EQ(illiq_out[3], 0.0 / 1000.0, 1e-12);   // |0.0| / 1000
    FC_TEST_ASSERT_DOUBLE_EQ(illiq_out[4], 0.015 / 1500.0, 1e-12); // |-0.015| / 1500
}

TEST(test_amihud_zero_volume) {
    const size_t n    = 3;
    double returns[3] = {0.01, -0.02, 0.005};
    double volume[3]  = {0.0, 1000.0, 0.0};
    double illiq_out[3];

    fc_status_t status = fc_ex_sig_amihud_batch(illiq_out, returns, volume, n);

    FC_TEST_ASSERT_EQ(status, FC_OK);
    ASSERT_TRUE(isnan(illiq_out[0]));
    FC_TEST_ASSERT_DOUBLE_EQ(illiq_out[1], 0.02 / 1000.0, 1e-12);
    ASSERT_TRUE(isnan(illiq_out[2]));
}

TEST(test_amihud_negative_volume) {
    const size_t n    = 2;
    double returns[2] = {0.01, -0.02};
    double volume[2]  = {-1000.0, 1000.0};
    double illiq_out[2];

    fc_status_t status = fc_ex_sig_amihud_batch(illiq_out, returns, volume, n);

    FC_TEST_ASSERT_EQ(status, FC_OK);
    ASSERT_TRUE(isnan(illiq_out[0]));
    FC_TEST_ASSERT_DOUBLE_EQ(illiq_out[1], 0.02 / 1000.0, 1e-12);
}

TEST(test_amihud_large_batch) {
    const size_t n = 1000;
    double returns[1000];
    double volume[1000];
    double illiq_out[1000];

    for (size_t i = 0; i < n; i++) {
        returns[i] = (double) i * 0.001 - 0.5;
        volume[i]  = 1000.0 + (double) i * 10.0;
    }

    fc_status_t status = fc_ex_sig_amihud_batch(illiq_out, returns, volume, n);

    FC_TEST_ASSERT_EQ(status, FC_OK);
    for (size_t i = 0; i < n; i++) {
        double expected = fabs(returns[i]) / volume[i];
        FC_TEST_ASSERT_DOUBLE_EQ(illiq_out[i], expected, 1e-12);
    }
}

TEST(test_amihud_null_input) {
    double illiq_out[10] = {0};
    double returns[10]   = {0};
    double volume[10]    = {0};

    FC_TEST_ASSERT_EQ(fc_ex_sig_amihud_batch(NULL, returns, volume, 10), FC_ERR_INVALID_ARG);
    FC_TEST_ASSERT_EQ(fc_ex_sig_amihud_batch(illiq_out, NULL, volume, 10), FC_ERR_INVALID_ARG);
    FC_TEST_ASSERT_EQ(fc_ex_sig_amihud_batch(illiq_out, returns, NULL, 10), FC_ERR_INVALID_ARG);
    FC_TEST_ASSERT_EQ(fc_ex_sig_amihud_batch(illiq_out, returns, volume, 0), FC_ERR_INVALID_ARG);
}

TEST(test_eff_spread_nan_inf) {
    const size_t n        = 4;
    double trade_price[4] = {NAN, INFINITY, -INFINITY, 100.0};
    double micro_price[4] = {100.0, 100.0, 100.0, NAN};
    double eff_out[4];

    fc_status_t status = fc_ex_sig_eff_spread_batch(eff_out, trade_price, micro_price, n);

    FC_TEST_ASSERT_EQ(status, FC_OK);
    ASSERT_TRUE(isnan(eff_out[0]));
    ASSERT_TRUE(isinf(eff_out[1]));
    ASSERT_TRUE(isinf(eff_out[2]));
    ASSERT_TRUE(isnan(eff_out[3]));
}

TEST(test_amihud_nan_inf) {
    const size_t n    = 4;
    double returns[4] = {NAN, INFINITY, 0.01, 0.01};
    double volume[4]  = {1000.0, 1000.0, INFINITY, NAN};
    double illiq_out[4];

    fc_status_t status = fc_ex_sig_amihud_batch(illiq_out, returns, volume, n);

    FC_TEST_ASSERT_EQ(status, FC_OK);
    ASSERT_TRUE(isnan(illiq_out[0]));
    ASSERT_TRUE(isinf(illiq_out[1]));
    FC_TEST_ASSERT_EQ(illiq_out[2], 0.0);
    ASSERT_TRUE(isnan(illiq_out[3]));
}

TEST(test_eff_spread_rolling_mean) {
    const size_t n         = 10;
    double trade_price[10] = {100.5, 100.3, 100.7, 100.2, 100.6, 100.4, 100.8, 100.1, 100.5, 100.3};
    double micro_price[10] = {100.0, 100.0, 100.0, 100.0, 100.0, 100.0, 100.0, 100.0, 100.0, 100.0};
    double rolling_mean[10] = {0.0}; /* Initialize to zero */
    size_t window_size      = 3;

    fc_arena_t* arena = fc_arena_create(n * sizeof(double));
    FC_TEST_ASSERT(arena != NULL);

    fc_status_t status = fc_ex_sig_eff_spread_rolling_mean(
        rolling_mean, trade_price, micro_price, arena, n, window_size
    );

    FC_TEST_ASSERT_EQ(status, FC_OK);

    /* First element: window [0:0] */
    FC_TEST_ASSERT_DOUBLE_EQ(rolling_mean[0], 1.0, 1e-10); // 2 * |100.5 - 100.0| = 1.0

    /* Second element: window [0:1], mean of (1.0, 0.6) */
    FC_TEST_ASSERT_DOUBLE_EQ(rolling_mean[1], 0.8, 1e-10); // (1.0 + 0.6) / 2

    /* Third element: window [0:2], mean of (1.0, 0.6, 1.4) */
    FC_TEST_ASSERT_DOUBLE_EQ(rolling_mean[2], 1.0, 1e-10); // (1.0 + 0.6 + 1.4) / 3

    /* Fourth element: window [1:3], mean of (0.6, 1.4, 0.4) */
    FC_TEST_ASSERT_DOUBLE_EQ(rolling_mean[3], 0.8, 1e-10); // (0.6 + 1.4 + 0.4) / 3

    fc_arena_destroy(arena);
}

TEST(test_eff_spread_rolling_stddev) {
    const size_t n         = 10;
    double trade_price[10] = {100.5, 100.5, 100.5, 100.5, 100.5, 100.5, 100.5, 100.5, 100.5, 100.5};
    double micro_price[10] = {100.0, 100.0, 100.0, 100.0, 100.0, 100.0, 100.0, 100.0, 100.0, 100.0};
    double rolling_stddev[10] = {0.0}; /* Initialize to zero */
    size_t window_size        = 3;

    fc_arena_t* arena = fc_arena_create(n * sizeof(double));
    FC_TEST_ASSERT(arena != NULL);

    fc_status_t status = fc_ex_sig_eff_spread_rolling_stddev(
        rolling_stddev, trade_price, micro_price, arena, n, window_size
    );

    FC_TEST_ASSERT_EQ(status, FC_OK);

    /* All values are constant (1.0), so stddev should be 0 */
    for (size_t i = 2; i < n; i++) {
        FC_TEST_ASSERT_DOUBLE_EQ(rolling_stddev[i], 0.0, 1e-10);
    }

    fc_arena_destroy(arena);
}

TEST(test_amihud_rolling_mean) {
    const size_t n     = 10;
    double returns[10] = {0.01, 0.02, 0.015, 0.01, 0.02, 0.015, 0.01, 0.02, 0.015, 0.01};
    double volume[10]  = {
        1000.0, 1000.0, 1000.0, 1000.0, 1000.0, 1000.0, 1000.0, 1000.0, 1000.0, 1000.0
    };
    double rolling_mean[10] = {0.0}; /* Initialize to zero */
    size_t window_size      = 3;

    fc_arena_t* arena = fc_arena_create(n * sizeof(double));
    FC_TEST_ASSERT(arena != NULL);

    fc_status_t status =
        fc_ex_sig_amihud_rolling_mean(rolling_mean, returns, volume, arena, n, window_size);

    FC_TEST_ASSERT_EQ(status, FC_OK);

    /* First element: 0.01 / 1000.0 = 0.00001 */
    FC_TEST_ASSERT_DOUBLE_EQ(rolling_mean[0], 0.00001, 1e-10);

    /* Third element: mean of (0.00001, 0.00002, 0.000015) */
    double expected_third = (0.00001 + 0.00002 + 0.000015) / 3.0;
    FC_TEST_ASSERT_DOUBLE_EQ(rolling_mean[2], expected_third, 1e-10);

    fc_arena_destroy(arena);
}

TEST(test_rolling_invalid_window) {
    double trade_price[10]  = {0};
    double micro_price[10]  = {0};
    double rolling_mean[10] = {0};

    fc_arena_t* arena = fc_arena_create(10 * sizeof(double));
    FC_TEST_ASSERT(arena != NULL);

    /* Window size = 0 should fail */
    FC_TEST_ASSERT_EQ(
        fc_ex_sig_eff_spread_rolling_mean(rolling_mean, trade_price, micro_price, arena, 10, 0),
        FC_ERR_INVALID_ARG
    );

    /* Window size > n should fail */
    FC_TEST_ASSERT_EQ(
        fc_ex_sig_eff_spread_rolling_mean(rolling_mean, trade_price, micro_price, arena, 10, 11),
        FC_ERR_INVALID_ARG
    );

    fc_arena_destroy(arena);
}

void register_spread_tests(void) {
    RUN_TEST(test_eff_spread_basic);
    RUN_TEST(test_eff_spread_negative_diff);
    RUN_TEST(test_eff_spread_large_batch);
    RUN_TEST(test_eff_spread_null_input);
    RUN_TEST(test_amihud_basic);
    RUN_TEST(test_amihud_zero_volume);
    RUN_TEST(test_amihud_negative_volume);
    RUN_TEST(test_amihud_large_batch);
    RUN_TEST(test_amihud_null_input);
    RUN_TEST(test_eff_spread_nan_inf);
    RUN_TEST(test_amihud_nan_inf);
    RUN_TEST(test_eff_spread_rolling_mean);
    RUN_TEST(test_eff_spread_rolling_stddev);
    RUN_TEST(test_amihud_rolling_mean);
    RUN_TEST(test_rolling_invalid_window);
}
