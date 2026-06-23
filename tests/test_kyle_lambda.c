/**
 * @file test_kyle_lambda.c
 * @brief Unit tests for Kyle's Lambda computation
 */

#include "signal/kyle_lambda.h"
#include "test_framework.h"
#include <math.h>
#include <string.h>

/* ========================================================================
 * Basic Functionality Tests
 * ======================================================================== */

TEST(test_kyle_lambda_basic) {
    /* Simple test case with known correlation */
    const size_t n_symbols = 1;
    const size_t window = 5;

    /* Price changes: [1, 2, 3, 4, 5] */
    double dprice[] = {1.0, 2.0, 3.0, 4.0, 5.0};

    /* Volume: [2, 4, 6, 8, 10] - perfectly correlated with dprice */
    double volume[] = {2.0, 4.0, 6.0, 8.0, 10.0};

    double lambda;
    fc_status_t status = fc_ex_sig_kyle_lambda_batch(&lambda, dprice, volume, n_symbols, window);

    FC_TEST_ASSERT_EQ(status, FC_OK);

    /* Expected: λ = Cov(ΔP, V) / Var(V)
     * Since V = 2 * ΔP (perfect linear relation),
     * Cov(ΔP, V) = Cov(ΔP, 2*ΔP) = 2 * Var(ΔP)
     * λ = 2 * Var(ΔP) / Var(2*ΔP) = 2 * Var(ΔP) / (4 * Var(ΔP)) = 0.5
     */
    FC_TEST_ASSERT_DOUBLE_EQ(lambda, 0.5, 1e-10);
}

TEST(test_kyle_lambda_multiple_symbols) {
    /* Test batch processing with multiple symbols */
    const size_t n_symbols = 3;
    const size_t window = 4;

    /* Symbol 0: positive correlation */
    /* Symbol 1: negative correlation */
    /* Symbol 2: no correlation (zero covariance) */
    double dprice[] = {
        1.0, 2.0, 3.0, 4.0,   /* Symbol 0 */
        1.0, 2.0, 3.0, 4.0,   /* Symbol 1 */
        1.0, -1.0, 1.0, -1.0  /* Symbol 2 */
    };

    double volume[] = {
        2.0, 4.0, 6.0, 8.0,   /* Symbol 0: V = 2*ΔP */
        8.0, 6.0, 4.0, 2.0,   /* Symbol 1: V = 10-2*ΔP (negative) */
        5.0, 5.0, 5.0, 5.0    /* Symbol 2: constant volume */
    };

    double lambda[3];
    fc_status_t status = fc_ex_sig_kyle_lambda_batch(lambda, dprice, volume, n_symbols, window);

    FC_TEST_ASSERT_EQ(status, FC_OK);

    /* Symbol 0: positive lambda ~0.5 */
    FC_TEST_ASSERT_DOUBLE_EQ(lambda[0], 0.5, 1e-10);

    /* Symbol 1: negative lambda (negative correlation) */
    FC_TEST_ASSERT(lambda[1] < 0.0);

    /* Symbol 2: zero lambda (constant volume => zero variance) */
    FC_TEST_ASSERT_DOUBLE_EQ(lambda[2], 0.0, 1e-10);
}

TEST(test_kyle_lambda_zero_variance) {
    /* Test with zero volume variance (constant volume) */
    const size_t n_symbols = 1;
    const size_t window = 4;

    double dprice[] = {1.0, 2.0, 3.0, 4.0};
    double volume[] = {5.0, 5.0, 5.0, 5.0};  /* Constant volume */

    double lambda;
    fc_status_t status = fc_ex_sig_kyle_lambda_batch(&lambda, dprice, volume, n_symbols, window);

    FC_TEST_ASSERT_EQ(status, FC_OK);
    FC_TEST_ASSERT_DOUBLE_EQ(lambda, 0.0, 1e-10);
}

TEST(test_kyle_lambda_realistic_data) {
    /* Test with more realistic price impact scenario */
    const size_t n_symbols = 1;
    const size_t window = 10;

    /* Simulated price changes (in basis points) */
    double dprice[] = {0.5, -0.3, 0.8, -0.2, 0.6, -0.4, 0.7, -0.1, 0.4, -0.5};

    /* Simulated volumes (signed by trade direction) */
    double volume[] = {100.0, -80.0, 150.0, -50.0, 120.0, -90.0, 140.0, -30.0, 110.0, -100.0};

    double lambda;
    fc_status_t status = fc_ex_sig_kyle_lambda_batch(&lambda, dprice, volume, n_symbols, window);

    FC_TEST_ASSERT_EQ(status, FC_OK);

    /* Lambda should be positive (price increases with buy volume) */
    FC_TEST_ASSERT(lambda > 0.0);

    /* Reasonable magnitude check (typically 1e-4 to 1e-2 for liquid markets) */
    FC_TEST_ASSERT(lambda < 0.1);
}

