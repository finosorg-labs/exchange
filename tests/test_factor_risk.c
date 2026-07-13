/**
 * @file test_factor_risk.c
 * @brief Unit tests for factor risk decomposition
 *
 * Tests include:
 * - Portfolio factor exposures calculation
 * - Factor risk calculation
 * - Specific risk calculation
 * - Complete factor decomposition
 * - Marginal factor risk
 * - Edge cases (zero weights, single factor, zero variance)
 */

#include "risk/factor_risk.h"
#include "test_framework.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

/* Test portfolio factor exposures calculation */
TEST(test_portfolio_factor_exposures_basic) {
    const size_t n_assets  = 3;
    const size_t n_factors = 2;

    /* Factor exposures: 3 assets × 2 factors (row-major) */
    double factor_exposures[] = {
        0.8,
        0.3, /* Asset 0: high market, low size */
        0.6,
        0.7, /* Asset 1: medium market, high size */
        0.4,
        -0.2 /* Asset 2: low market, negative size */
    };

    /* Equal weights */
    double weights[] = {1.0 / 3.0, 1.0 / 3.0, 1.0 / 3.0};

    double portfolio_exposures[2];

    fc_status_t status = fc_ex_risk_portfolio_factor_exposures(
        portfolio_exposures, weights, factor_exposures, n_assets, n_factors
    );

    FC_TEST_ASSERT_EQ(status, FC_OK);

    /* Factor 0: (0.8 + 0.6 + 0.4) / 3 = 1.8 / 3 = 0.6 */
    FC_TEST_ASSERT_DOUBLE_EQ(portfolio_exposures[0], 0.6, 1e-10);

    /* Factor 1: (0.3 + 0.7 - 0.2) / 3 = 0.8 / 3 */
    FC_TEST_ASSERT_DOUBLE_EQ(portfolio_exposures[1], 0.8 / 3.0, 1e-10);
}

/* Test factor risk calculation */
TEST(test_factor_risk_basic) {
    const size_t n_factors = 2;

    /* Portfolio exposures */
    double portfolio_exposures[] = {0.6, 0.4};

    /* Factor covariance matrix (2×2, symmetric) */
    /* Var(F1) = 0.04, Var(F2) = 0.09, Cov(F1,F2) = 0.02 */
    double factor_covariance[] = {0.04, 0.02, 0.02, 0.09};

    double work_buffer[2];
    double factor_risk;

    fc_status_t status = fc_ex_risk_factor_risk(
        &factor_risk, portfolio_exposures, factor_covariance, n_factors, work_buffer
    );

    FC_TEST_ASSERT_EQ(status, FC_OK);

    /* Expected: X^T × F × X
     * F × X = [0.04*0.6 + 0.02*0.4, 0.02*0.6 + 0.09*0.4]
     *       = [0.024 + 0.008, 0.012 + 0.036]
     *       = [0.032, 0.048]
     * X^T × (F × X) = 0.6*0.032 + 0.4*0.048
     *               = 0.0192 + 0.0192 = 0.0384
     */
    double expected = 0.6 * 0.032 + 0.4 * 0.048;
    FC_TEST_ASSERT_DOUBLE_EQ(factor_risk, expected, 1e-10);
}

/* Test specific risk calculation */
TEST(test_specific_risk_basic) {
    const size_t n_assets = 3;

    double weights[]           = {0.5, 0.3, 0.2};
    double specific_variance[] = {0.01, 0.04, 0.09};

    double specific_risk;

    fc_status_t status =
        fc_ex_risk_specific_risk(&specific_risk, weights, specific_variance, n_assets);

    FC_TEST_ASSERT_EQ(status, FC_OK);

    /* Expected: Σ(w_i² × σ²_ε_i)
     * = 0.5² × 0.01 + 0.3² × 0.04 + 0.2² × 0.09
     * = 0.25 × 0.01 + 0.09 × 0.04 + 0.04 × 0.09
     * = 0.0025 + 0.0036 + 0.0036 = 0.0097
     */
    double expected = 0.25 * 0.01 + 0.09 * 0.04 + 0.04 * 0.09;
    FC_TEST_ASSERT_DOUBLE_EQ(specific_risk, expected, 1e-10);
}

