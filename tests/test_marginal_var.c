/**
 * @file test_marginal_var.c
 * @brief Unit tests for marginal VaR calculation
 *
 * Tests include:
 * - Portfolio returns calculation
 * - Marginal VaR using correlation method
 * - Component VaR calculation
 * - VaR decomposition verification
 * - Marginal VaR using perturbation method
 * - Edge cases (zero variance, single asset, negative VaR)
 */

#include "risk/marginal_risk.h"
#include "test_framework.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

/* Test portfolio returns calculation */
TEST(test_portfolio_returns_basic) {
    const size_t n_assets = 3;
    const size_t n_days   = 5;

    /* Returns: 3 assets × 5 days (row-major) */
    double returns[] = {
        0.01,
        0.02,
        -0.01,
        0.03,
        0.00, /* Asset 0 */
        0.02,
        -0.01,
        0.01,
        0.02,
        0.01, /* Asset 1 */
        -0.01,
        0.01,
        0.02,
        -0.02,
        0.01 /* Asset 2 */
    };

    /* Equal weights */
    double weights[] = {1.0 / 3.0, 1.0 / 3.0, 1.0 / 3.0};

    double portfolio_returns[5];

    fc_status_t status =
        fc_ex_risk_portfolio_returns(portfolio_returns, returns, weights, n_assets, n_days);

    FC_TEST_ASSERT_EQ(status, FC_OK);

    /* Day 0: (0.01 + 0.02 - 0.01) / 3 = 0.02 / 3 */
    FC_TEST_ASSERT_DOUBLE_EQ(portfolio_returns[0], 0.02 / 3.0, 1e-10);
    /* Day 1: (0.02 - 0.01 + 0.01) / 3 = 0.02 / 3 */
    FC_TEST_ASSERT_DOUBLE_EQ(portfolio_returns[1], 0.02 / 3.0, 1e-10);
    /* Day 2: (-0.01 + 0.01 + 0.02) / 3 = 0.02 / 3 */
    FC_TEST_ASSERT_DOUBLE_EQ(portfolio_returns[2], 0.02 / 3.0, 1e-10);
    /* Day 3: (0.03 + 0.02 - 0.02) / 3 = 0.03 / 3 = 0.01 */
    FC_TEST_ASSERT_DOUBLE_EQ(portfolio_returns[3], 0.01, 1e-10);
    /* Day 4: (0.00 + 0.01 + 0.01) / 3 = 0.02 / 3 */
    FC_TEST_ASSERT_DOUBLE_EQ(portfolio_returns[4], 0.02 / 3.0, 1e-10);
}

/* Test marginal VaR correlation method */
TEST(test_marginal_var_correlation_basic) {
    const size_t n_assets = 2;
    const size_t n_days   = 10;

    /* Highly correlated assets */
    double returns[20];
    for (size_t i = 0; i < n_days; i++) {
        returns[i]          = sin((double) i * 0.5) * 0.02; /* Asset 0 */
        returns[n_days + i] = sin((double) i * 0.5) * 0.03; /* Asset 1: scaled version */
    }

    double weights[]     = {0.6, 0.4};
    double portfolio_var = 100000.0; /* $100k VaR */

    double marginal_var[2];

    fc_status_t status = fc_ex_risk_marginal_var_correlation(
        marginal_var, returns, weights, portfolio_var, n_assets, n_days
    );

    FC_TEST_ASSERT_EQ(status, FC_OK);

    /* Both assets should have positive marginal VaR */
    FC_TEST_ASSERT(marginal_var[0] > 0.0);
    FC_TEST_ASSERT(marginal_var[1] > 0.0);

    /* Asset 1 has higher volatility, should have higher marginal VaR */
    FC_TEST_ASSERT(marginal_var[1] > marginal_var[0]);
}

/* Test component VaR calculation */
TEST(test_component_var_basic) {
    const size_t n_assets = 3;

    double marginal_var[] = {1000.0, 2000.0, 1500.0};
    double weights[]      = {0.5, 0.3, 0.2};
    double component_var[3];

    fc_status_t status = fc_ex_risk_component_var(component_var, marginal_var, weights, n_assets);

    FC_TEST_ASSERT_EQ(status, FC_OK);

    /* Component VaR = Marginal VaR × Weight */
    FC_TEST_ASSERT_DOUBLE_EQ(component_var[0], 1000.0 * 0.5, 1e-10);
    FC_TEST_ASSERT_DOUBLE_EQ(component_var[1], 2000.0 * 0.3, 1e-10);
    FC_TEST_ASSERT_DOUBLE_EQ(component_var[2], 1500.0 * 0.2, 1e-10);
}

/* Test VaR decomposition verification */
TEST(test_var_decomposition_verification) {
    const size_t n_assets = 4;

    double component_var[] = {500.0, 300.0, 150.0, 50.0};
    double portfolio_var   = 1000.0; /* Sum = 1000 */

    int result = fc_ex_risk_verify_var_decomposition(component_var, portfolio_var, n_assets, 1e-6);

    FC_TEST_ASSERT_EQ(result, 1);

    /* Test with mismatch */
    double portfolio_var_wrong = 900.0;
    result =
        fc_ex_risk_verify_var_decomposition(component_var, portfolio_var_wrong, n_assets, 1e-6);

    FC_TEST_ASSERT_EQ(result, 0);
}

