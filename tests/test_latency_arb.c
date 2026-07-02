/**
 * @file test_latency_arb.c
 * @brief Unit tests for latency arbitrage strategy
 */

#include "test_framework.h"
#include "strategy/latency_arb.h"
#include "platform.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define EPSILON 1e-9

TEST(test_latarb_calibrate_perfect_linear) {
    const size_t n_pairs = 2;
    const size_t window = 100;
    const double true_a_0 = 1.2;
    const double true_b_0 = 5.0;
    const double true_a_1 = 0.8;
    const double true_b_1 = -3.0;

    double* hist_fast = (double*)malloc(n_pairs * window * sizeof(double));
    double* hist_slow = (double*)malloc(n_pairs * window * sizeof(double));
    double coef_a_out[2];
    double coef_b_out[2];

    for (size_t i = 0; i < window; i++) {
        hist_fast[0 * window + i] = 100.0 + 0.5 * i;
        hist_slow[0 * window + i] = true_a_0 * hist_fast[0 * window + i] + true_b_0;

        hist_fast[1 * window + i] = 200.0 + 1.0 * i;
        hist_slow[1 * window + i] = true_a_1 * hist_fast[1 * window + i] + true_b_1;
    }

    fc_status_t status = fc_ex_strat_latarb_calibrate(
        coef_a_out,
        coef_b_out,
        hist_fast,
        hist_slow,
        n_pairs,
        window
    );
    ASSERT_EQ(status, FC_OK);

    FC_TEST_ASSERT_DOUBLE_EQ(coef_a_out[0], true_a_0, 0.001);
    FC_TEST_ASSERT_DOUBLE_EQ(coef_b_out[0], true_b_0, 0.1);
    FC_TEST_ASSERT_DOUBLE_EQ(coef_a_out[1], true_a_1, 0.001);
    FC_TEST_ASSERT_DOUBLE_EQ(coef_b_out[1], true_b_1, 0.1);

    free(hist_fast);
    free(hist_slow);
}

TEST(test_latarb_calibrate_with_noise) {
    const size_t n_pairs = 1;
    const size_t window = 200;
    const double true_a = 1.1;
    const double true_b = 2.5;

    double* hist_fast = (double*)malloc(window * sizeof(double));
    double* hist_slow = (double*)malloc(window * sizeof(double));
    double coef_a_out[1];
    double coef_b_out[1];

    srand(42);
    for (size_t i = 0; i < window; i++) {
        hist_fast[i] = 100.0 + 0.2 * i;
        double noise = ((rand() % 1000) / 1000.0 - 0.5) * 0.5;
        hist_slow[i] = true_a * hist_fast[i] + true_b + noise;
    }

    fc_status_t status = fc_ex_strat_latarb_calibrate(
        coef_a_out,
        coef_b_out,
        hist_fast,
        hist_slow,
        n_pairs,
        window
    );
    ASSERT_EQ(status, FC_OK);

    FC_TEST_ASSERT_DOUBLE_EQ(coef_a_out[0], true_a, 0.01);
    FC_TEST_ASSERT_DOUBLE_EQ(coef_b_out[0], true_b, 1.0);

    free(hist_fast);
    free(hist_slow);
}

TEST(test_latarb_calibrate_null_args) {
    double hist_fast[10] = {0};
    double hist_slow[10] = {0};
    double coef_a[1];
    double coef_b[1];

    fc_status_t status = fc_ex_strat_latarb_calibrate(NULL, coef_b, hist_fast, hist_slow, 1, 10);
    ASSERT_EQ(status, FC_ERR_INVALID_ARG);

    status = fc_ex_strat_latarb_calibrate(coef_a, NULL, hist_fast, hist_slow, 1, 10);
    ASSERT_EQ(status, FC_ERR_INVALID_ARG);

    status = fc_ex_strat_latarb_calibrate(coef_a, coef_b, NULL, hist_slow, 1, 10);
    ASSERT_EQ(status, FC_ERR_INVALID_ARG);

    status = fc_ex_strat_latarb_calibrate(coef_a, coef_b, hist_fast, NULL, 1, 10);
    ASSERT_EQ(status, FC_ERR_INVALID_ARG);
}

TEST(test_latarb_calibrate_invalid_dimensions) {
    double hist_fast[10] = {0};
    double hist_slow[10] = {0};
    double coef_a[1];
    double coef_b[1];

    fc_status_t status = fc_ex_strat_latarb_calibrate(coef_a, coef_b, hist_fast, hist_slow, 0, 10);
    ASSERT_EQ(status, FC_ERR_INVALID_ARG);

    status = fc_ex_strat_latarb_calibrate(coef_a, coef_b, hist_fast, hist_slow, 1, 0);
    ASSERT_EQ(status, FC_ERR_INVALID_ARG);

    status = fc_ex_strat_latarb_calibrate(coef_a, coef_b, hist_fast, hist_slow, 1, 1);
    ASSERT_EQ(status, FC_ERR_INVALID_ARG);
}