/* Test complete factor decomposition */
TEST(test_factor_decomposition_complete) {
    const size_t n_assets  = 3;
    const size_t n_factors = 2;

    /* Factor exposures */
    double factor_exposures[] = {
        1.0,
        0.5, /* Asset 0 */
        0.8,
        0.3, /* Asset 1 */
        0.6,
        0.2 /* Asset 2 */
    };

    /* Factor covariance */
    double factor_covariance[] = {0.04, 0.01, 0.01, 0.09};

    /* Specific variance */
    double specific_variance[] = {0.01, 0.02, 0.03};

    /* Weights */
    double weights[] = {0.4, 0.4, 0.2};

    double factor_risk, specific_risk, total_risk;
    double portfolio_exposures[2];
    double work_buffer[2];

    fc_status_t status = fc_ex_risk_factor_decomposition(
        &factor_risk,
        &specific_risk,
        &total_risk,
        portfolio_exposures,
        weights,
        factor_exposures,
        factor_covariance,
        specific_variance,
        n_assets,
        n_factors,
        work_buffer
    );

    FC_TEST_ASSERT_EQ(status, FC_OK);

    /* Verify decomposition */
    FC_TEST_ASSERT(factor_risk >= 0.0);
    FC_TEST_ASSERT(specific_risk >= 0.0);
    FC_TEST_ASSERT_DOUBLE_EQ(total_risk, factor_risk + specific_risk, 1e-10);

    /* Verify portfolio exposures were computed */
    /* Factor 0: 0.4*1.0 + 0.4*0.8 + 0.2*0.6 = 0.4 + 0.32 + 0.12 = 0.84 */
    FC_TEST_ASSERT_DOUBLE_EQ(portfolio_exposures[0], 0.84, 1e-10);

    /* Factor 1: 0.4*0.5 + 0.4*0.3 + 0.2*0.2 = 0.2 + 0.12 + 0.04 = 0.36 */
    FC_TEST_ASSERT_DOUBLE_EQ(portfolio_exposures[1], 0.36, 1e-10);

    /* Verify decomposition property */
    int valid =
        fc_ex_risk_verify_factor_decomposition(factor_risk, specific_risk, total_risk, 1e-6);
    FC_TEST_ASSERT(valid == 1);
}

/* Test marginal factor risk */
TEST(test_marginal_factor_risk_basic) {
    const size_t n_assets  = 2;
    const size_t n_factors = 2;

    double weights[] = {0.6, 0.4};

    double factor_exposures[] = {
        1.0,
        0.5, /* Asset 0 */
        0.8,
        0.3 /* Asset 1 */
    };

    double factor_covariance[] = {0.04, 0.01, 0.01, 0.09};

    /* Portfolio exposures: [0.6*1.0 + 0.4*0.8, 0.6*0.5 + 0.4*0.3] = [0.92, 0.42] */
    double portfolio_exposures[] = {0.92, 0.42};

    double marginal_factor_risk[2];
    double work_buffer[2];

    fc_status_t status = fc_ex_risk_marginal_factor_risk(
        marginal_factor_risk,
        weights,
        factor_exposures,
        factor_covariance,
        portfolio_exposures,
        n_assets,
        n_factors,
        work_buffer
    );

    FC_TEST_ASSERT_EQ(status, FC_OK);

    /* Both assets should have positive marginal risk */
    FC_TEST_ASSERT(marginal_factor_risk[0] >= 0.0);
    FC_TEST_ASSERT(marginal_factor_risk[1] >= 0.0);

    /* Asset 0 has higher factor exposure, should have higher marginal risk */
    FC_TEST_ASSERT(marginal_factor_risk[0] > marginal_factor_risk[1]);
}

/* Test with single factor */
TEST(test_factor_decomposition_single_factor) {
    const size_t n_assets  = 3;
    const size_t n_factors = 1;

    /* Market factor only */
    double factor_exposures[] = {1.0, 0.8, 0.6}; /* Market betas */

    /* Market variance */
    double factor_covariance[] = {0.04};

    /* Specific variances */
    double specific_variance[] = {0.01, 0.02, 0.03};

    /* Equal weights */
    double weights[] = {1.0 / 3.0, 1.0 / 3.0, 1.0 / 3.0};

    double factor_risk, specific_risk, total_risk;
    double work_buffer[1];

    fc_status_t status = fc_ex_risk_factor_decomposition(
        &factor_risk,
        &specific_risk,
        &total_risk,
        NULL, /* Don't need portfolio exposures */
        weights,
        factor_exposures,
        factor_covariance,
        specific_variance,
        n_assets,
        n_factors,
        work_buffer
    );

    FC_TEST_ASSERT_EQ(status, FC_OK);

    /* Portfolio beta: (1.0 + 0.8 + 0.6) / 3 = 0.8 */
    /* Factor risk: 0.8² × 0.04 = 0.64 × 0.04 = 0.0256 */
    double expected_factor_risk = 0.64 * 0.04;
    FC_TEST_ASSERT_DOUBLE_EQ(factor_risk, expected_factor_risk, 1e-10);

    /* Specific risk: (1/3)² × (0.01 + 0.02 + 0.03) = (1/9) × 0.06 */
    double expected_specific_risk = (1.0 / 9.0) * (0.01 + 0.02 + 0.03);
    FC_TEST_ASSERT_DOUBLE_EQ(specific_risk, expected_specific_risk, 1e-10);

    FC_TEST_ASSERT_DOUBLE_EQ(total_risk, factor_risk + specific_risk, 1e-10);
}