/* Test marginal VaR with single asset */
TEST(test_marginal_var_single_asset) {
    const size_t n_assets = 1;
    const size_t n_days   = 10;

    double returns[]     = {0.01, -0.02, 0.03, -0.01, 0.02, 0.01, -0.01, 0.02, 0.00, 0.01};
    double weights[]     = {1.0};
    double portfolio_var = 50000.0;

    double marginal_var[1];

    fc_status_t status = fc_ex_risk_marginal_var_correlation(
        marginal_var, returns, weights, portfolio_var, n_assets, n_days
    );

    FC_TEST_ASSERT_EQ(status, FC_OK);

    /* For single asset, marginal VaR should equal portfolio VaR */
    FC_TEST_ASSERT_DOUBLE_EQ(marginal_var[0], portfolio_var, 1e-6);
}

/* Test marginal VaR with zero variance portfolio */
TEST(test_marginal_var_zero_variance) {
    const size_t n_assets = 2;
    const size_t n_days   = 5;

    /* Constant returns (zero variance) */
    double returns[] = {
        0.01,
        0.01,
        0.01,
        0.01,
        0.01, /* Asset 0 */
        0.02,
        0.02,
        0.02,
        0.02,
        0.02 /* Asset 1 */
    };

    double weights[]     = {0.5, 0.5};
    double portfolio_var = 0.0;

    double marginal_var[2];

    fc_status_t status = fc_ex_risk_marginal_var_correlation(
        marginal_var, returns, weights, portfolio_var, n_assets, n_days
    );

    FC_TEST_ASSERT_EQ(status, FC_OK);

    /* Zero variance should give zero marginal VaR */
    FC_TEST_ASSERT_DOUBLE_EQ(marginal_var[0], 0.0, 1e-10);
    FC_TEST_ASSERT_DOUBLE_EQ(marginal_var[1], 0.0, 1e-10);
}

/* Test marginal VaR with negatively correlated assets */
TEST(test_marginal_var_negative_correlation) {
    const size_t n_assets = 2;
    const size_t n_days   = 10;

    /* Negatively correlated assets with unequal weights to ensure non-zero portfolio variance */
    double returns[20];
    for (size_t i = 0; i < n_days; i++) {
        returns[i]          = sin((double) i * 0.5) * 0.02;
        returns[n_days + i] = -sin((double) i * 0.5) * 0.02; /* Opposite direction */
    }

    double weights[]     = {0.6, 0.4}; /* Unequal weights */
    double portfolio_var = 10000.0;

    double marginal_var[2];

    fc_status_t status = fc_ex_risk_marginal_var_correlation(
        marginal_var, returns, weights, portfolio_var, n_assets, n_days
    );

    FC_TEST_ASSERT_EQ(status, FC_OK);

    /* With negative correlation, marginal VaRs should have opposite signs */
    FC_TEST_ASSERT(marginal_var[0] * marginal_var[1] < 0.0);
}

/* Test perturbation method */
TEST(test_marginal_var_perturbation_basic) {
    const size_t n_assets = 3;
    const size_t n_days   = 20;

    /* Generate realistic returns */
    double returns[60];
    for (size_t i = 0; i < n_assets; i++) {
        for (size_t t = 0; t < n_days; t++) {
            returns[i * n_days + t] = sin((double) (i + t) * 0.3) * 0.02;
        }
    }

    double weights[]     = {0.5, 0.3, 0.2};
    double portfolio_var = 75000.0;
    double confidence    = 0.95;
    double epsilon       = 0.0001;

    double work_buffer[20];
    double marginal_var[3];

    fc_status_t status = fc_ex_risk_marginal_var_perturbation(
        marginal_var,
        returns,
        weights,
        portfolio_var,
        confidence,
        epsilon,
        work_buffer,
        n_assets,
        n_days
    );

    FC_TEST_ASSERT_EQ(status, FC_OK);

    /* Marginal VaRs should be finite */
    for (size_t i = 0; i < n_assets; i++) {
        FC_TEST_ASSERT(!isnan(marginal_var[i]));
        FC_TEST_ASSERT(!isinf(marginal_var[i]));
    }
}

