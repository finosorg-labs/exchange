/**
 * @file test_normalize.c
 * @brief Unit tests for online normalization using Welford incremental Z-Score
 *
 * Tests include:
 * - Basic z-score normalization
 * - Multiple symbols and features
 * - Edge cases (zero stddev, insufficient data, NULL pointers)
 * - Numerical accuracy
 * - State update correctness
 */

#include "signal/normalize.h"
#include "test_framework.h"
#include "mem_aligned.h"
#include <math.h>
#include <string.h>
#include <float.h>

/* Test basic z-score normalization with single feature */
TEST(test_normalize_basic_single_feature) {
    const size_t n_symbols  = 3;
    const int n_features    = 1;

    /* Setup Welford state: mean=10.0, stddev=2.0 (based on 100 samples) */
    fc_welford_state_t states[1];
    states[0].count = 100;
    states[0].mean  = 10.0;
    states[0].m2    = 99.0 * 4.0; /* variance = 4.0, stddev = 2.0 */

    /* Features: 8.0, 10.0, 12.0 */
    double features[] = {8.0, 10.0, 12.0};
    double z_out[3];

    fc_status_t status = fc_ex_sig_normalize_zscore(
        z_out,
        states,
        features,
        n_symbols,
        n_features
    );

    FC_TEST_ASSERT_EQ(status, FC_OK);

    /* z-scores: (8-10)/2 = -1, (10-10)/2 = 0, (12-10)/2 = 1 */
    FC_TEST_ASSERT_DOUBLE_EQ(z_out[0], -1.0, 1e-10);
    FC_TEST_ASSERT_DOUBLE_EQ(z_out[1], 0.0, 1e-10);
    FC_TEST_ASSERT_DOUBLE_EQ(z_out[2], 1.0, 1e-10);
}

/* Test multi-dimensional feature normalization */
TEST(test_normalize_multi_feature) {
    const size_t n_symbols  = 2;
    const int n_features    = 3;

    /* Setup Welford states for 3 features */
    fc_welford_state_t states[3];

    /* Feature 0: mean=100, stddev=10 */
    states[0].count = 50;
    states[0].mean  = 100.0;
    states[0].m2    = 49.0 * 100.0; /* variance = 100, stddev = 10 */

    /* Feature 1: mean=0, stddev=1 */
    states[1].count = 50;
    states[1].mean  = 0.0;
    states[1].m2    = 49.0 * 1.0; /* variance = 1, stddev = 1 */

    /* Feature 2: mean=50, stddev=5 */
    states[2].count = 50;
    states[2].mean  = 50.0;
    states[2].m2    = 49.0 * 25.0; /* variance = 25, stddev = 5 */

    /* Feature matrix (row-major):
     * Symbol 0: [110, 1, 55]
     * Symbol 1: [90, -2, 45]
     */
    double features[] = {
        110.0, 1.0, 55.0,  /* Symbol 0 */
        90.0, -2.0, 45.0   /* Symbol 1 */
    };
    double z_out[6];

    fc_status_t status = fc_ex_sig_normalize_zscore(
        z_out,
        states,
        features,
        n_symbols,
        n_features
    );

    FC_TEST_ASSERT_EQ(status, FC_OK);

    /* Expected z-scores:
     * Symbol 0: [(110-100)/10, (1-0)/1, (55-50)/5] = [1, 1, 1]
     * Symbol 1: [(90-100)/10, (-2-0)/1, (45-50)/5] = [-1, -2, -1]
     */
    FC_TEST_ASSERT_DOUBLE_EQ(z_out[0], 1.0, 1e-10);  /* Symbol 0, Feature 0 */
    FC_TEST_ASSERT_DOUBLE_EQ(z_out[1], 1.0, 1e-10);  /* Symbol 0, Feature 1 */
    FC_TEST_ASSERT_DOUBLE_EQ(z_out[2], 1.0, 1e-10);  /* Symbol 0, Feature 2 */
    FC_TEST_ASSERT_DOUBLE_EQ(z_out[3], -1.0, 1e-10); /* Symbol 1, Feature 0 */
    FC_TEST_ASSERT_DOUBLE_EQ(z_out[4], -2.0, 1e-10); /* Symbol 1, Feature 1 */
    FC_TEST_ASSERT_DOUBLE_EQ(z_out[5], -1.0, 1e-10); /* Symbol 1, Feature 2 */
}

