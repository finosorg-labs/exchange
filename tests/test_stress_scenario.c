/**
 * @file test_stress_scenario.c
 * @brief Unit tests for stress scenario application
 *
 * Tests include:
 * - Single scenario application (multiplicative and additive)
 * - Batch scenario processing
 * - Historical scenario extraction
 * - Scenario VaR calculation
 * - Worst scenario identification
 * - Edge cases (zero positions, extreme shocks)
 */

#include "risk/stress_scenario.h"
#include "test_framework.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

/* Test single scenario application - multiplicative shock */
TEST(test_apply_scenario_multiplicative) {
    const size_t n_assets = 4;

    double position_values[] = {100000.0, 50000.0, 75000.0, 25000.0};
    double shocks[]          = {-0.10, -0.05, 0.03, -0.20};

    double scenario_pnl;

    fc_status_t status = fc_ex_risk_apply_scenario(
        &scenario_pnl, position_values, shocks, FC_SHOCK_MULTIPLICATIVE, n_assets
    );

    FC_TEST_ASSERT_EQ(status, FC_OK);

    /* Expected: 100000*(-0.10) + 50000*(-0.05) + 75000*0.03 + 25000*(-0.20)
     *         = -10000 - 2500 + 2250 - 5000 = -15250
     */
    double expected = -15250.0;
    FC_TEST_ASSERT_DOUBLE_EQ(scenario_pnl, expected, 1e-6);
}

/* Test single scenario application - additive shock */
TEST(test_apply_scenario_additive) {
    const size_t n_assets = 3;

    double position_values[] = {100000.0, 50000.0, 75000.0};
    double shocks[]          = {-1000.0, 500.0, -250.0};

    double scenario_pnl;

    fc_status_t status = fc_ex_risk_apply_scenario(
        &scenario_pnl, position_values, shocks, FC_SHOCK_ADDITIVE, n_assets
    );

    FC_TEST_ASSERT_EQ(status, FC_OK);

    /* Expected: -1000 + 500 - 250 = -750 */
    double expected = -750.0;
    FC_TEST_ASSERT_DOUBLE_EQ(scenario_pnl, expected, 1e-6);
}

/* Test batch scenario application - multiplicative */
TEST(test_apply_scenarios_batch_multiplicative) {
    const size_t n_assets    = 3;
    const size_t n_scenarios = 4;

    double position_values[] = {100000.0, 50000.0, 75000.0};

    /* Shock matrix (3 assets × 4 scenarios, row-major) */
    double shock_matrix[] = {
        -0.30,
        -0.10,
        0.05,
        -0.05, /* Asset 0 */
        -0.25,
        -0.08,
        0.03,
        -0.02, /* Asset 1 */
        -0.20,
        -0.05,
        0.02,
        -0.03 /* Asset 2 */
    };

    double scenario_pnls[4];

    fc_status_t status = fc_ex_risk_apply_scenarios_batch(
        scenario_pnls, position_values, shock_matrix, FC_SHOCK_MULTIPLICATIVE, n_assets, n_scenarios
    );

    FC_TEST_ASSERT_EQ(status, FC_OK);

    /* Scenario 0: 100000*(-0.30) + 50000*(-0.25) + 75000*(-0.20) = -57500 */
    FC_TEST_ASSERT_DOUBLE_EQ(scenario_pnls[0], -57500.0, 1e-6);

    /* Scenario 1: 100000*(-0.10) + 50000*(-0.08) + 75000*(-0.05) = -17750 */
    FC_TEST_ASSERT_DOUBLE_EQ(scenario_pnls[1], -17750.0, 1e-6);

    /* Scenario 2: 100000*0.05 + 50000*0.03 + 75000*0.02 = 8000 */
    FC_TEST_ASSERT_DOUBLE_EQ(scenario_pnls[2], 8000.0, 1e-6);

    /* Scenario 3: 100000*(-0.05) + 50000*(-0.02) + 75000*(-0.03) = -8250 */
    FC_TEST_ASSERT_DOUBLE_EQ(scenario_pnls[3], -8250.0, 1e-6);
}