/* Test with zero weights */
TEST(test_factor_decomposition_zero_weights) {
    const size_t n_assets  = 3;
    const size_t n_factors = 2;

    double factor_exposures[] = {1.0, 0.5, 0.8, 0.3, 0.6, 0.2};

    double factor_covariance[] = {0.04, 0.01, 0.01, 0.09};

    double specific_variance[] = {0.01, 0.02, 0.03};

    /* All zero weights */
    double weights[] = {0.0, 0.0, 0.0};

    double factor_risk, specific_risk, total_risk;
    double work_buffer[2];

    fc_status_t status = fc_ex_risk_factor_decomposition(
        &factor_risk,
        &specific_risk,
        &total_risk,
        NULL,
        weights,
        factor_exposures,
        factor_covariance,
        specific_variance,
        n_assets,
        n_factors,
        work_buffer
    );

    FC_TEST_ASSERT_EQ(status, FC_OK);

    /* Zero weights should result in zero risk */
    FC_TEST_ASSERT_DOUBLE_EQ(factor_risk, 0.0, 1e-10);
    FC_TEST_ASSERT_DOUBLE_EQ(specific_risk, 0.0, 1e-10);
    FC_TEST_ASSERT_DOUBLE_EQ(total_risk, 0.0, 1e-10);
}

/* Test with zero factor covariance */
TEST(test_factor_decomposition_zero_factor_covariance) {
    const size_t n_assets  = 2;
    const size_t n_factors = 2;

    double weights[] = {0.5, 0.5};

    double factor_exposures[] = {1.0, 0.5, 0.8, 0.3};

    /* Zero covariance matrix (uncorrelated factors with zero variance) */
    double factor_covariance[] = {0.0, 0.0, 0.0, 0.0};

    double specific_variance[] = {0.01, 0.02};

    double factor_risk, specific_risk, total_risk;
    double work_buffer[2];

    fc_status_t status = fc_ex_risk_factor_decomposition(
        &factor_risk,
        &specific_risk,
        &total_risk,
        NULL,
        weights,
        factor_exposures,
        factor_covariance,
        specific_variance,
        n_assets,
        n_factors,
        work_buffer
    );

    FC_TEST_ASSERT_EQ(status, FC_OK);

    /* Factor risk should be zero */
    FC_TEST_ASSERT_DOUBLE_EQ(factor_risk, 0.0, 1e-10);

    /* Only specific risk remains */
    FC_TEST_ASSERT(specific_risk > 0.0);
    FC_TEST_ASSERT_DOUBLE_EQ(total_risk, specific_risk, 1e-10);
}

/* Test input validation */
TEST(test_factor_decomposition_null_inputs) {
    const size_t n_assets  = 2;
    const size_t n_factors = 2;

    double factor_risk, specific_risk, total_risk;
    double weights[2]           = {0.5, 0.5};
    double factor_exposures[4]  = {1.0, 0.5, 0.8, 0.3};
    double factor_covariance[4] = {0.04, 0.01, 0.01, 0.09};
    double specific_variance[2] = {0.01, 0.02};
    double work_buffer[2];

    /* NULL factor_risk */
    fc_status_t status = fc_ex_risk_factor_decomposition(
        NULL,
        &specific_risk,
        &total_risk,
        NULL,
        weights,
        factor_exposures,
        factor_covariance,
        specific_variance,
        n_assets,
        n_factors,
        work_buffer
    );
    FC_TEST_ASSERT_EQ(status, FC_ERR_INVALID_ARG);

    /* NULL weights */
    status = fc_ex_risk_factor_decomposition(
        &factor_risk,
        &specific_risk,
        &total_risk,
        NULL,
        NULL,
        factor_exposures,
        factor_covariance,
        specific_variance,
        n_assets,
        n_factors,
        work_buffer
    );
    FC_TEST_ASSERT_EQ(status, FC_ERR_INVALID_ARG);

    /* Zero assets */
    status = fc_ex_risk_factor_decomposition(
        &factor_risk,
        &specific_risk,
        &total_risk,
        NULL,
        weights,
        factor_exposures,
        factor_covariance,
        specific_variance,
        0,
        n_factors,
        work_buffer
    );
    FC_TEST_ASSERT_EQ(status, FC_ERR_INVALID_ARG);

    /* Zero factors */
    status = fc_ex_risk_factor_decomposition(
        &factor_risk,
        &specific_risk,
        &total_risk,
        NULL,
        weights,
        factor_exposures,
        factor_covariance,
        specific_variance,
        n_assets,
        0,
        work_buffer
    );
    FC_TEST_ASSERT_EQ(status, FC_ERR_INVALID_ARG);
}

