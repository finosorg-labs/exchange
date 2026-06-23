/**
 * @file test_microprice.c
 * @brief Unit tests for micro-price signal computation
 */

#include "signal/microprice.h"
#include "test_framework.h"
#include "platform.h"
#include "error.h"
#include "mem_aligned.h"
#include <math.h>
#include <string.h>

TEST(test_microprice_basic) {
    double mp_out[5];
    double bid_p[5] = {100.0, 100.0, 100.0, 100.0, 100.0};
    double bid_q[5] = {500.0, 800.0, 200.0, 1000.0, 10.0};
    double ask_p[5] = {100.5, 100.5, 100.5, 101.0, 101.0};
    double ask_q[5] = {500.0, 200.0, 800.0, 10.0, 1000.0};

    fc_status_t status = fc_ex_sig_microprice_batch(mp_out, bid_p, bid_q, ask_p, ask_q, 5);

    FC_TEST_ASSERT_EQ(status, FC_OK);
    FC_TEST_ASSERT_DOUBLE_EQ(mp_out[0], 100.25, 1e-10);
    FC_TEST_ASSERT_DOUBLE_EQ(mp_out[1], 100.4, 1e-10);   // More bid weight -> closer to ask
    FC_TEST_ASSERT_DOUBLE_EQ(mp_out[2], 100.1, 1e-10);   // More ask weight -> closer to bid
    FC_TEST_ASSERT_DOUBLE_EQ(mp_out[3], 100.99009900990099, 1e-10);  // Heavy bid weight
    FC_TEST_ASSERT_DOUBLE_EQ(mp_out[4], 100.00990099009901, 1e-10);  // Heavy ask weight
}

TEST(test_microprice_edge_cases) {
    double mp_out[4];
    double bid_p[4] = {100.0, 100.0, 100.0, 100.0};
    double bid_q[4] = {0.0, 0.0, 500.0, -100.0};
    double ask_p[4] = {100.5, 100.5, 100.5, 100.5};
    double ask_q[4] = {0.0, 500.0, 0.0, 50.0};

    fc_status_t status = fc_ex_sig_microprice_batch(mp_out, bid_p, bid_q, ask_p, ask_q, 4);

    FC_TEST_ASSERT_EQ(status, FC_OK);
    ASSERT_TRUE(isnan(mp_out[0]));  // Both zero -> NaN
    ASSERT_FALSE(isnan(mp_out[1])); // bid_q=0, ask_q=500 -> bid_p weighted by ask_q
    FC_TEST_ASSERT_DOUBLE_EQ(mp_out[1], 100.0, 1e-10);
    ASSERT_FALSE(isnan(mp_out[2])); // bid_q=500, ask_q=0 -> ask_p weighted by bid_q
    FC_TEST_ASSERT_DOUBLE_EQ(mp_out[2], 100.5, 1e-10);
    ASSERT_TRUE(isnan(mp_out[3]));  // Negative total -> NaN
}

TEST(test_microprice_null_pointers) {
    double mp_out[1];
    double bid_p[1] = {100.0};
    double bid_q[1] = {500.0};
    double ask_p[1] = {100.5};
    double ask_q[1] = {500.0};

    FC_TEST_ASSERT_EQ(fc_ex_sig_microprice_batch(NULL, bid_p, bid_q, ask_p, ask_q, 1), FC_ERR_INVALID_ARG);
    FC_TEST_ASSERT_EQ(fc_ex_sig_microprice_batch(mp_out, NULL, bid_q, ask_p, ask_q, 1), FC_ERR_INVALID_ARG);
    FC_TEST_ASSERT_EQ(fc_ex_sig_microprice_batch(mp_out, bid_p, NULL, ask_p, ask_q, 1), FC_ERR_INVALID_ARG);
    FC_TEST_ASSERT_EQ(fc_ex_sig_microprice_batch(mp_out, bid_p, bid_q, NULL, ask_q, 1), FC_ERR_INVALID_ARG);
    FC_TEST_ASSERT_EQ(fc_ex_sig_microprice_batch(mp_out, bid_p, bid_q, ask_p, NULL, 1), FC_ERR_INVALID_ARG);
}