/* Test batch scenario application - additive */
TEST(test_apply_scenarios_batch_additive) {
    const size_t n_assets    = 2;
    const size_t n_scenarios = 3;

    double position_values[] = {100000.0, 50000.0};

    double shock_matrix[] = {
        -5000.0,
        1000.0,
        -2000.0, /* Asset 0 */
        -3000.0,
        500.0,
        -1000.0 /* Asset 1 */
    };

    double scenario_pnls[3];

    fc_status_t status = fc_ex_risk_apply_scenarios_batch(
        scenario_pnls, position_values, shock_matrix, FC_SHOCK_ADDITIVE, n_assets, n_scenarios
    );

    FC_TEST_ASSERT_EQ(status, FC_OK);

    /* Scenario 0: -5000 - 3000 = -8000 */
    FC_TEST_ASSERT_DOUBLE_EQ(scenario_pnls[0], -8000.0, 1e-6);

    /* Scenario 1: 1000 + 500 = 1500 */
    FC_TEST_ASSERT_DOUBLE_EQ(scenario_pnls[1], 1500.0, 1e-6);

    /* Scenario 2: -2000 - 1000 = -3000 */
    FC_TEST_ASSERT_DOUBLE_EQ(scenario_pnls[2], -3000.0, 1e-6);
}

/* Test historical scenario extraction */
TEST(test_extract_historical_scenario) {
    const size_t n_assets = 3;
    const size_t n_days   = 10;

    /* Historical returns matrix (3 assets × 10 days, row-major) */
    double returns[] = {
        0.01,  -0.02, 0.03,  -0.01, 0.02,  -0.03, 0.01,  0.02,  -0.01, 0.03, /* Asset 0 */
        0.02,  -0.01, 0.01,  0.02,  -0.02, 0.01,  -0.01, 0.03,  -0.02, 0.01, /* Asset 1 */
        -0.01, 0.02,  -0.03, 0.01,  0.03,  -0.02, 0.02,  -0.01, 0.02,  -0.02 /* Asset 2 */
    };

    double shocks[3];

    /* Extract scenario from day 2 to day 5 (inclusive) */
    fc_status_t status =
        fc_ex_risk_extract_historical_scenario(shocks, returns, 2, 5, n_assets, n_days);

    FC_TEST_ASSERT_EQ(status, FC_OK);

    /* Asset 0: 0.03 + (-0.01) + 0.02 + (-0.03) = 0.01 */
    FC_TEST_ASSERT_DOUBLE_EQ(shocks[0], 0.01, 1e-10);

    /* Asset 1: 0.01 + 0.02 + (-0.02) + 0.01 = 0.02 */
    FC_TEST_ASSERT_DOUBLE_EQ(shocks[1], 0.02, 1e-10);

    /* Asset 2: (-0.03) + 0.01 + 0.03 + (-0.02) = -0.01 */
    FC_TEST_ASSERT_DOUBLE_EQ(shocks[2], -0.01, 1e-10);
}

/* Test scenario VaR calculation */
TEST(test_scenario_var) {
    const size_t n_scenarios = 10;
    const double confidence  = 0.95;

    /* Scenario P&L values (sorted: -50000, -30000, -10000, -5000, 0, 1000, 2000, 5000, 10000,
     * 15000) */
    double scenario_pnls[] = {
        -10000.0, 5000.0, -30000.0, 2000.0, 10000.0, -5000.0, 0.0, 15000.0, 1000.0, -50000.0
    };

    double work_buffer[10];
    double var_estimate;

    fc_status_t status =
        fc_ex_risk_scenario_var(&var_estimate, scenario_pnls, confidence, n_scenarios, work_buffer);

    FC_TEST_ASSERT_EQ(status, FC_OK);

    /* 95% VaR: at 5th percentile (index 0 after sorting), which is -50000 */
    /* VaR = -(-50000) = 50000 */
    FC_TEST_ASSERT_DOUBLE_EQ(var_estimate, 50000.0, 1e-6);
}

