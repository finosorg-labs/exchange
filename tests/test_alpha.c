/**
 * @file test_alpha.c
 * @brief Unit tests for Alpha factor aggregation
 *
 * Tests include:
 * - Basic weighted aggregation
 * - Confidence scoring
 * - Weight normalization
 * - Per-symbol vs uniform weights
 * - Signal agreement computation
 * - Inverse volatility weighting
 * - Edge cases (NaN, Inf, zero weights)
 * - Minimum confidence threshold
 */

#include "signal/alpha.h"
#include "test_framework.h"
#include <math.h>
#include <string.h>

/* Test basic weighted aggregation with uniform weights */
TEST(test_alpha_basic_aggregation) {
    const size_t n_symbols = 3;
    const int n_signals    = 4;

    /* Signals: 3 symbols × 4 signals */
    double signals[] = {
        1.0,
        2.0,
        3.0,
        4.0, /* Symbol 0: all positive */
        -1.0,
        -2.0,
        -3.0,
        -4.0, /* Symbol 1: all negative */
        1.0,
        -1.0,
        2.0,
        -2.0 /* Symbol 2: mixed */
    };

    /* Uniform weights */
    double weights[] = {0.25, 0.25, 0.25, 0.25};

    double alpha_out[3];
    double confidence_out[3];

    fc_ex_alpha_cfg_t cfg = {
        .normalize_weights  = 0, /* Already normalized */
        .per_symbol_weights = 0, /* Uniform weights */
        .min_confidence     = 0.0,
        .strength_scale     = 1.0 /* Default scale for normalized signals */
    };

    fc_status_t status = fc_ex_sig_alpha_aggregate(
        alpha_out, confidence_out, signals, weights, &cfg, n_symbols, n_signals
    );

    FC_TEST_ASSERT_EQ(status, FC_OK);

    /* Symbol 0: 0.25 * (1 + 2 + 3 + 4) = 2.5 */
    FC_TEST_ASSERT_DOUBLE_EQ(alpha_out[0], 2.5, 1e-10);
    /* High confidence: all signals positive */
    FC_TEST_ASSERT(confidence_out[0] > 0.9);

    /* Symbol 1: 0.25 * (-1 - 2 - 3 - 4) = -2.5 */
    FC_TEST_ASSERT_DOUBLE_EQ(alpha_out[1], -2.5, 1e-10);
    /* High confidence: all signals negative */
    FC_TEST_ASSERT(confidence_out[1] > 0.9);

    /* Symbol 2: 0.25 * (1 - 1 + 2 - 2) = 0.0 */
    FC_TEST_ASSERT_DOUBLE_EQ(alpha_out[2], 0.0, 1e-10);
    /* Lower confidence: mixed signals */
    FC_TEST_ASSERT(confidence_out[2] < 0.7);
}

/* Test weight normalization */
TEST(test_alpha_weight_normalization) {
    const size_t n_symbols = 2;
    const int n_signals    = 3;

    double signals[] = {1.0, 2.0, 3.0, -1.0, -2.0, -3.0};

    /* Non-normalized weights */
    double weights[] = {1.0, 2.0, 3.0}; /* Sum = 6.0 */

    double alpha_out[2];
    double confidence_out[2];

    fc_ex_alpha_cfg_t cfg = {
        .normalize_weights  = 1, /* Enable normalization */
        .per_symbol_weights = 0,
        .min_confidence     = 0.0,
        .strength_scale     = 1.0
    };

    fc_status_t status = fc_ex_sig_alpha_aggregate(
        alpha_out, confidence_out, signals, weights, &cfg, n_symbols, n_signals
    );

    FC_TEST_ASSERT_EQ(status, FC_OK);

    /* Normalized weights: 1/6, 2/6, 3/6 */
    /* Symbol 0: (1/6)*1 + (2/6)*2 + (3/6)*3 = 1/6 + 4/6 + 9/6 = 14/6 ≈ 2.333... */
    FC_TEST_ASSERT_DOUBLE_EQ(alpha_out[0], 14.0 / 6.0, 1e-10);

    /* Symbol 1: (1/6)*(-1) + (2/6)*(-2) + (3/6)*(-3) = -14/6 */
    FC_TEST_ASSERT_DOUBLE_EQ(alpha_out[1], -14.0 / 6.0, 1e-10);
}