TEST(test_latarb_signal_basic) {
    const size_t n = 3;

    double price_fast[] = {100.0, 200.0, 150.0};
    double price_slow[] = {125.0, 245.0, 182.0};
    double coef_a[] = {1.2, 1.2, 1.2};
    double coef_b[] = {5.0, 5.0, 5.0};
    double theta[] = {2.0, 2.0, 2.0};

    double dev_out[3];
    int hit_out[3];

    fc_status_t status = fc_ex_strat_latarb_signal(
        dev_out,
        hit_out,
        price_fast,
        price_slow,
        coef_a,
        coef_b,
        theta,
        n
    );
    ASSERT_EQ(status, FC_OK);

    double expected_pred_0 = 1.2 * 100.0 + 5.0;
    double expected_dev_0 = 125.0 - expected_pred_0;
    FC_TEST_ASSERT_DOUBLE_EQ(dev_out[0], expected_dev_0, EPSILON);
    ASSERT_EQ(hit_out[0], (fabs(expected_dev_0) > 2.0) ? 1 : 0);

    double expected_pred_1 = 1.2 * 200.0 + 5.0;
    double expected_dev_1 = 245.0 - expected_pred_1;
    FC_TEST_ASSERT_DOUBLE_EQ(dev_out[1], expected_dev_1, EPSILON);
    ASSERT_EQ(hit_out[1], (fabs(expected_dev_1) > 2.0) ? 1 : 0);

    double expected_pred_2 = 1.2 * 150.0 + 5.0;
    double expected_dev_2 = 182.0 - expected_pred_2;
    FC_TEST_ASSERT_DOUBLE_EQ(dev_out[2], expected_dev_2, EPSILON);
    ASSERT_EQ(hit_out[2], (fabs(expected_dev_2) > 2.0) ? 1 : 0);
}

TEST(test_latarb_signal_threshold) {
    const size_t n = 4;

    double price_fast[] = {100.0, 100.0, 100.0, 100.0};
    double price_slow[] = {123.0, 122.5, 122.0, 121.0};
    double coef_a[] = {1.2, 1.2, 1.2, 1.2};
    double coef_b[] = {2.0, 2.0, 2.0, 2.0};
    double theta[] = {1.0, 0.5, 0.1, 0.0};

    double dev_out[4];
    int hit_out[4];

    fc_status_t status = fc_ex_strat_latarb_signal(
        dev_out,
        hit_out,
        price_fast,
        price_slow,
        coef_a,
        coef_b,
        theta,
        n
    );
    ASSERT_EQ(status, FC_OK);

    // p_exp = 1.2 * 100 + 2 = 122.0
    // dev[0] = 123.0 - 122.0 = 1.0, |dev| = 1.0, theta = 1.0, hit = 0 (not > threshold)
    // dev[1] = 122.5 - 122.0 = 0.5, |dev| = 0.5, theta = 0.5, hit = 0 (not > threshold)
    // dev[2] = 122.0 - 122.0 = 0.0, |dev| = 0.0, theta = 0.1, hit = 0 (not > threshold)
    // dev[3] = 121.0 - 122.0 = -1.0, |dev| = 1.0, theta = 0.0, hit = 0 (theta <= 0, no signal)

    ASSERT_EQ(hit_out[0], 0);
    ASSERT_EQ(hit_out[1], 0);
    ASSERT_EQ(hit_out[2], 0);
    ASSERT_EQ(hit_out[3], 0);
}

TEST(test_latarb_signal_nan_handling) {
    const size_t n = 3;

    double price_fast[] = {100.0, NAN, 150.0};
    double price_slow[] = {125.0, 245.0, NAN};
    double coef_a[] = {1.2, 1.2, 1.2};
    double coef_b[] = {5.0, 5.0, 5.0};
    double theta[] = {2.0, 2.0, 2.0};

    double dev_out[3];
    int hit_out[3];

    fc_status_t status = fc_ex_strat_latarb_signal(
        dev_out,
        hit_out,
        price_fast,
        price_slow,
        coef_a,
        coef_b,
        theta,
        n
    );
    ASSERT_EQ(status, FC_OK);

    ASSERT_TRUE(isnan(dev_out[1]));
    ASSERT_EQ(hit_out[1], 0);

    ASSERT_TRUE(isnan(dev_out[2]));
    ASSERT_EQ(hit_out[2], 0);
}