/* Test worst scenario identification */
TEST(test_worst_scenario) {
    const size_t n_scenarios = 6;

    double scenario_pnls[] = {-10000.0, 5000.0, -30000.0, 2000.0, 10000.0, -5000.0};

    double worst_pnl;
    size_t worst_index;

    fc_status_t status =
        fc_ex_risk_worst_scenario(&worst_pnl, &worst_index, scenario_pnls, n_scenarios);

    FC_TEST_ASSERT_EQ(status, FC_OK);

    FC_TEST_ASSERT_DOUBLE_EQ(worst_pnl, -30000.0, 1e-6);
    FC_TEST_ASSERT_EQ(worst_index, 2);
}

/* Test edge case: zero positions */
TEST(test_zero_positions) {
    const size_t n_assets = 3;

    double position_values[] = {0.0, 0.0, 0.0};
    double shocks[]          = {-0.30, -0.20, -0.10};

    double scenario_pnl;

    fc_status_t status = fc_ex_risk_apply_scenario(
        &scenario_pnl, position_values, shocks, FC_SHOCK_MULTIPLICATIVE, n_assets
    );

    FC_TEST_ASSERT_EQ(status, FC_OK);
    FC_TEST_ASSERT_DOUBLE_EQ(scenario_pnl, 0.0, 1e-10);
}

/* Test edge case: extreme shock */
TEST(test_extreme_shock) {
    const size_t n_assets = 2;

    double position_values[] = {1000000.0, 500000.0};
    double shocks[]          = {-0.99, -0.95};

    double scenario_pnl;

    fc_status_t status = fc_ex_risk_apply_scenario(
        &scenario_pnl, position_values, shocks, FC_SHOCK_MULTIPLICATIVE, n_assets
    );

    FC_TEST_ASSERT_EQ(status, FC_OK);

    /* Expected: 1000000*(-0.99) + 500000*(-0.95) = -990000 - 475000 = -1465000 */
    FC_TEST_ASSERT_DOUBLE_EQ(scenario_pnl, -1465000.0, 1e-6);
}

/* Test edge case: single asset */
TEST(test_single_asset) {
    const size_t n_assets = 1;

    double position_values[] = {100000.0};
    double shocks[]          = {-0.15};

    double scenario_pnl;

    fc_status_t status = fc_ex_risk_apply_scenario(
        &scenario_pnl, position_values, shocks, FC_SHOCK_MULTIPLICATIVE, n_assets
    );

    FC_TEST_ASSERT_EQ(status, FC_OK);
    FC_TEST_ASSERT_DOUBLE_EQ(scenario_pnl, -15000.0, 1e-6);
}

/* Test edge case: single scenario */
TEST(test_single_scenario) {
    const size_t n_assets    = 2;
    const size_t n_scenarios = 1;

    double position_values[] = {100000.0, 50000.0};
    double shock_matrix[]    = {-0.10, -0.15};

    double scenario_pnls[1];

    fc_status_t status = fc_ex_risk_apply_scenarios_batch(
        scenario_pnls, position_values, shock_matrix, FC_SHOCK_MULTIPLICATIVE, n_assets, n_scenarios
    );

    FC_TEST_ASSERT_EQ(status, FC_OK);

    /* Scenario 0: 100000*(-0.10) + 50000*(-0.15) = -17500 */
    FC_TEST_ASSERT_DOUBLE_EQ(scenario_pnls[0], -17500.0, 1e-6);
}

/* Test error: NULL pointer */
TEST(test_null_pointer) {
    double position_values[] = {100000.0};
    double shocks[]          = {-0.10};
    double scenario_pnl;

    fc_status_t status =
        fc_ex_risk_apply_scenario(NULL, position_values, shocks, FC_SHOCK_MULTIPLICATIVE, 1);
    FC_TEST_ASSERT_EQ(status, FC_ERR_INVALID_ARG);

    status = fc_ex_risk_apply_scenario(&scenario_pnl, NULL, shocks, FC_SHOCK_MULTIPLICATIVE, 1);
    FC_TEST_ASSERT_EQ(status, FC_ERR_INVALID_ARG);

    status =
        fc_ex_risk_apply_scenario(&scenario_pnl, position_values, NULL, FC_SHOCK_MULTIPLICATIVE, 1);
    FC_TEST_ASSERT_EQ(status, FC_ERR_INVALID_ARG);
}

