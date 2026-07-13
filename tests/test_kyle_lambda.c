/**
 * @file test_kyle_lambda.c
 * @brief Unit tests for Kyle's Lambda computation
 */

#include "signal/kyle_lambda.h"
#include "test_framework.h"
#include <math.h>
#include <string.h>

/*
 * Basic Functionality Tests
 *  */

TEST(test_kyle_lambda_basic) {
    /* Simple test case with known correlation */
    const size_t n_symbols = 1;
    const size_t window    = 5;

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
    const size_t window    = 4;

    /* Symbol 0: positive correlation */
    /* Symbol 1: negative correlation */
    /* Symbol 2: no correlation (zero covariance) */
    double dprice[] = {
        1.0,
        2.0,
        3.0,
        4.0, /* Symbol 0 */
        1.0,
        2.0,
        3.0,
        4.0, /* Symbol 1 */
        1.0,
        -1.0,
        1.0,
        -1.0 /* Symbol 2 */
    };

    double volume[] = {
        2.0,
        4.0,
        6.0,
        8.0, /* Symbol 0: V = 2*ΔP */
        8.0,
        6.0,
        4.0,
        2.0, /* Symbol 1: V = 10-2*ΔP (negative) */
        5.0,
        5.0,
        5.0,
        5.0 /* Symbol 2: constant volume */
    };

    double lambda[3]   = {0.0};
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
    const size_t window    = 4;

    double dprice[] = {1.0, 2.0, 3.0, 4.0};
    double volume[] = {5.0, 5.0, 5.0, 5.0}; /* Constant volume */

    double lambda;
    fc_status_t status = fc_ex_sig_kyle_lambda_batch(&lambda, dprice, volume, n_symbols, window);

    FC_TEST_ASSERT_EQ(status, FC_OK);
    FC_TEST_ASSERT_DOUBLE_EQ(lambda, 0.0, 1e-10);
}