/* ========================================================================
 * Edge Cases and Error Handling
 * ======================================================================== */

TEST(test_kyle_lambda_null_inputs) {
    const size_t n_symbols = 1;
    const size_t window = 4;
    double dprice[] = {1.0, 2.0, 3.0, 4.0};
    double volume[] = {2.0, 4.0, 6.0, 8.0};
    double lambda;

    /* NULL lambda_out */
    fc_status_t status = fc_ex_sig_kyle_lambda_batch(NULL, dprice, volume, n_symbols, window);
    FC_TEST_ASSERT_EQ(status, FC_ERR_INVALID_ARG);

    /* NULL dprice */
    status = fc_ex_sig_kyle_lambda_batch(&lambda, NULL, volume, n_symbols, window);
    FC_TEST_ASSERT_EQ(status, FC_ERR_INVALID_ARG);

    /* NULL volume */
    status = fc_ex_sig_kyle_lambda_batch(&lambda, dprice, NULL, n_symbols, window);
    FC_TEST_ASSERT_EQ(status, FC_ERR_INVALID_ARG);
}

TEST(test_kyle_lambda_invalid_dimensions) {
    double dprice[] = {1.0, 2.0, 3.0, 4.0};
    double volume[] = {2.0, 4.0, 6.0, 8.0};
    double lambda;

    /* Zero symbols */
    fc_status_t status = fc_ex_sig_kyle_lambda_batch(&lambda, dprice, volume, 0, 4);
    FC_TEST_ASSERT_EQ(status, FC_ERR_INVALID_ARG);

    /* Window too small (< 2) */
    status = fc_ex_sig_kyle_lambda_batch(&lambda, dprice, volume, 1, 1);
    FC_TEST_ASSERT_EQ(status, FC_ERR_INVALID_ARG);

    status = fc_ex_sig_kyle_lambda_batch(&lambda, dprice, volume, 1, 0);
    FC_TEST_ASSERT_EQ(status, FC_ERR_INVALID_ARG);
}

TEST(test_kyle_lambda_nan_input) {
    const size_t n_symbols = 1;
    const size_t window = 4;
    double lambda;

    /* NaN in dprice */
    double dprice_nan[] = {1.0, NAN, 3.0, 4.0};
    double volume[] = {2.0, 4.0, 6.0, 8.0};
    fc_status_t status = fc_ex_sig_kyle_lambda_batch(&lambda, dprice_nan, volume, n_symbols, window);
    FC_TEST_ASSERT_EQ(status, FC_ERR_NAN_INPUT);

    /* NaN in volume */
    double dprice[] = {1.0, 2.0, 3.0, 4.0};
    double volume_nan[] = {2.0, NAN, 6.0, 8.0};
    status = fc_ex_sig_kyle_lambda_batch(&lambda, dprice, volume_nan, n_symbols, window);
    FC_TEST_ASSERT_EQ(status, FC_ERR_NAN_INPUT);
}

TEST(test_kyle_lambda_minimum_window) {
    /* Test with minimum valid window size (2) */
    const size_t n_symbols = 1;
    const size_t window = 2;

    double dprice[] = {1.0, 2.0};
    double volume[] = {2.0, 4.0};

    double lambda;
    fc_status_t status = fc_ex_sig_kyle_lambda_batch(&lambda, dprice, volume, n_symbols, window);

    FC_TEST_ASSERT_EQ(status, FC_OK);
    /* With only 2 points and perfect correlation, lambda should be 0.5 */
    FC_TEST_ASSERT_DOUBLE_EQ(lambda, 0.5, 1e-10);
}

TEST(test_kyle_lambda_large_window) {
    /* Test with larger window to verify no overflow issues */
    const size_t n_symbols = 1;
    const size_t window = 100;

    double dprice[100];
    double volume[100];

    /* Generate synthetic data with known relationship */
    for (size_t i = 0; i < window; i++) {
        dprice[i] = sin(i * 0.1);
        volume[i] = 2.0 * dprice[i] + cos(i * 0.05);  /* Linear + noise */
    }

    double lambda;
    fc_status_t status = fc_ex_sig_kyle_lambda_batch(&lambda, dprice, volume, n_symbols, window);

    FC_TEST_ASSERT_EQ(status, FC_OK);
    /* Lambda should be positive and reasonable */
    FC_TEST_ASSERT(lambda > 0.0);
    FC_TEST_ASSERT(lambda < 10.0);
}

TEST(test_kyle_lambda_negative_correlation) {
    /* Test with negative price-volume correlation */
    const size_t n_symbols = 1;
    const size_t window = 5;

    double dprice[] = {1.0, 2.0, 3.0, 4.0, 5.0};
    double volume[] = {10.0, 8.0, 6.0, 4.0, 2.0};  /* Inverse relationship */

    double lambda;
    fc_status_t status = fc_ex_sig_kyle_lambda_batch(&lambda, dprice, volume, n_symbols, window);

    FC_TEST_ASSERT_EQ(status, FC_OK);
    FC_TEST_ASSERT(lambda < 0.0);  /* Negative lambda */
}