TEST(test_latarb_signal_null_args) {
    double price_fast[3] = {100.0, 200.0, 150.0};
    double price_slow[3] = {125.0, 245.0, 182.0};
    double coef_a[3] = {1.2, 1.2, 1.2};
    double coef_b[3] = {5.0, 5.0, 5.0};
    double theta[3] = {2.0, 2.0, 2.0};
    double dev_out[3];
    int hit_out[3];

    fc_status_t status = fc_ex_strat_latarb_signal(NULL, hit_out, price_fast, price_slow, coef_a, coef_b, theta, 3);
    ASSERT_EQ(status, FC_ERR_INVALID_ARG);

    status = fc_ex_strat_latarb_signal(dev_out, NULL, price_fast, price_slow, coef_a, coef_b, theta, 3);
    ASSERT_EQ(status, FC_ERR_INVALID_ARG);

    status = fc_ex_strat_latarb_signal(dev_out, hit_out, NULL, price_slow, coef_a, coef_b, theta, 3);
    ASSERT_EQ(status, FC_ERR_INVALID_ARG);

    status = fc_ex_strat_latarb_signal(dev_out, hit_out, price_fast, NULL, coef_a, coef_b, theta, 3);
    ASSERT_EQ(status, FC_ERR_INVALID_ARG);

    status = fc_ex_strat_latarb_signal(dev_out, hit_out, price_fast, price_slow, NULL, coef_b, theta, 3);
    ASSERT_EQ(status, FC_ERR_INVALID_ARG);

    status = fc_ex_strat_latarb_signal(dev_out, hit_out, price_fast, price_slow, coef_a, NULL, theta, 3);
    ASSERT_EQ(status, FC_ERR_INVALID_ARG);

    status = fc_ex_strat_latarb_signal(dev_out, hit_out, price_fast, price_slow, coef_a, coef_b, NULL, 3);
    ASSERT_EQ(status, FC_ERR_INVALID_ARG);
}

TEST(test_latarb_signal_zero_size) {
    double price_fast[1] = {100.0};
    double price_slow[1] = {125.0};
    double coef_a[1] = {1.2};
    double coef_b[1] = {5.0};
    double theta[1] = {2.0};
    double dev_out[1];
    int hit_out[1];

    fc_status_t status = fc_ex_strat_latarb_signal(
        dev_out,
        hit_out,
        price_fast,
        price_slow,
        coef_a,
        coef_b,
        theta,
        0
    );
    ASSERT_EQ(status, FC_ERR_INVALID_ARG);
}

TEST(test_latarb_calibrate_batch) {
    const size_t n_pairs = 100;
    const size_t window = 50;

    double* hist_fast = (double*)malloc(n_pairs * window * sizeof(double));
    double* hist_slow = (double*)malloc(n_pairs * window * sizeof(double));
    double* coef_a_out = (double*)malloc(n_pairs * sizeof(double));
    double* coef_b_out = (double*)malloc(n_pairs * sizeof(double));

    for (size_t i = 0; i < n_pairs; i++) {
        double true_a = 1.0 + 0.01 * i;
        double true_b = 5.0 + 0.1 * i;

        for (size_t j = 0; j < window; j++) {
            hist_fast[i * window + j] = 100.0 + 2.0 * j;
            hist_slow[i * window + j] = true_a * hist_fast[i * window + j] + true_b;
        }
    }

    fc_status_t status = fc_ex_strat_latarb_calibrate(
        coef_a_out,
        coef_b_out,
        hist_fast,
        hist_slow,
        n_pairs,
        window
    );
    ASSERT_EQ(status, FC_OK);

    for (size_t i = 0; i < n_pairs; i++) {
        double expected_a = 1.0 + 0.01 * i;
        double expected_b = 5.0 + 0.1 * i;

        FC_TEST_ASSERT_DOUBLE_EQ(coef_a_out[i], expected_a, 0.001);
        FC_TEST_ASSERT_DOUBLE_EQ(coef_b_out[i], expected_b, 0.1);
    }

    free(hist_fast);
    free(hist_slow);
    free(coef_a_out);
    free(coef_b_out);
}

void register_latency_arb_tests(void) {
    RUN_TEST(test_latarb_calibrate_perfect_linear);
    RUN_TEST(test_latarb_calibrate_with_noise);
    RUN_TEST(test_latarb_calibrate_null_args);
    RUN_TEST(test_latarb_calibrate_invalid_dimensions);
    RUN_TEST(test_latarb_signal_basic);
    RUN_TEST(test_latarb_signal_threshold);
    RUN_TEST(test_latarb_signal_nan_handling);
    RUN_TEST(test_latarb_signal_null_args);
    RUN_TEST(test_latarb_signal_zero_size);
    RUN_TEST(test_latarb_calibrate_batch);
}