TEST(test_kyle_lambda_realistic_data) {
    /* Test with more realistic price impact scenario */
    const size_t n_symbols = 1;
    const size_t window    = 10;

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

/*
 * Edge Cases and Error Handling
 *  */

TEST(test_kyle_lambda_null_inputs) {
    const size_t n_symbols = 1;
    const size_t window    = 4;
    double dprice[]        = {1.0, 2.0, 3.0, 4.0};
    double volume[]        = {2.0, 4.0, 6.0, 8.0};
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
    const size_t window    = 4;
    double lambda;

    /* NaN in dprice */
    double dprice_nan[] = {1.0, NAN, 3.0, 4.0};
    double volume[]     = {2.0, 4.0, 6.0, 8.0};
    fc_status_t status =
        fc_ex_sig_kyle_lambda_batch(&lambda, dprice_nan, volume, n_symbols, window);
    FC_TEST_ASSERT_EQ(status, FC_ERR_NAN_INPUT);

    /* NaN in volume */
    double dprice[]     = {1.0, 2.0, 3.0, 4.0};
    double volume_nan[] = {2.0, NAN, 6.0, 8.0};
    status = fc_ex_sig_kyle_lambda_batch(&lambda, dprice, volume_nan, n_symbols, window);
    FC_TEST_ASSERT_EQ(status, FC_ERR_NAN_INPUT);
}

TEST(test_kyle_lambda_minimum_window) {
    /* Test with minimum valid window size (2) */
    const size_t n_symbols = 1;
    const size_t window    = 2;

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
    const size_t window    = 100;

    double dprice[100];
    double volume[100];

    /* Generate synthetic data with known relationship */
    for (size_t i = 0; i < window; i++) {
        dprice[i] = sin(i * 0.1);
        volume[i] = 2.0 * dprice[i] + cos(i * 0.05); /* Linear + noise */
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
    const size_t window    = 5;

    double dprice[] = {1.0, 2.0, 3.0, 4.0, 5.0};
    double volume[] = {10.0, 8.0, 6.0, 4.0, 2.0}; /* Inverse relationship */

    double lambda;
    fc_status_t status = fc_ex_sig_kyle_lambda_batch(&lambda, dprice, volume, n_symbols, window);

    FC_TEST_ASSERT_EQ(status, FC_OK);
    FC_TEST_ASSERT(lambda < 0.0); /* Negative lambda */
}

TEST(test_kyle_lambda_zero_mean) {
    /* Test with zero-mean price changes */
    const size_t n_symbols = 1;
    const size_t window    = 6;

    double dprice[] = {-2.0, -1.0, 0.0, 0.0, 1.0, 2.0}; /* Mean = 0 */
    double volume[] = {-4.0, -2.0, 0.0, 0.0, 2.0, 4.0}; /* Mean = 0, V = 2*ΔP */

    double lambda;
    fc_status_t status = fc_ex_sig_kyle_lambda_batch(&lambda, dprice, volume, n_symbols, window);

    FC_TEST_ASSERT_EQ(status, FC_OK);
    FC_TEST_ASSERT_DOUBLE_EQ(lambda, 0.5, 1e-10);
}

/*
 * Numerical Precision Tests
 *  */

TEST(test_kyle_lambda_numerical_stability) {
    /* Test numerical stability with very small values */
    const size_t n_symbols = 1;
    const size_t window    = 5;

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
    const size_t window    = 5;

    double dprice[] = {1e6, 2e6, 3e6, 4e6, 5e6};
    double volume[] = {2e6, 4e6, 6e6, 8e6, 10e6};

    double lambda;
    fc_status_t status = fc_ex_sig_kyle_lambda_batch(&lambda, dprice, volume, n_symbols, window);

    FC_TEST_ASSERT_EQ(status, FC_OK);
    FC_TEST_ASSERT_DOUBLE_EQ(lambda, 0.5, 1e-10);
}

/*
 * Performance Sanity Tests
 *  */

TEST(test_kyle_lambda_many_symbols) {
    /* Test batch processing with many symbols */
    const size_t n_symbols = 100;
    const size_t window    = 50;

    double* dprice = (double*) malloc(n_symbols * window * sizeof(double));
    double* volume = (double*) malloc(n_symbols * window * sizeof(double));
    double* lambda = (double*) malloc(n_symbols * sizeof(double));

    if (!dprice || !volume || !lambda) {
        free(dprice);
        free(volume);
        free(lambda);
        FC_TEST_ASSERT_NOT_NULL(NULL);
    }

    /* Initialize with varying patterns */
    for (size_t i = 0; i < n_symbols; i++) {
        for (size_t j = 0; j < window; j++) {
            size_t idx  = i * window + j;
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

/*
 * OLS Regression Method Tests
 *  */

TEST(test_kyle_lambda_ols_basic) {
    /* Test OLS method with same data as basic covariance test */
    const size_t n_symbols = 1;
    const size_t window    = 5;

    double dprice[] = {1.0, 2.0, 3.0, 4.0, 5.0};
    double volume[] = {2.0, 4.0, 6.0, 8.0, 10.0};

    double lambda, r_squared, std_error;
    fc_status_t status = fc_ex_sig_kyle_lambda_ols(
        &lambda, &r_squared, &std_error, NULL, dprice, volume, n_symbols, window
    );

    FC_TEST_ASSERT_EQ(status, FC_OK);

    /* Lambda should be 0.5 (same as covariance method) */
    FC_TEST_ASSERT_DOUBLE_EQ(lambda, 0.5, 1e-10);

    /* Perfect linear fit, R² should be 1.0 */
    FC_TEST_ASSERT_DOUBLE_EQ(r_squared, 1.0, 1e-10);

    /* Perfect fit means zero residual error */
    FC_TEST_ASSERT(std_error < 1e-10);
}

TEST(test_kyle_lambda_ols_statistics) {
    /* Test with realistic data that has noise */
    const size_t n_symbols = 1;
    const size_t window    = 10;

    double dprice[] = {0.5, -0.3, 0.8, -0.2, 0.6, -0.4, 0.7, -0.1, 0.4, -0.5};
    double volume[] = {100.0, -80.0, 150.0, -50.0, 120.0, -90.0, 140.0, -30.0, 110.0, -100.0};

    double lambda, r_squared, std_error;
    fc_status_t status = fc_ex_sig_kyle_lambda_ols(
        &lambda, &r_squared, &std_error, NULL, dprice, volume, n_symbols, window
    );

    FC_TEST_ASSERT_EQ(status, FC_OK);

    /* Lambda should be positive (price increases with buy volume) */
    FC_TEST_ASSERT(lambda > 0.0);

    /* R² should be between 0 and 1 */
    FC_TEST_ASSERT(r_squared >= 0.0 && r_squared <= 1.0);

    /* Standard error should be positive and finite */
    FC_TEST_ASSERT(std_error > 0.0);
    FC_TEST_ASSERT(!isinf(std_error));
}

TEST(test_kyle_lambda_ols_vs_covariance) {
    /* Verify OLS and covariance methods give same lambda
     * OLS uses regression with intercept (ΔP = α + λ·V + ε), where the slope coefficient λ
     * is mathematically equivalent to Cov(ΔP, V) / Var(V), so results should match.
     */
    const size_t n_symbols = 3;
    const size_t window    = 20;

    double dprice[60];
    double volume[60];

    /* Generate test data */
    for (size_t i = 0; i < n_symbols; i++) {
        for (size_t j = 0; j < window; j++) {
            size_t idx  = i * window + j;
            dprice[idx] = sin(j * 0.1 + i * 0.5);
            volume[idx] = cos(j * 0.15 + i * 0.3) * (100.0 + i * 10.0);
        }
    }

    double lambda_cov[3] = {0.0}, lambda_ols[3] = {0.0};

    /* Compute with both methods */
    fc_status_t status1 =
        fc_ex_sig_kyle_lambda_batch(lambda_cov, dprice, volume, n_symbols, window);

    fc_status_t status2 =
        fc_ex_sig_kyle_lambda_ols(lambda_ols, NULL, NULL, NULL, dprice, volume, n_symbols, window);

    FC_TEST_ASSERT_EQ(status1, FC_OK);
    FC_TEST_ASSERT_EQ(status2, FC_OK);

    /* Lambda values should match within numerical tolerance
     * Both compute the same statistic via different methods:
     * - Covariance: λ = Cov(ΔP, V) / Var(V)
     * - OLS: QR decomposition with fit_intercept=1, slope coefficient λ
     * The slope in regression with intercept equals Cov(ΔP, V) / Var(V) mathematically.
     * Numerical differences due to different algorithms, but should be small.
     */
    for (size_t i = 0; i < n_symbols; i++) {
        FC_TEST_ASSERT_DOUBLE_EQ(lambda_cov[i], lambda_ols[i], 1e-10);
    }
}

TEST(test_kyle_lambda_ols_residuals) {
    /* Test residuals output */
    const size_t n_symbols = 2;
    const size_t window    = 5;

    /* Symbol 0: perfect fit (V = 2*ΔP) */
    /* Symbol 1: noisy fit */
    double dprice[] = {
        1.0,
        2.0,
        3.0,
        4.0,
        5.0, /* Symbol 0 */
        1.0,
        2.1,
        2.9,
        4.2,
        4.8 /* Symbol 1 (with noise) */
    };
    double volume[] = {
        2.0,
        4.0,
        6.0,
        8.0,
        10.0, /* Symbol 0 */
        2.0,
        4.0,
        6.0,
        8.0,
        10.0 /* Symbol 1 */
    };

    double lambda[2] = {0.0}, r_squared[2] = {0.0};
    double residuals[10] = {0.0};

    fc_status_t status = fc_ex_sig_kyle_lambda_ols(
        lambda, r_squared, NULL, residuals, dprice, volume, n_symbols, window
    );

    FC_TEST_ASSERT_EQ(status, FC_OK);

    /* Symbol 0: perfect fit, residuals should be near zero */
    for (size_t j = 0; j < window; j++) {
        FC_TEST_ASSERT(fabs(residuals[j]) < 1e-10);
    }
    FC_TEST_ASSERT_DOUBLE_EQ(r_squared[0], 1.0, 1e-10);

    /* Symbol 1: noisy fit, residuals should be non-zero */
    double residual_sum = 0.0;
    for (size_t j = 0; j < window; j++) {
        residual_sum += fabs(residuals[window + j]);
    }
    FC_TEST_ASSERT(residual_sum > 1e-6); /* Has non-trivial residuals */
    FC_TEST_ASSERT(r_squared[1] < 1.0);  /* Not perfect fit */
    FC_TEST_ASSERT(r_squared[1] > 0.9);  /* But still strong correlation */
}

/*
 * Extended API Tests (Validity Flags + Workspace)
 */

TEST(test_kyle_lambda_ext_validity_flags) {
    /* Test validity flags with mixed valid/invalid symbols */
    const size_t n_symbols = 3;
    const size_t window    = 4;

    /* Symbol 0: valid (normal data) */
    /* Symbol 1: invalid (zero variance) */
    /* Symbol 2: valid (normal data) */
    double dprice[] = {
        1.0,
        2.0,
        3.0,
        4.0, /* Symbol 0 */
        1.0,
        2.0,
        3.0,
        4.0, /* Symbol 1 */
        -1.0,
        1.0,
        -1.0,
        1.0 /* Symbol 2 */
    };

    double volume[] = {
        2.0,
        4.0,
        6.0,
        8.0, /* Symbol 0: varying */
        5.0,
        5.0,
        5.0,
        5.0, /* Symbol 1: constant (zero variance) */
        3.0,
        3.0,
        3.0,
        3.0 /* Symbol 2: constant (zero variance) */
    };

    double lambda[3] = {0.0};
    bool valid[3];

    fc_status_t status =
        fc_ex_sig_kyle_lambda_batch_ext(lambda, valid, dprice, volume, n_symbols, window, NULL, 0);

    FC_TEST_ASSERT_EQ(status, FC_OK);

    /* Symbol 0: valid, should have lambda ~0.5 */
    FC_TEST_ASSERT(valid[0]);
    FC_TEST_ASSERT_DOUBLE_EQ(lambda[0], 0.5, 1e-10);

    /* Symbol 1: invalid (zero variance), lambda should be 0.0 */
    FC_TEST_ASSERT(!valid[1]);
    FC_TEST_ASSERT_DOUBLE_EQ(lambda[1], 0.0, 1e-10);

    /* Symbol 2: invalid (zero variance), lambda should be 0.0 */
    FC_TEST_ASSERT(!valid[2]);
    FC_TEST_ASSERT_DOUBLE_EQ(lambda[2], 0.0, 1e-10);
}

TEST(test_kyle_lambda_ext_workspace_zero_allocation) {
    /* Test zero-allocation path with caller-provided workspace */
    const size_t n_symbols = 2;
    const size_t window    = 10;

    double dprice[] = {
        1.0, 2.0,  3.0, 4.0,  5.0, 6.0,  7.0, 8.0,  9.0, 10.0, /* Symbol 0 */
        0.5, -0.3, 0.8, -0.2, 0.6, -0.4, 0.7, -0.1, 0.4, -0.5  /* Symbol 1 */
    };
    double volume[] = {
        2.0,   4.0,   6.0,   8.0,   10.0,  12.0,  14.0,  16.0,  18.0,  20.0,  /* Symbol 0 */
        100.0, -80.0, 150.0, -50.0, 120.0, -90.0, 140.0, -30.0, 110.0, -100.0 /* Symbol 1 */
    };

    /* Allocate workspace */
    size_t ws_size    = fc_ex_sig_kyle_lambda_workspace_size(window);
    double* workspace = (double*) malloc(ws_size);
    FC_TEST_ASSERT_NOT_NULL(workspace);

    double lambda[2];
    bool valid[2];

    /* Call with workspace (zero heap allocation) */
    fc_status_t status = fc_ex_sig_kyle_lambda_batch_ext(
        lambda, valid, dprice, volume, n_symbols, window, workspace, ws_size
    );

    FC_TEST_ASSERT_EQ(status, FC_OK);

    /* Both symbols should be valid */
    FC_TEST_ASSERT(valid[0]);
    FC_TEST_ASSERT(valid[1]);

    /* Symbol 0: perfect correlation, lambda = 0.5 */
    FC_TEST_ASSERT_DOUBLE_EQ(lambda[0], 0.5, 1e-10);

    /* Symbol 1: positive lambda */
    FC_TEST_ASSERT(lambda[1] > 0.0);

    free(workspace);
}

TEST(test_kyle_lambda_ext_insufficient_workspace) {
    /* Test error handling when workspace is too small */
    const size_t n_symbols = 1;
    const size_t window    = 10;

    double dprice[] = {1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0};
    double volume[] = {2.0, 4.0, 6.0, 8.0, 10.0, 12.0, 14.0, 16.0, 18.0, 20.0};

    double lambda;
    bool valid;

    /* Allocate insufficient workspace (only half required size) */
    size_t required_size    = fc_ex_sig_kyle_lambda_workspace_size(window);
    size_t small_size       = required_size / 2;
    double* small_workspace = (double*) malloc(small_size);

    fc_status_t status = fc_ex_sig_kyle_lambda_batch_ext(
        &lambda, &valid, dprice, volume, n_symbols, window, small_workspace, small_size
    );

    /* Should return workspace too small error */
    FC_TEST_ASSERT_EQ(status, FC_ERR_WORKSPACE_TOO_SMALL);

    free(small_workspace);
}

TEST(test_kyle_lambda_ext_backward_compatibility) {
    /* Verify that original API and extended API give same results */
    const size_t n_symbols = 3;
    const size_t window    = 20;

    double dprice[60];
    double volume[60];

    /* Generate test data */
    for (size_t i = 0; i < n_symbols; i++) {
        for (size_t j = 0; j < window; j++) {
            size_t idx  = i * window + j;
            dprice[idx] = sin(j * 0.1 + i * 0.5);
            volume[idx] = cos(j * 0.15 + i * 0.3) * (100.0 + i * 10.0);
        }
    }

    double lambda_orig[3] = {0.0}, lambda_ext[3] = {0.0};
    bool valid[3];

    /* Call original API */
    fc_status_t status1 =
        fc_ex_sig_kyle_lambda_batch(lambda_orig, dprice, volume, n_symbols, window);

    /* Call extended API without workspace */
    fc_status_t status2 = fc_ex_sig_kyle_lambda_batch_ext(
        lambda_ext, valid, dprice, volume, n_symbols, window, NULL, 0
    );

    FC_TEST_ASSERT_EQ(status1, FC_OK);
    FC_TEST_ASSERT_EQ(status2, FC_OK);

    /* Results should match exactly */
    for (size_t i = 0; i < n_symbols; i++) {
        FC_TEST_ASSERT(valid[i]); /* All should be valid */
        FC_TEST_ASSERT_DOUBLE_EQ(lambda_orig[i], lambda_ext[i], 1e-15);
    }
}

TEST(test_kyle_lambda_ols_ext_validity_flags) {
    /* Test OLS extended API with validity flags */
    const size_t n_symbols = 2;
    const size_t window    = 5;

    /* Symbol 0: valid (normal data) */
    /* Symbol 1: valid (normal data) */
    double dprice[] = {
        1.0,
        2.0,
        3.0,
        4.0,
        5.0, /* Symbol 0 */
        1.0,
        2.1,
        2.9,
        4.2,
        4.8 /* Symbol 1 (with noise) */
    };
    double volume[] = {
        2.0,
        4.0,
        6.0,
        8.0,
        10.0, /* Symbol 0 */
        2.0,
        4.0,
        6.0,
        8.0,
        10.0 /* Symbol 1 */
    };

    double lambda[2] = {0.0}, r_squared[2] = {0.0};
    bool valid[2];

    fc_status_t status = fc_ex_sig_kyle_lambda_ols_ext(
        lambda, valid, r_squared, NULL, NULL, dprice, volume, n_symbols, window, NULL, 0
    );

    FC_TEST_ASSERT_EQ(status, FC_OK);

    /* Both should be valid */
    FC_TEST_ASSERT(valid[0]);
    FC_TEST_ASSERT(valid[1]);

    /* Symbol 0: perfect fit */
    FC_TEST_ASSERT_DOUBLE_EQ(lambda[0], 0.5, 1e-10);
    FC_TEST_ASSERT_DOUBLE_EQ(r_squared[0], 1.0, 1e-10);

    /* Symbol 1: good but not perfect fit */
    FC_TEST_ASSERT(lambda[1] > 0.0);
    FC_TEST_ASSERT(r_squared[1] > 0.9);
    FC_TEST_ASSERT(r_squared[1] < 1.0);
}

TEST(test_kyle_lambda_ols_ext_workspace) {
    /* Test OLS with caller-provided workspace */
    const size_t n_symbols = 1;
    const size_t window    = 10;

    double dprice[] = {0.5, -0.3, 0.8, -0.2, 0.6, -0.4, 0.7, -0.1, 0.4, -0.5};
    double volume[] = {100.0, -80.0, 150.0, -50.0, 120.0, -90.0, 140.0, -30.0, 110.0, -100.0};

    /* Allocate workspace */
    size_t ws_size    = fc_ex_sig_kyle_lambda_ols_workspace_size(window);
    double* workspace = (double*) malloc(ws_size);
    FC_TEST_ASSERT_NOT_NULL(workspace);

    double lambda, r_squared;
    bool valid;

    fc_status_t status = fc_ex_sig_kyle_lambda_ols_ext(
        &lambda,
        &valid,
        &r_squared,
        NULL,
        NULL,
        dprice,
        volume,
        n_symbols,
        window,
        workspace,
        ws_size
    );

    FC_TEST_ASSERT_EQ(status, FC_OK);
    FC_TEST_ASSERT(valid);
    FC_TEST_ASSERT(lambda > 0.0);
    FC_TEST_ASSERT(r_squared >= 0.0 && r_squared <= 1.0);

    free(workspace);
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
    RUN_TEST(test_kyle_lambda_ols_basic);
    RUN_TEST(test_kyle_lambda_ols_statistics);
    RUN_TEST(test_kyle_lambda_ols_vs_covariance);
    RUN_TEST(test_kyle_lambda_ols_residuals);
    RUN_TEST(test_kyle_lambda_ext_validity_flags);
    RUN_TEST(test_kyle_lambda_ext_workspace_zero_allocation);
    RUN_TEST(test_kyle_lambda_ext_insufficient_workspace);
    RUN_TEST(test_kyle_lambda_ext_backward_compatibility);
    RUN_TEST(test_kyle_lambda_ols_ext_validity_flags);
    RUN_TEST(test_kyle_lambda_ols_ext_workspace);
}