/* Test large portfolio */
TEST(test_factor_decomposition_large_portfolio) {
    const size_t n_assets  = 100;
    const size_t n_factors = 10;

    /* Allocate arrays */
    double* weights           = (double*) malloc(n_assets * sizeof(double));
    double* factor_exposures  = (double*) malloc(n_assets * n_factors * sizeof(double));
    double* factor_covariance = (double*) malloc(n_factors * n_factors * sizeof(double));
    double* specific_variance = (double*) malloc(n_assets * sizeof(double));
    double* work_buffer       = (double*) malloc(n_factors * sizeof(double));

    if (!weights || !factor_exposures || !factor_covariance || !specific_variance || !work_buffer) {
        free(weights);
        free(factor_exposures);
        free(factor_covariance);
        free(specific_variance);
        free(work_buffer);
        FC_TEST_ASSERT(0);
    }

    /* Initialize with realistic values */
    for (size_t i = 0; i < n_assets; i++) {
        weights[i]           = 1.0 / (double) n_assets;
        specific_variance[i] = 0.01 + 0.05 * ((double) i / (double) n_assets);

        for (size_t k = 0; k < n_factors; k++) {
            factor_exposures[i * n_factors + k] = 0.5 + 0.5 * sin((double) (i + k));
        }
    }

    /* Initialize factor covariance (diagonal dominant) */
    for (size_t k = 0; k < n_factors; k++) {
        for (size_t l = 0; l < n_factors; l++) {
            if (k == l) {
                factor_covariance[k * n_factors + l] = 0.04;
            } else {
                factor_covariance[k * n_factors + l] = 0.005;
            }
        }
    }

    double factor_risk, specific_risk, total_risk;

    fc_status_t status = fc_ex_risk_factor_decomposition(
        &factor_risk,
        &specific_risk,
        &total_risk,
        NULL,
        weights,
        factor_exposures,
        factor_covariance,
        specific_variance,
        n_assets,
        n_factors,
        work_buffer
    );

    FC_TEST_ASSERT_EQ(status, FC_OK);
    FC_TEST_ASSERT(factor_risk >= 0.0);
    FC_TEST_ASSERT(specific_risk >= 0.0);
    FC_TEST_ASSERT_DOUBLE_EQ(total_risk, factor_risk + specific_risk, 1e-8);

    /* Verify decomposition */
    int valid =
        fc_ex_risk_verify_factor_decomposition(factor_risk, specific_risk, total_risk, 1e-6);
    FC_TEST_ASSERT(valid == 1);

    free(weights);
    free(factor_exposures);
    free(factor_covariance);
    free(specific_variance);
    free(work_buffer);
}

/* Test verification function */
TEST(test_verify_factor_decomposition) {
    double factor_risk   = 0.05;
    double specific_risk = 0.03;
    double total_risk    = 0.08;

    /* Valid decomposition */
    int valid =
        fc_ex_risk_verify_factor_decomposition(factor_risk, specific_risk, total_risk, 1e-6);
    FC_TEST_ASSERT(valid == 1);

    /* Invalid decomposition */
    total_risk = 0.10;
    valid = fc_ex_risk_verify_factor_decomposition(factor_risk, specific_risk, total_risk, 1e-6);
    FC_TEST_ASSERT(valid == 0);

    /* Valid with larger tolerance */
    valid = fc_ex_risk_verify_factor_decomposition(factor_risk, specific_risk, total_risk, 0.25);
    FC_TEST_ASSERT(valid == 1);
}

/* Register all factor risk tests */
void register_factor_risk_tests(void) {
    RUN_TEST(test_portfolio_factor_exposures_basic);
    RUN_TEST(test_factor_risk_basic);
    RUN_TEST(test_specific_risk_basic);
    RUN_TEST(test_factor_decomposition_complete);
    RUN_TEST(test_marginal_factor_risk_basic);
    RUN_TEST(test_factor_decomposition_single_factor);
    RUN_TEST(test_factor_decomposition_zero_weights);
    RUN_TEST(test_factor_decomposition_zero_factor_covariance);
    RUN_TEST(test_factor_decomposition_null_inputs);
    RUN_TEST(test_factor_decomposition_large_portfolio);
    RUN_TEST(test_verify_factor_decomposition);
}