TEST(test_kyle_lambda_zero_mean) {
    /* Test with zero-mean price changes */
    const size_t n_symbols = 1;
    const size_t window = 6;

    double dprice[] = {-2.0, -1.0, 0.0, 0.0, 1.0, 2.0};  /* Mean = 0 */
    double volume[] = {-4.0, -2.0, 0.0, 0.0, 2.0, 4.0};  /* Mean = 0, V = 2*ΔP */

    double lambda;
    fc_status_t status = fc_ex_sig_kyle_lambda_batch(&lambda, dprice, volume, n_symbols, window);

    FC_TEST_ASSERT_EQ(status, FC_OK);
    FC_TEST_ASSERT_DOUBLE_EQ(lambda, 0.5, 1e-10);
}

/* ========================================================================
 * Numerical Precision Tests
 * ======================================================================== */

TEST(test_kyle_lambda_numerical_stability) {
    /* Test numerical stability with very small values */
    const size_t n_symbols = 1;
    const size_t window = 5;

    double dprice[] = {1e-6, 2e-6, 3e-6, 4e-6, 5e-6};
    double volume[] = {2e-6, 4e-6, 6e-6, 8e-6, 10e-6};

    double lambda;
    fc_status_t status = fc_ex_sig_kyle_lambda_batch(&lambda, dprice, volume, n_symbols, window);

    FC_TEST_ASSERT_EQ(status, FC_OK);
    /* Despite small values, lambda should still be ~0.5 */
    FC_TEST_ASSERT_DOUBLE_EQ(lambda, 0.5, 1e-6);
}

TEST(test_kyle_lambda_large_values) {
    /* Test with large values to check overflow handling */
    const size_t n_symbols = 1;
    const size_t window = 5;

    double dprice[] = {1e6, 2e6, 3e6, 4e6, 5e6};
    double volume[] = {2e6, 4e6, 6e6, 8e6, 10e6};

    double lambda;
    fc_status_t status = fc_ex_sig_kyle_lambda_batch(&lambda, dprice, volume, n_symbols, window);

    FC_TEST_ASSERT_EQ(status, FC_OK);
    FC_TEST_ASSERT_DOUBLE_EQ(lambda, 0.5, 1e-10);
}

/* ========================================================================
 * Performance Sanity Tests
 * ======================================================================== */

TEST(test_kyle_lambda_many_symbols) {
    /* Test batch processing with many symbols */
    const size_t n_symbols = 100;
    const size_t window = 50;

    double* dprice = (double*)malloc( n_symbols * window * sizeof(double));
    double* volume = (double*)malloc( n_symbols * window * sizeof(double));
    double* lambda = (double*)malloc( n_symbols * sizeof(double));

    FC_TEST_ASSERT_NOT_NULL(dprice);
    FC_TEST_ASSERT_NOT_NULL(volume);
    FC_TEST_ASSERT_NOT_NULL(lambda);

    /* Initialize with varying patterns */
    for (size_t i = 0; i < n_symbols; i++) {
        for (size_t j = 0; j < window; j++) {
            size_t idx = i * window + j;
            dprice[idx] = sin(j * 0.1 + i * 0.01);
            volume[idx] = cos(j * 0.1 + i * 0.01) * (i + 1);
        }
    }

    fc_status_t status = fc_ex_sig_kyle_lambda_batch(lambda, dprice, volume, n_symbols, window);

    FC_TEST_ASSERT_EQ(status, FC_OK);

    /* Verify all lambdas are finite */
    for (size_t i = 0; i < n_symbols; i++) {
        FC_TEST_ASSERT(!isnan(lambda[i]));
        FC_TEST_ASSERT(!isinf(lambda[i]));
    }

    free(dprice);
    free(volume);
    free(lambda);
}

/* Register all Kyle's Lambda tests */
void register_kyle_lambda_tests(void) {
    RUN_TEST(test_kyle_lambda_basic);
    RUN_TEST(test_kyle_lambda_multiple_symbols);
    RUN_TEST(test_kyle_lambda_zero_variance);
    RUN_TEST(test_kyle_lambda_realistic_data);
    RUN_TEST(test_kyle_lambda_null_inputs);
    RUN_TEST(test_kyle_lambda_invalid_dimensions);
    RUN_TEST(test_kyle_lambda_nan_input);
    RUN_TEST(test_kyle_lambda_minimum_window);
    RUN_TEST(test_kyle_lambda_large_window);
    RUN_TEST(test_kyle_lambda_negative_correlation);
    RUN_TEST(test_kyle_lambda_zero_mean);
    RUN_TEST(test_kyle_lambda_numerical_stability);
    RUN_TEST(test_kyle_lambda_large_values);
    RUN_TEST(test_kyle_lambda_many_symbols);
}