/* Test per-symbol weights */
TEST(test_alpha_per_symbol_weights) {
    const size_t n_symbols = 2;
    const int n_signals    = 3;

    double signals[] = {1.0, 2.0, 3.0, 1.0, 2.0, 3.0};

    /* Per-symbol weights: different for each symbol */
    double weights[] = {
        0.5,
        0.3,
        0.2, /* Symbol 0 weights */
        0.2,
        0.3,
        0.5 /* Symbol 1 weights */
    };

    double alpha_out[2];
    double confidence_out[2];

    fc_ex_alpha_cfg_t cfg = {
        .normalize_weights  = 0,
        .per_symbol_weights = 1, /* Per-symbol weights */
        .min_confidence     = 0.0,
        .strength_scale     = 1.0
    };

    fc_status_t status = fc_ex_sig_alpha_aggregate(
        alpha_out, confidence_out, signals, weights, &cfg, n_symbols, n_signals
    );

    FC_TEST_ASSERT_EQ(status, FC_OK);

    /* Symbol 0: 0.5*1 + 0.3*2 + 0.2*3 = 0.5 + 0.6 + 0.6 = 1.7 */
    FC_TEST_ASSERT_DOUBLE_EQ(alpha_out[0], 1.7, 1e-10);

    /* Symbol 1: 0.2*1 + 0.3*2 + 0.5*3 = 0.2 + 0.6 + 1.5 = 2.3 */
    FC_TEST_ASSERT_DOUBLE_EQ(alpha_out[1], 2.3, 1e-10);
}

/* Test minimum confidence threshold */
TEST(test_alpha_min_confidence_threshold) {
    const size_t n_symbols = 2;
    const int n_signals    = 3;

    double signals[] = {
        1.0,
        2.0,
        3.0, /* Symbol 0: strong agreement */
        1.0,
        -1.0,
        0.5 /* Symbol 1: weak/mixed */
    };

    double weights[] = {0.33, 0.33, 0.34};

    double alpha_out[2];
    double confidence_out[2];

    fc_ex_alpha_cfg_t cfg = {
        .normalize_weights  = 0,
        .per_symbol_weights = 0,
        .min_confidence     = 0.7, /* High threshold */
        .strength_scale     = 1.0
    };

    fc_status_t status = fc_ex_sig_alpha_aggregate(
        alpha_out, confidence_out, signals, weights, &cfg, n_symbols, n_signals
    );

    FC_TEST_ASSERT_EQ(status, FC_OK);

    /* Symbol 0: high confidence, alpha preserved */
    FC_TEST_ASSERT(confidence_out[0] >= 0.7);
    FC_TEST_ASSERT(alpha_out[0] > 0.0);

    /* Symbol 1: low confidence, alpha zeroed */
    FC_TEST_ASSERT(confidence_out[1] < 0.7);
    FC_TEST_ASSERT_DOUBLE_EQ(alpha_out[1], 0.0, 1e-10);
}