/* Test error: zero assets */
TEST(test_zero_assets) {
    double position_values[] = {100000.0};
    double shocks[]          = {-0.10};
    double scenario_pnl;

    fc_status_t status = fc_ex_risk_apply_scenario(
        &scenario_pnl, position_values, shocks, FC_SHOCK_MULTIPLICATIVE, 0
    );
    FC_TEST_ASSERT_EQ(status, FC_ERR_INVALID_ARG);
}

/* Test error: invalid day range */
TEST(test_invalid_day_range) {
    double returns[] = {0.01, 0.02, 0.03, 0.04, 0.05};
    double shocks[1];

    fc_status_t status = fc_ex_risk_extract_historical_scenario(shocks, returns, 3, 2, 1, 5);
    FC_TEST_ASSERT_EQ(status, FC_ERR_INVALID_ARG);

    status = fc_ex_risk_extract_historical_scenario(shocks, returns, 0, 5, 1, 5);
    FC_TEST_ASSERT_EQ(status, FC_ERR_INVALID_ARG);
}

/* Test error: invalid confidence level */
TEST(test_invalid_confidence) {
    double scenario_pnls[] = {-10000.0, 5000.0};
    double work_buffer[2];
    double var_estimate;

    fc_status_t status = fc_ex_risk_scenario_var(&var_estimate, scenario_pnls, 0.0, 2, work_buffer);
    FC_TEST_ASSERT_EQ(status, FC_ERR_INVALID_ARG);

    status = fc_ex_risk_scenario_var(&var_estimate, scenario_pnls, 1.0, 2, work_buffer);
    FC_TEST_ASSERT_EQ(status, FC_ERR_INVALID_ARG);

    status = fc_ex_risk_scenario_var(&var_estimate, scenario_pnls, -0.5, 2, work_buffer);
    FC_TEST_ASSERT_EQ(status, FC_ERR_INVALID_ARG);
}

/* Test large portfolio */
TEST(test_large_portfolio) {
    const size_t n_assets    = 1000;
    const size_t n_scenarios = 10;

    double* position_values = FC_TEST_MALLOC(double, n_assets);
    double* shock_matrix    = FC_TEST_MALLOC(double, n_assets* n_scenarios);
    double* scenario_pnls   = FC_TEST_MALLOC(double, n_scenarios);

    if (!position_values || !shock_matrix || !scenario_pnls) {
        FC_TEST_FAIL("Memory allocation failed");
        return;
    }

    for (size_t i = 0; i < n_assets; i++) {
        position_values[i] = 10000.0 + (double) i * 100.0;
    }

    for (size_t i = 0; i < n_assets; i++) {
        for (size_t s = 0; s < n_scenarios; s++) {
            shock_matrix[i * n_scenarios + s] =
                -0.01 * (double) (s + 1) + 0.001 * (double) (i % 10);
        }
    }

    fc_status_t status = fc_ex_risk_apply_scenarios_batch(
        scenario_pnls, position_values, shock_matrix, FC_SHOCK_MULTIPLICATIVE, n_assets, n_scenarios
    );

    FC_TEST_ASSERT_EQ(status, FC_OK);

    for (size_t s = 0; s < n_scenarios; s++) {
        FC_TEST_ASSERT(scenario_pnls[s] < 0.0);
    }

    /* Memory will be automatically freed by test framework */
}

/* Main test runner */
/* Register all stress scenario tests */
void register_stress_scenario_tests(void) {
    RUN_TEST(test_apply_scenario_multiplicative);
    RUN_TEST(test_apply_scenario_additive);
    RUN_TEST(test_apply_scenarios_batch_multiplicative);
    RUN_TEST(test_apply_scenarios_batch_additive);
    RUN_TEST(test_extract_historical_scenario);
    RUN_TEST(test_scenario_var);
    RUN_TEST(test_worst_scenario);
    RUN_TEST(test_zero_positions);
    RUN_TEST(test_extreme_shock);
    RUN_TEST(test_single_asset);
    RUN_TEST(test_single_scenario);
    RUN_TEST(test_null_pointer);
    RUN_TEST(test_zero_assets);
    RUN_TEST(test_invalid_day_range);
    RUN_TEST(test_invalid_confidence);
    RUN_TEST(test_large_portfolio);
}