/* Test zero standard deviation (all values identical) */
TEST(test_normalize_zero_stddev) {
    const size_t n_symbols  = 3;
    const int n_features    = 1;

    /* Setup Welford state with zero variance (all values = 42.0) */
    fc_welford_state_t states[1];
    states[0].count = 100;
    states[0].mean  = 42.0;
    states[0].m2    = 0.0; /* zero variance */

    double features[] = {40.0, 42.0, 44.0};
    double z_out[3];

    fc_status_t status = fc_ex_sig_normalize_zscore(
        z_out,
        states,
        features,
        n_symbols,
        n_features
    );

    FC_TEST_ASSERT_EQ(status, FC_OK);

    /* With zero stddev, all z-scores should be 0.0 (avoid division by zero) */
    FC_TEST_ASSERT_DOUBLE_EQ(z_out[0], 0.0, 1e-10);
    FC_TEST_ASSERT_DOUBLE_EQ(z_out[1], 0.0, 1e-10);
    FC_TEST_ASSERT_DOUBLE_EQ(z_out[2], 0.0, 1e-10);
}

/* Test insufficient data (count < 2) */
TEST(test_normalize_insufficient_data) {
    const size_t n_symbols  = 2;
    const int n_features    = 1;

    /* Setup Welford state with only 1 sample (insufficient for stddev) */
    fc_welford_state_t states[1];
    states[0].count = 1;
    states[0].mean  = 10.0;
    states[0].m2    = 0.0;

    double features[] = {8.0, 12.0};
    double z_out[2];

    fc_status_t status = fc_ex_sig_normalize_zscore(
        z_out,
        states,
        features,
        n_symbols,
        n_features
    );

    FC_TEST_ASSERT_EQ(status, FC_OK);

    /* With count < 2, z-scores should be 0.0 */
    FC_TEST_ASSERT_DOUBLE_EQ(z_out[0], 0.0, 1e-10);
    FC_TEST_ASSERT_DOUBLE_EQ(z_out[1], 0.0, 1e-10);
}

/* Test NULL pointer handling */
TEST(test_normalize_null_pointers) {
    double z_out[1];
    fc_welford_state_t states[1];
    double features[1] = {1.0};

    /* Initialize state to avoid uninitialized warning */
    fc_welford_init(&states[0]);

    /* NULL z_out */
    fc_status_t status = fc_ex_sig_normalize_zscore(
        NULL,
        states,
        features,
        1,
        1
    );
    FC_TEST_ASSERT_EQ(status, FC_ERR_INVALID_ARG);

    /* NULL states */
    status = fc_ex_sig_normalize_zscore(
        z_out,
        NULL,
        features,
        1,
        1
    );
    FC_TEST_ASSERT_EQ(status, FC_ERR_INVALID_ARG);

    /* NULL features */
    status = fc_ex_sig_normalize_zscore(
        z_out,
        states,
        NULL,
        1,
        1
    );
    FC_TEST_ASSERT_EQ(status, FC_ERR_INVALID_ARG);
}

/* Test invalid dimensions */
TEST(test_normalize_invalid_dimensions) {
    double z_out[1];
    fc_welford_state_t states[1];
    double features[1] = {1.0};

    /* Initialize state to avoid uninitialized warning */
    fc_welford_init(&states[0]);

    /* Zero symbols */
    fc_status_t status = fc_ex_sig_normalize_zscore(
        z_out,
        states,
        features,
        0,
        1
    );
    FC_TEST_ASSERT_EQ(status, FC_ERR_INVALID_ARG);

    /* Zero features */
    status = fc_ex_sig_normalize_zscore(
        z_out,
        states,
        features,
        1,
        0
    );
    FC_TEST_ASSERT_EQ(status, FC_ERR_INVALID_ARG);

    /* Negative features */
    status = fc_ex_sig_normalize_zscore(
        z_out,
        states,
        features,
        1,
        -1
    );
    FC_TEST_ASSERT_EQ(status, FC_ERR_INVALID_ARG);
}