/* Test NaN/Inf handling */
TEST(test_alpha_nan_inf_handling) {
    const size_t n_symbols = 3;
    const int n_signals    = 4;

    double signals[] = {
        1.0,
        2.0,
        NAN,
        3.0, /* Symbol 0: contains NaN */
        1.0,
        INFINITY,
        2.0,
        3.0, /* Symbol 1: contains Inf */
        NAN,
        NAN,
        NAN,
        NAN /* Symbol 2: all NaN */
    };

    double weights[] = {0.25, 0.25, 0.25, 0.25};

    double alpha_out[3];
    double confidence_out[3];

    fc_ex_alpha_cfg_t cfg = {
        .normalize_weights  = 0,
        .per_symbol_weights = 0,
        .min_confidence     = 0.0,
        .strength_scale     = 1.0
    };

    fc_status_t status = fc_ex_sig_alpha_aggregate(
        alpha_out, confidence_out, signals, weights, &cfg, n_symbols, n_signals
    );

    FC_TEST_ASSERT_EQ(status, FC_OK);

    /* Symbol 0: NaN ignored, alpha = 0.25 * (1 + 2 + 3) = 1.5 */
    FC_TEST_ASSERT_DOUBLE_EQ(alpha_out[0], 1.5, 1e-10);
    /* Confidence reduced due to NaN */
    FC_TEST_ASSERT(confidence_out[0] > 0.0);

    /* Symbol 1: Inf ignored, alpha = 0.25 * (1 + 2 + 3) = 1.5 */
    FC_TEST_ASSERT_DOUBLE_EQ(alpha_out[1], 1.5, 1e-10);

    /* Symbol 2: all NaN, alpha = 0, confidence = 0 */
    FC_TEST_ASSERT_DOUBLE_EQ(alpha_out[2], 0.0, 1e-10);
    FC_TEST_ASSERT_DOUBLE_EQ(confidence_out[2], 0.0, 1e-10);
}

/* Test weight normalization with zero weights */
TEST(test_alpha_zero_weights) {
    const size_t n_symbols = 1;
    const int n_signals    = 3;

    double signals[] = {1.0, 2.0, 3.0};
    double weights[] = {0.0, 0.0, 0.0}; /* All zero */

    double alpha_out[1];
    double confidence_out[1];

    fc_ex_alpha_cfg_t cfg = {
        .normalize_weights  = 1,
        .per_symbol_weights = 0,
        .min_confidence     = 0.0,
        .strength_scale     = 1.0
    };

    fc_status_t status = fc_ex_sig_alpha_aggregate(
        alpha_out, confidence_out, signals, weights, &cfg, n_symbols, n_signals
    );

    FC_TEST_ASSERT_EQ(status, FC_OK);

    /* Zero weight sum results in zero alpha and confidence */
    FC_TEST_ASSERT_DOUBLE_EQ(alpha_out[0], 0.0, 1e-10);
    FC_TEST_ASSERT_DOUBLE_EQ(confidence_out[0], 0.0, 1e-10);
}

/* Test standalone weight normalization */
TEST(test_normalize_weights) {
    const size_t n_symbols = 2;
    const int n_signals    = 3;

    double weights_in[] = {
        1.0,
        2.0,
        3.0, /* Symbol 0 */
        4.0,
        5.0,
        6.0 /* Symbol 1 */
    };

    double weights_out[6];

    fc_status_t status = fc_ex_sig_normalize_weights(weights_out, weights_in, n_symbols, n_signals);
    FC_TEST_ASSERT_EQ(status, FC_OK);

    /* Symbol 0: sum = 6, normalized = 1/6, 2/6, 3/6 */
    FC_TEST_ASSERT_DOUBLE_EQ(weights_out[0], 1.0 / 6.0, 1e-10);
    FC_TEST_ASSERT_DOUBLE_EQ(weights_out[1], 2.0 / 6.0, 1e-10);
    FC_TEST_ASSERT_DOUBLE_EQ(weights_out[2], 3.0 / 6.0, 1e-10);

    /* Symbol 1: sum = 15, normalized = 4/15, 5/15, 6/15 */
    FC_TEST_ASSERT_DOUBLE_EQ(weights_out[3], 4.0 / 15.0, 1e-10);
    FC_TEST_ASSERT_DOUBLE_EQ(weights_out[4], 5.0 / 15.0, 1e-10);
    FC_TEST_ASSERT_DOUBLE_EQ(weights_out[5], 6.0 / 15.0, 1e-10);

    /* Verify sums to 1.0 */
    double sum0 = weights_out[0] + weights_out[1] + weights_out[2];
    double sum1 = weights_out[3] + weights_out[4] + weights_out[5];
    FC_TEST_ASSERT_DOUBLE_EQ(sum0, 1.0, 1e-10);
    FC_TEST_ASSERT_DOUBLE_EQ(sum1, 1.0, 1e-10);
}