TEST(test_microprice_invalid_size) {
    double mp_out[1];
    double bid_p[1] = {100.0};
    double bid_q[1] = {500.0};
    double ask_p[1] = {100.5};
    double ask_q[1] = {500.0};

    FC_TEST_ASSERT_EQ(fc_ex_sig_microprice_batch(mp_out, bid_p, bid_q, ask_p, ask_q, 0), FC_ERR_INVALID_ARG);
}

TEST(test_microprice_numerical_accuracy) {
    double mp_out[3];

    double bid_p[3] = {1000000.0, 0.00001, 100.0};
    double bid_q[3] = {500000.0, 500.0, 0.001};
    double ask_p[3] = {1000000.5, 0.000015, 100.5};
    double ask_q[3] = {500000.0, 500.0, 1000.0};

    fc_status_t status = fc_ex_sig_microprice_batch(mp_out, bid_p, bid_q, ask_p, ask_q, 3);

    FC_TEST_ASSERT_EQ(status, FC_OK);
    FC_TEST_ASSERT_DOUBLE_EQ(mp_out[0], 1000000.25, 1e-6);
    FC_TEST_ASSERT_DOUBLE_EQ(mp_out[1], 0.0000125, 1e-12);
    // mp = (100.0 * 1000.0 + 100.5 * 0.001) / 1000.001 = 100.0000004999995
    FC_TEST_ASSERT_DOUBLE_EQ(mp_out[2], 100.0000004999995, 1e-10);
}

TEST(test_microprice_properties) {
    double mp_out[4];
    double bid_p[4] = {100.0, 100.0, 99.5, 50.0};
    double bid_q[4] = {500.0, 100.0, 800.0, 1000.0};
    double ask_p[4] = {100.5, 101.0, 100.0, 50.1};
    double ask_q[4] = {500.0, 900.0, 200.0, 1.0};

    fc_status_t status = fc_ex_sig_microprice_batch(mp_out, bid_p, bid_q, ask_p, ask_q, 4);

    FC_TEST_ASSERT_EQ(status, FC_OK);

    for (size_t i = 0; i < 4; i++) {
        ASSERT_FALSE(isnan(mp_out[i]));
        ASSERT_TRUE(mp_out[i] >= bid_p[i] && mp_out[i] <= ask_p[i]);
    }
}

TEST(test_microprice_batch_sizes) {
    size_t sizes[] = {1, 7, 8, 15, 16, 31, 32, 100, 1000};
    size_t n_sizes = sizeof(sizes) / sizeof(sizes[0]);

    for (size_t s = 0; s < n_sizes; s++) {
        size_t n = sizes[s];
        double* mp_out = fc_aligned_alloc(n * sizeof(double), 64);
        double* bid_p = fc_aligned_alloc(n * sizeof(double), 64);
        double* bid_q = fc_aligned_alloc(n * sizeof(double), 64);
        double* ask_p = fc_aligned_alloc(n * sizeof(double), 64);
        double* ask_q = fc_aligned_alloc(n * sizeof(double), 64);

        for (size_t i = 0; i < n; i++) {
            bid_p[i] = 100.0 + (double)i * 0.1;
            bid_q[i] = 500.0 + (double)i;
            ask_p[i] = bid_p[i] + 0.5;
            ask_q[i] = 500.0 + (double)(n - i);
        }

        fc_status_t status = fc_ex_sig_microprice_batch(mp_out, bid_p, bid_q, ask_p, ask_q, n);

        FC_TEST_ASSERT_EQ(status, FC_OK);

        for (size_t i = 0; i < n; i++) {
            ASSERT_FALSE(isnan(mp_out[i]));
            ASSERT_TRUE(mp_out[i] >= bid_p[i] && mp_out[i] <= ask_p[i]);
        }

        fc_aligned_free(mp_out);
        fc_aligned_free(bid_p);
        fc_aligned_free(bid_q);
        fc_aligned_free(ask_p);
        fc_aligned_free(ask_q);
    }
}

void register_microprice_tests(void) {
    RUN_TEST(test_microprice_basic);
    RUN_TEST(test_microprice_edge_cases);
    RUN_TEST(test_microprice_null_pointers);
    RUN_TEST(test_microprice_invalid_size);
    RUN_TEST(test_microprice_numerical_accuracy);
    RUN_TEST(test_microprice_properties);
    RUN_TEST(test_microprice_batch_sizes);
}