/* Test input validation */
TEST(test_marginal_var_input_validation) {
    const size_t n_assets  = 2;
    const size_t n_days    = 10;
    double returns[20]     = {0};
    double weights[2]      = {0.5, 0.5};
    double marginal_var[2] = {0};
    double portfolio_var   = 10000.0;

    /* NULL pointer checks */
    fc_status_t status = fc_ex_risk_marginal_var_correlation(
        NULL, returns, weights, portfolio_var, n_assets, n_days
    );
    FC_TEST_ASSERT_EQ(status, FC_ERR_INVALID_ARG);

    status = fc_ex_risk_marginal_var_correlation(
        marginal_var, NULL, weights, portfolio_var, n_assets, n_days
    );
    FC_TEST_ASSERT_EQ(status, FC_ERR_INVALID_ARG);

    status = fc_ex_risk_marginal_var_correlation(
        marginal_var, returns, NULL, portfolio_var, n_assets, n_days
    );
    FC_TEST_ASSERT_EQ(status, FC_ERR_INVALID_ARG);

    /* Invalid dimensions */
    status = fc_ex_risk_marginal_var_correlation(
        marginal_var, returns, weights, portfolio_var, 0, n_days
    );
    FC_TEST_ASSERT_EQ(status, FC_ERR_INVALID_ARG);

    status = fc_ex_risk_marginal_var_correlation(
        marginal_var, returns, weights, portfolio_var, n_assets, 1
    );
    FC_TEST_ASSERT_EQ(status, FC_ERR_INVALID_ARG);

    /* Negative VaR */
    status = fc_ex_risk_marginal_var_correlation(
        marginal_var, returns, weights, -100.0, n_assets, n_days
    );
    FC_TEST_ASSERT_EQ(status, FC_ERR_INVALID_ARG);
}

/* Test component VaR input validation */
TEST(test_component_var_input_validation) {
    const size_t n_assets  = 2;
    double marginal_var[2] = {1000.0, 2000.0};
    double weights[2]      = {0.5, 0.5};
    double component_var[2];

    /* NULL pointer checks */
    fc_status_t status = fc_ex_risk_component_var(NULL, marginal_var, weights, n_assets);
    FC_TEST_ASSERT_EQ(status, FC_ERR_INVALID_ARG);

    status = fc_ex_risk_component_var(component_var, NULL, weights, n_assets);
    FC_TEST_ASSERT_EQ(status, FC_ERR_INVALID_ARG);

    status = fc_ex_risk_component_var(component_var, marginal_var, NULL, n_assets);
    FC_TEST_ASSERT_EQ(status, FC_ERR_INVALID_ARG);

    /* Zero assets */
    status = fc_ex_risk_component_var(component_var, marginal_var, weights, 0);
    FC_TEST_ASSERT_EQ(status, FC_ERR_INVALID_ARG);
}

/* Test large portfolio */
TEST(test_marginal_var_large_portfolio) {
    const size_t n_assets = 100;
    const size_t n_days   = 252; /* One trading year */

    size_t returns_size = n_assets * n_days * sizeof(double);
    size_t weights_size = n_assets * sizeof(double);

    /* Round up to multiple of 64 for aligned_alloc */
    returns_size = ((returns_size + 63) / 64) * 64;
    weights_size = ((weights_size + 63) / 64) * 64;

    double* returns      = (double*) aligned_alloc(64, returns_size);
    double* weights      = (double*) aligned_alloc(64, weights_size);
    double* marginal_var = (double*) aligned_alloc(64, weights_size);

    FC_TEST_ASSERT_NOT_NULL(returns);
    FC_TEST_ASSERT_NOT_NULL(weights);
    FC_TEST_ASSERT_NOT_NULL(marginal_var);

    /* Equal weights */
    for (size_t i = 0; i < n_assets; i++) {
        weights[i] = 1.0 / (double) n_assets;
    }

    /* Generate correlated returns */
    for (size_t i = 0; i < n_assets; i++) {
        for (size_t t = 0; t < n_days; t++) {
            double common_factor    = sin((double) t * 0.1) * 0.01;
            double idiosyncratic    = sin((double) (i * t) * 0.05) * 0.005;
            returns[i * n_days + t] = common_factor + idiosyncratic;
        }
    }

    double portfolio_var = 500000.0;

    fc_status_t status = fc_ex_risk_marginal_var_correlation(
        marginal_var, returns, weights, portfolio_var, n_assets, n_days
    );

    FC_TEST_ASSERT_EQ(status, FC_OK);

    /* Verify all marginal VaRs are finite */
    for (size_t i = 0; i < n_assets; i++) {
        FC_TEST_ASSERT(!isnan(marginal_var[i]));
        FC_TEST_ASSERT(!isinf(marginal_var[i]));
    }

    free(returns);
    free(weights);
    free(marginal_var);
}

/* Register all marginal VaR tests */
void register_marginal_var_tests(void) {
    RUN_TEST(test_portfolio_returns_basic);
    RUN_TEST(test_marginal_var_correlation_basic);
    RUN_TEST(test_component_var_basic);
    RUN_TEST(test_var_decomposition_verification);
    RUN_TEST(test_marginal_var_single_asset);
    RUN_TEST(test_marginal_var_zero_variance);
    RUN_TEST(test_marginal_var_negative_correlation);
    RUN_TEST(test_marginal_var_perturbation_basic);
    RUN_TEST(test_marginal_var_input_validation);
    RUN_TEST(test_component_var_input_validation);
    RUN_TEST(test_marginal_var_large_portfolio);
}