/* Test signal agreement computation */
TEST(test_compute_agreement) {
    const size_t n_symbols = 3;
    const int n_signals    = 4;

    double signals[] = {
        1.0,
        2.0,
        3.0,
        4.0, /* Symbol 0: all positive, increasing */
        -1.0,
        -2.0,
        -3.0,
        -4.0, /* Symbol 1: all negative, decreasing */
        1.0,
        1.0,
        1.0,
        1.0 /* Symbol 2: all same (low variance) */
    };

    double agreement_out[3];

    fc_status_t status = fc_ex_sig_compute_agreement(agreement_out, signals, n_symbols, n_signals);
    FC_TEST_ASSERT_EQ(status, FC_OK);

    /* Symbol 0: mean = 2.5, high positive agreement */
    FC_TEST_ASSERT(agreement_out[0] > 0.0);

    /* Symbol 1: mean = -2.5, high negative agreement */
    FC_TEST_ASSERT(agreement_out[1] < 0.0);

    /* Symbol 2: mean = 1.0, but std = 0, so very high agreement */
    FC_TEST_ASSERT(agreement_out[2] > agreement_out[0]);
}

/* Test inverse volatility weighting */
TEST(test_inverse_vol_weights) {
    const size_t window_size = 5;
    const int n_signals      = 3;

    /* Historical signals: 5 time steps × 3 signals */
    double signals_hist[] = {
        1.0,
        5.0,
        10.0, /* t=0 */
        1.1,
        5.5,
        12.0, /* t=1 */
        0.9,
        4.5,
        8.0, /* t=2 */
        1.0,
        5.0,
        11.0, /* t=3 */
        1.0,
        6.0,
        9.0 /* t=4 */
    };

    double weights_out[3];
    double work_buffer[3]; /* Work buffer for n_signals doubles */

    fc_status_t status = fc_ex_sig_inverse_vol_weights(
        weights_out, signals_hist, work_buffer, window_size, n_signals
    );
    FC_TEST_ASSERT_EQ(status, FC_OK);

    /* Signal 0 has lowest volatility, should get highest weight */
    /* Signal 2 has highest volatility, should get lowest weight */
    FC_TEST_ASSERT(weights_out[0] > weights_out[1]);
    FC_TEST_ASSERT(weights_out[1] > weights_out[2]);

    /* Weights should sum to 1.0 */
    double sum = weights_out[0] + weights_out[1] + weights_out[2];
    FC_TEST_ASSERT_DOUBLE_EQ(sum, 1.0, 1e-10);
}