/* Test NaN input handling */
TEST(test_normalize_nan_input) {
    const size_t n_symbols  = 3;
    const int n_features    = 1;

    fc_welford_state_t states[1];
    states[0].count = 100;
    states[0].mean  = 10.0;
    states[0].m2    = 99.0 * 4.0;

    /* Features with NaN */
    double features[] = {8.0, NAN, 12.0};
    double z_out[3];

    fc_status_t status = fc_ex_sig_normalize_zscore(
        z_out,
        states,
        features,
        n_symbols,
        n_features
    );

    FC_TEST_ASSERT_EQ(status, FC_ERR_NAN_INPUT);
}

/* Test state update correctness */
TEST(test_normalize_update_states_basic) {
    const size_t n_symbols  = 3;
    const int n_features    = 2;

    /* Initialize empty states */
    fc_welford_state_t states[2];
    memset(states, 0, sizeof(states));

    /* Feature matrix:
     * Symbol 0: [10, 20]
     * Symbol 1: [12, 24]
     * Symbol 2: [14, 28]
     */
    double features[] = {
        10.0, 20.0,  /* Symbol 0 */
        12.0, 24.0,  /* Symbol 1 */
        14.0, 28.0   /* Symbol 2 */
    };

    fc_status_t status = fc_ex_sig_normalize_update_states(
        states,
        features,
        n_symbols,
        n_features
    );

    FC_TEST_ASSERT_EQ(status, FC_OK);

    /* Verify Feature 0 state: mean of [10, 12, 14] = 12 */
    FC_TEST_ASSERT_EQ(states[0].count, 3);
    FC_TEST_ASSERT_DOUBLE_EQ(states[0].mean, 12.0, 1e-10);

    /* Verify Feature 1 state: mean of [20, 24, 28] = 24 */
    FC_TEST_ASSERT_EQ(states[1].count, 3);
    FC_TEST_ASSERT_DOUBLE_EQ(states[1].mean, 24.0, 1e-10);

    /* Verify variance: Feature 0 has variance = 4.0, m2 = 2 * 4.0 = 8.0 */
    double expected_m2_f0 = 2.0 * 2.0 + 0.0 * 0.0 + 2.0 * 2.0; /* sum of squared deviations */
    FC_TEST_ASSERT_DOUBLE_EQ(states[0].m2, expected_m2_f0, 1e-10);
}

/* Test state update with NULL pointers */
TEST(test_normalize_update_states_null) {
    fc_welford_state_t states[1];
    double features[1] = {1.0};

    /* NULL states */
    fc_status_t status = fc_ex_sig_normalize_update_states(
        NULL,
        features,
        1,
        1
    );
    FC_TEST_ASSERT_EQ(status, FC_ERR_INVALID_ARG);

    /* NULL features */
    status = fc_ex_sig_normalize_update_states(
        states,
        NULL,
        1,
        1
    );
    FC_TEST_ASSERT_EQ(status, FC_ERR_INVALID_ARG);
}

/* Test state update with NaN */
TEST(test_normalize_update_states_nan) {
    const size_t n_symbols  = 2;
    const int n_features    = 1;

    fc_welford_state_t states[1];
    memset(states, 0, sizeof(states));

    double features[] = {10.0, NAN};

    fc_status_t status = fc_ex_sig_normalize_update_states(
        states,
        features,
        n_symbols,
        n_features
    );

    FC_TEST_ASSERT_EQ(status, FC_ERR_NAN_INPUT);
}