/* Test input validation */
TEST(test_alpha_input_validation) {
    double alpha_out[1];
    double confidence_out[1];
    double signals[] = {1.0};
    double weights[] = {1.0};

    fc_ex_alpha_cfg_t cfg = {
        .normalize_weights  = 0,
        .per_symbol_weights = 0,
        .min_confidence     = 0.0,
        .strength_scale     = 1.0
    };

    /* NULL pointers */
    FC_TEST_ASSERT_EQ(
        fc_ex_sig_alpha_aggregate(NULL, confidence_out, signals, weights, &cfg, 1, 1),
        FC_ERR_INVALID_ARG
    );
    FC_TEST_ASSERT_EQ(
        fc_ex_sig_alpha_aggregate(alpha_out, NULL, signals, weights, &cfg, 1, 1), FC_ERR_INVALID_ARG
    );
    FC_TEST_ASSERT_EQ(
        fc_ex_sig_alpha_aggregate(alpha_out, confidence_out, NULL, weights, &cfg, 1, 1),
        FC_ERR_INVALID_ARG
    );
    FC_TEST_ASSERT_EQ(
        fc_ex_sig_alpha_aggregate(alpha_out, confidence_out, signals, NULL, &cfg, 1, 1),
        FC_ERR_INVALID_ARG
    );
    FC_TEST_ASSERT_EQ(
        fc_ex_sig_alpha_aggregate(alpha_out, confidence_out, signals, weights, NULL, 1, 1),
        FC_ERR_INVALID_ARG
    );

    /* Zero sizes */
    FC_TEST_ASSERT_EQ(
        fc_ex_sig_alpha_aggregate(alpha_out, confidence_out, signals, weights, &cfg, 0, 1),
        FC_ERR_INVALID_ARG
    );
    FC_TEST_ASSERT_EQ(
        fc_ex_sig_alpha_aggregate(alpha_out, confidence_out, signals, weights, &cfg, 1, 0),
        FC_ERR_INVALID_ARG
    );

    /* Invalid min_confidence */
    cfg.min_confidence = -0.1;
    FC_TEST_ASSERT_EQ(
        fc_ex_sig_alpha_aggregate(alpha_out, confidence_out, signals, weights, &cfg, 1, 1),
        FC_ERR_INVALID_ARG
    );
    cfg.min_confidence = 1.1;
    FC_TEST_ASSERT_EQ(
        fc_ex_sig_alpha_aggregate(alpha_out, confidence_out, signals, weights, &cfg, 1, 1),
        FC_ERR_INVALID_ARG
    );

    /* Invalid strength_scale (must be > 0) */
    cfg.min_confidence = 0.5;
    cfg.strength_scale = 0.0;
    FC_TEST_ASSERT_EQ(
        fc_ex_sig_alpha_aggregate(alpha_out, confidence_out, signals, weights, &cfg, 1, 1),
        FC_ERR_INVALID_ARG
    );
    cfg.strength_scale = -1.0;
    FC_TEST_ASSERT_EQ(
        fc_ex_sig_alpha_aggregate(alpha_out, confidence_out, signals, weights, &cfg, 1, 1),
        FC_ERR_INVALID_ARG
    );
}

/* Test strength_scale impact on confidence */
TEST(test_alpha_strength_scale) {
    const size_t n_symbols = 2;
    const int n_signals    = 3;

    /* Symbol 0: weak signals (all 0.1), all positive - high agreement, low strength */
    /* Symbol 1: strong signals (all 10.0), all positive - high agreement, high strength */
    double signals[] = {
        0.1,
        0.1,
        0.1, /* Symbol 0 */
        10.0,
        10.0,
        10.0 /* Symbol 1 */
    };
    double weights[] = {0.33, 0.33, 0.34};

    double alpha_out1[2], confidence_out1[2];
    double alpha_out2[2], confidence_out2[2];

    /* Test with scale=1.0 (assumes signals in [-1,1]) */
    fc_ex_alpha_cfg_t cfg1 = {
        .normalize_weights  = 0,
        .per_symbol_weights = 0,
        .min_confidence     = 0.0,
        .strength_scale     = 1.0
    };

    fc_status_t status = fc_ex_sig_alpha_aggregate(
        alpha_out1, confidence_out1, signals, weights, &cfg1, n_symbols, n_signals
    );
    FC_TEST_ASSERT_EQ(status, FC_OK);

    /* Test with scale=10.0 (assumes larger signals) */
    fc_ex_alpha_cfg_t cfg2 = {
        .normalize_weights  = 0,
        .per_symbol_weights = 0,
        .min_confidence     = 0.0,
        .strength_scale     = 10.0
    };

    status = fc_ex_sig_alpha_aggregate(
        alpha_out2, confidence_out2, signals, weights, &cfg2, n_symbols, n_signals
    );
    FC_TEST_ASSERT_EQ(status, FC_OK);

    /* With scale=1.0:
     * - Symbol 0 (avg_strength=0.1): tanh(0.1/1.0)=0.099 -> low confidence
     * - Symbol 1 (avg_strength=10.0): tanh(10.0/1.0)=0.999 -> very high confidence
     */
    FC_TEST_ASSERT(confidence_out1[0] < 0.2); /* Weak signal, low confidence */
    FC_TEST_ASSERT(confidence_out1[1] > 0.9); /* Strong signal, high confidence */

    /* With scale=10.0:
     * - Symbol 0 (avg_strength=0.1): tanh(0.1/10.0)=0.01 -> very low confidence
     * - Symbol 1 (avg_strength=10.0): tanh(10.0/10.0)=tanh(1.0)=0.76 -> moderate-high confidence
     */
    FC_TEST_ASSERT(confidence_out2[0] < 0.05); /* Weak signal with larger scale */
    FC_TEST_ASSERT(
        confidence_out2[1] > 0.7 && confidence_out2[1] < 0.8
    ); /* Strong signal normalized */

    /* Confidence for symbol 1 should be lower with larger scale (less saturated) */
    FC_TEST_ASSERT(confidence_out2[1] < confidence_out1[1]);
}

/* Test large batch */
TEST(test_alpha_large_batch) {
    const size_t n_symbols = 1000;
    const int n_signals    = 10;

    /* Align sizes to 64-byte boundary */
    size_t signals_size = ((n_symbols * n_signals * sizeof(double) + 63) / 64) * 64;
    size_t weights_size = ((n_signals * sizeof(double) + 63) / 64) * 64;
    size_t output_size  = ((n_symbols * sizeof(double) + 63) / 64) * 64;

    double* signals        = (double*) aligned_alloc(64, signals_size);
    double* weights        = (double*) aligned_alloc(64, weights_size);
    double* alpha_out      = (double*) aligned_alloc(64, output_size);
    double* confidence_out = (double*) aligned_alloc(64, output_size);

    FC_TEST_ASSERT(signals && weights && alpha_out && confidence_out);

    /* Initialize with pattern */
    for (size_t i = 0; i < n_symbols; i++) {
        for (int j = 0; j < n_signals; j++) {
            signals[i * n_signals + j] = sin((double) (i + j) * 0.1);
        }
    }

    for (int j = 0; j < n_signals; j++) {
        weights[j] = 1.0 / (double) n_signals;
    }

    fc_ex_alpha_cfg_t cfg = {
        .normalize_weights  = 0,
        .per_symbol_weights = 0,
        .min_confidence     = 0.0,
        .strength_scale     = 1.0
    };

    fc_status_t status = fc_ex_sig_alpha_aggregate(
        alpha_out, confidence_out, signals, weights, &cfg, n_symbols, n_signals
    );

    FC_TEST_ASSERT_EQ(status, FC_OK);

    /* Verify reasonable results */
    for (size_t i = 0; i < n_symbols; i++) {
        FC_TEST_ASSERT(!isnan(alpha_out[i]));
        FC_TEST_ASSERT(!isinf(alpha_out[i]));
        FC_TEST_ASSERT(confidence_out[i] >= 0.0 && confidence_out[i] <= 1.0);
    }

    free(signals);
    free(weights);
    free(alpha_out);
    free(confidence_out);
}

/* Register all alpha tests */
void register_alpha_tests(void) {
    RUN_TEST(test_alpha_basic_aggregation);
    RUN_TEST(test_alpha_weight_normalization);
    RUN_TEST(test_alpha_per_symbol_weights);
    RUN_TEST(test_alpha_min_confidence_threshold);
    RUN_TEST(test_alpha_nan_inf_handling);
    RUN_TEST(test_alpha_zero_weights);
    RUN_TEST(test_normalize_weights);
    RUN_TEST(test_compute_agreement);
    RUN_TEST(test_inverse_vol_weights);
    RUN_TEST(test_alpha_input_validation);
    RUN_TEST(test_alpha_strength_scale);
    RUN_TEST(test_alpha_large_batch);
}