/* Test large batch normalization (performance-oriented) */
TEST(test_normalize_large_batch) {
    const size_t n_symbols  = 1000;
    const int n_features    = 50;

    /* Allocate aligned memory */
    fc_welford_state_t* states = fc_aligned_alloc(n_features * sizeof(fc_welford_state_t), 64);
    double* features = fc_aligned_alloc(n_symbols * n_features * sizeof(double), 64);
    double* z_out = fc_aligned_alloc(n_symbols * n_features * sizeof(double), 64);

    FC_TEST_ASSERT(states != NULL);
    FC_TEST_ASSERT(features != NULL);
    FC_TEST_ASSERT(z_out != NULL);

    /* Initialize states with reasonable values */
    for (int f = 0; f < n_features; f++) {
        states[f].count = 1000;
        states[f].mean  = 100.0 + f;
        states[f].m2    = 999.0 * (10.0 + f); /* variance increases with feature index */
    }

    /* Initialize features with varying values */
    for (size_t s = 0; s < n_symbols; s++) {
        for (int f = 0; f < n_features; f++) {
            features[s * n_features + f] = 100.0 + f + (s % 20) - 10.0;
        }
    }

    fc_status_t status = fc_ex_sig_normalize_zscore(
        z_out,
        states,
        features,
        n_symbols,
        n_features
    );

    FC_TEST_ASSERT_EQ(status, FC_OK);

    /* Verify that all outputs are finite and reasonable */
    for (size_t s = 0; s < n_symbols; s++) {
        for (int f = 0; f < n_features; f++) {
            const double z = z_out[s * n_features + f];
            FC_TEST_ASSERT(!isnan(z));
            FC_TEST_ASSERT(!isinf(z));
            FC_TEST_ASSERT(fabs(z) < 100.0); /* Reasonable z-score range */
        }
    }

    fc_aligned_free(states);
    fc_aligned_free(features);
    fc_aligned_free(z_out);
}

/* Test numerical accuracy with known statistics */
TEST(test_normalize_numerical_accuracy) {
    const size_t n_symbols  = 5;
    const int n_features    = 1;

    /* Setup state with known mean and variance */
    fc_welford_state_t states[1];
    states[0].count = 10;
    states[0].mean  = 50.0;
    states[0].m2    = 9.0 * 100.0; /* sample variance = 100, stddev = 10 */

    /* Features: 40, 45, 50, 55, 60 */
    double features[] = {40.0, 45.0, 50.0, 55.0, 60.0};
    double z_out[5];

    fc_status_t status = fc_ex_sig_normalize_zscore(
        z_out,
        states,
        features,
        n_symbols,
        n_features
    );

    FC_TEST_ASSERT_EQ(status, FC_OK);

    /* Expected z-scores with high precision */
    FC_TEST_ASSERT_DOUBLE_EQ(z_out[0], -1.0, 1e-12);
    FC_TEST_ASSERT_DOUBLE_EQ(z_out[1], -0.5, 1e-12);
    FC_TEST_ASSERT_DOUBLE_EQ(z_out[2], 0.0, 1e-12);
    FC_TEST_ASSERT_DOUBLE_EQ(z_out[3], 0.5, 1e-12);
    FC_TEST_ASSERT_DOUBLE_EQ(z_out[4], 1.0, 1e-12);
}

/* Test registration function */
void register_normalize_tests(void) {
    RUN_TEST(test_normalize_basic_single_feature);
    RUN_TEST(test_normalize_multi_feature);
    RUN_TEST(test_normalize_zero_stddev);
    RUN_TEST(test_normalize_insufficient_data);
    RUN_TEST(test_normalize_null_pointers);
    RUN_TEST(test_normalize_invalid_dimensions);
    RUN_TEST(test_normalize_nan_input);
    RUN_TEST(test_normalize_update_states_basic);
    RUN_TEST(test_normalize_update_states_null);
    RUN_TEST(test_normalize_update_states_nan);
    RUN_TEST(test_normalize_large_batch);
    RUN_TEST(test_normalize_numerical_accuracy);
}
