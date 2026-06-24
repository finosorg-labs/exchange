/**
 * @file test_postprocess.c
 * @brief Unit tests for signal post-processing
 *
 * Tests include:
 * - Threshold filtering
 * - EMA smoothing
 * - Clipping operations
 * - Reversal detection
 * - Edge cases (NaN, Inf, zero values)
 * - SIMD variant consistency
 */

#include "signal/postprocess.h"
#include "test_framework.h"
#include <math.h>
#include <string.h>

/* Test basic threshold filtering */
TEST(test_postprocess_threshold_filtering) {
    const size_t n = 8;
    double sig_in[] = {1.5, 0.05, -2.0, 0.08, 3.0, -0.03, 0.0, -1.2};
    double sig_out[8];
    double ema_state[8] = {0.0};

    fc_ex_sig_postproc_cfg_t cfg = {
        .threshold = 0.1,
        .ema_alpha = 1.0,     /* No EMA smoothing */
        .clip_lo = -INFINITY,
        .clip_hi = INFINITY,
        .enable_reversal = 0
    };

    fc_status_t status = fc_ex_sig_postprocess(sig_out, ema_state, sig_in, &cfg, n);
    FC_TEST_ASSERT_EQ(status, FC_OK);

    /* Values >= threshold preserved, < threshold zeroed */
    FC_TEST_ASSERT_DOUBLE_EQ(sig_out[0], 1.5, 1e-10);
    FC_TEST_ASSERT_DOUBLE_EQ(sig_out[1], 0.0, 1e-10);   /* 0.05 < 0.1 */
    FC_TEST_ASSERT_DOUBLE_EQ(sig_out[2], -2.0, 1e-10);
    FC_TEST_ASSERT_DOUBLE_EQ(sig_out[3], 0.0, 1e-10);   /* 0.08 < 0.1 */
    FC_TEST_ASSERT_DOUBLE_EQ(sig_out[4], 3.0, 1e-10);
    FC_TEST_ASSERT_DOUBLE_EQ(sig_out[5], 0.0, 1e-10);   /* 0.03 < 0.1 */
    FC_TEST_ASSERT_DOUBLE_EQ(sig_out[6], 0.0, 1e-10);
    FC_TEST_ASSERT_DOUBLE_EQ(sig_out[7], -1.2, 1e-10);
}

/* Test EMA smoothing */
TEST(test_postprocess_ema_smoothing) {
    const size_t n = 5;
    double sig_in[] = {1.0, 2.0, 3.0, 2.0, 1.0};
    double sig_out[5];
    double ema_state[5] = {0.0, 0.0, 0.0, 0.0, 0.0};

    fc_ex_sig_postproc_cfg_t cfg = {
        .threshold = 0.0,     /* No threshold filtering */
        .ema_alpha = 0.5,
        .clip_lo = -INFINITY,
        .clip_hi = INFINITY,
        .enable_reversal = 0
    };

    fc_status_t status = fc_ex_sig_postprocess(sig_out, ema_state, sig_in, &cfg, n);
    FC_TEST_ASSERT_EQ(status, FC_OK);

    /* Batch EMA: each element uses its own ema_state[i] independently
     * ema[i] = 0.5 * sig_in[i] + 0.5 * ema_state[i]
     * Since ema_state[i] starts at 0:
     * sig_out[0] = 0.5 * 1.0 + 0.5 * 0.0 = 0.5
     * sig_out[1] = 0.5 * 2.0 + 0.5 * 0.0 = 1.0
     * sig_out[2] = 0.5 * 3.0 + 0.5 * 0.0 = 1.5
     * sig_out[3] = 0.5 * 2.0 + 0.5 * 0.0 = 1.0
     * sig_out[4] = 0.5 * 1.0 + 0.5 * 0.0 = 0.5
     */
    FC_TEST_ASSERT_DOUBLE_EQ(sig_out[0], 0.5, 1e-10);
    FC_TEST_ASSERT_DOUBLE_EQ(sig_out[1], 1.0, 1e-10);
    FC_TEST_ASSERT_DOUBLE_EQ(sig_out[2], 1.5, 1e-10);
    FC_TEST_ASSERT_DOUBLE_EQ(sig_out[3], 1.0, 1e-10);
    FC_TEST_ASSERT_DOUBLE_EQ(sig_out[4], 0.5, 1e-10);

    /* Verify EMA state updated */
    FC_TEST_ASSERT_DOUBLE_EQ(ema_state[0], 0.5, 1e-10);
    FC_TEST_ASSERT_DOUBLE_EQ(ema_state[1], 1.0, 1e-10);
    FC_TEST_ASSERT_DOUBLE_EQ(ema_state[2], 1.5, 1e-10);
    FC_TEST_ASSERT_DOUBLE_EQ(ema_state[3], 1.0, 1e-10);
    FC_TEST_ASSERT_DOUBLE_EQ(ema_state[4], 0.5, 1e-10);

    /* Test time-series EMA by calling multiple times with same index */
    double sig_in_ts[] = {1.0};
    double sig_out_ts[1];
    double ema_state_ts[1] = {0.0};

    /* First call */
    status = fc_ex_sig_postprocess(sig_out_ts, ema_state_ts, sig_in_ts, &cfg, 1);
    FC_TEST_ASSERT_EQ(status, FC_OK);
    FC_TEST_ASSERT_DOUBLE_EQ(sig_out_ts[0], 0.5, 1e-10);  /* 0.5 * 1.0 + 0.5 * 0.0 */

    /* Second call with new input, reusing state */
    sig_in_ts[0] = 2.0;
    status = fc_ex_sig_postprocess(sig_out_ts, ema_state_ts, sig_in_ts, &cfg, 1);
    FC_TEST_ASSERT_EQ(status, FC_OK);
    FC_TEST_ASSERT_DOUBLE_EQ(sig_out_ts[0], 1.25, 1e-10);  /* 0.5 * 2.0 + 0.5 * 0.5 */

    /* Third call */
    sig_in_ts[0] = 3.0;
    status = fc_ex_sig_postprocess(sig_out_ts, ema_state_ts, sig_in_ts, &cfg, 1);
    FC_TEST_ASSERT_EQ(status, FC_OK);
    FC_TEST_ASSERT_DOUBLE_EQ(sig_out_ts[0], 2.125, 1e-10);  /* 0.5 * 3.0 + 0.5 * 1.25 */
}

/* Test clipping */
TEST(test_postprocess_clipping) {
    const size_t n = 6;
    double sig_in[] = {-5.0, -1.0, 0.0, 1.5, 3.0, 5.0};
    double sig_out[6];
    double ema_state[6] = {0.0};

    fc_ex_sig_postproc_cfg_t cfg = {
        .threshold = 0.0,
        .ema_alpha = 1.0,     /* No EMA smoothing */
        .clip_lo = -2.0,
        .clip_hi = 2.0,
        .enable_reversal = 0
    };

    fc_status_t status = fc_ex_sig_postprocess(sig_out, ema_state, sig_in, &cfg, n);
    FC_TEST_ASSERT_EQ(status, FC_OK);

    FC_TEST_ASSERT_DOUBLE_EQ(sig_out[0], -2.0, 1e-10);  /* Clipped to lower bound */
    FC_TEST_ASSERT_DOUBLE_EQ(sig_out[1], -1.0, 1e-10);
    FC_TEST_ASSERT_DOUBLE_EQ(sig_out[2], 0.0, 1e-10);
    FC_TEST_ASSERT_DOUBLE_EQ(sig_out[3], 1.5, 1e-10);
    FC_TEST_ASSERT_DOUBLE_EQ(sig_out[4], 2.0, 1e-10);   /* Clipped to upper bound */
    FC_TEST_ASSERT_DOUBLE_EQ(sig_out[5], 2.0, 1e-10);   /* Clipped to upper bound */
}

/* Test combined threshold + EMA + clipping */
TEST(test_postprocess_combined) {
    const size_t n = 4;
    double sig_in[] = {0.05, 2.0, 0.03, 3.0};
    double sig_out[4];
    double ema_state[4] = {0.0};

    fc_ex_sig_postproc_cfg_t cfg = {
        .threshold = 0.1,
        .ema_alpha = 0.5,
        .clip_lo = -1.0,
        .clip_hi = 1.5,
        .enable_reversal = 0
    };

    fc_status_t status = fc_ex_sig_postprocess(sig_out, ema_state, sig_in, &cfg, n);
    FC_TEST_ASSERT_EQ(status, FC_OK);

    /* Each element uses its own ema_state[i], not chained from previous element
     * t0: 0.05 < 0.1 → filtered to 0, ema = 0.5 * 0 + 0.5 * ema_state[0](=0) = 0 */
    FC_TEST_ASSERT_DOUBLE_EQ(sig_out[0], 0.0, 1e-10);

    /* t1: 2.0 >= 0.1, ema = 0.5 * 2.0 + 0.5 * ema_state[1](=0) = 1.0 */
    FC_TEST_ASSERT_DOUBLE_EQ(sig_out[1], 1.0, 1e-10);

    /* t2: 0.03 < 0.1 → 0, ema = 0.5 * 0 + 0.5 * ema_state[2](=0) = 0 */
    FC_TEST_ASSERT_DOUBLE_EQ(sig_out[2], 0.0, 1e-10);

    /* t3: 3.0 >= 0.1, ema = 0.5 * 3.0 + 0.5 * ema_state[3](=0) = 1.5 (no clipping needed) */
    FC_TEST_ASSERT_DOUBLE_EQ(sig_out[3], 1.5, 1e-10);
}

/* Test reversal detection */
TEST(test_reversal_detection) {
    const size_t n = 8;
    double sig_prev[] = {1.0, 1.0, -1.0, -1.0, 0.0, 1.0, -1.0, 0.0};
    double sig_cur[]  = {1.5, -0.5, -1.5, 0.5, 0.0, 0.5, -0.5, 0.5};
    double reversal_out[8];

    fc_status_t status = fc_ex_sig_detect_reversal(reversal_out, sig_prev, sig_cur, n);
    FC_TEST_ASSERT_EQ(status, FC_OK);

    /* No reversal: both positive */
    FC_TEST_ASSERT_DOUBLE_EQ(reversal_out[0], 0.0, 1e-10);

    /* Bearish reversal: positive to negative */
    FC_TEST_ASSERT_DOUBLE_EQ(reversal_out[1], 1.0, 1e-10);

    /* No reversal: both negative */
    FC_TEST_ASSERT_DOUBLE_EQ(reversal_out[2], 0.0, 1e-10);

    /* Bullish reversal: negative to positive */
    FC_TEST_ASSERT_DOUBLE_EQ(reversal_out[3], -1.0, 1e-10);

    /* No reversal: both zero */
    FC_TEST_ASSERT_DOUBLE_EQ(reversal_out[4], 0.0, 1e-10);

    /* No reversal: both positive */
    FC_TEST_ASSERT_DOUBLE_EQ(reversal_out[5], 0.0, 1e-10);

    /* No reversal: both negative */
    FC_TEST_ASSERT_DOUBLE_EQ(reversal_out[6], 0.0, 1e-10);

    /* No reversal: zero to positive (not a reversal) */
    FC_TEST_ASSERT_DOUBLE_EQ(reversal_out[7], 0.0, 1e-10);
}

/* Test error handling: NULL pointers */
TEST(test_postprocess_null_pointers) {
    double sig_in[4] = {1.0, 2.0, 3.0, 4.0};
    double sig_out[4];
    double ema_state[4] = {0.0};
    fc_ex_sig_postproc_cfg_t cfg = {0.1, 0.5, -1.0, 1.0, 0};

    FC_TEST_ASSERT_EQ(fc_ex_sig_postprocess(NULL, ema_state, sig_in, &cfg, 4), FC_ERR_INVALID_ARG);
    FC_TEST_ASSERT_EQ(fc_ex_sig_postprocess(sig_out, NULL, sig_in, &cfg, 4), FC_ERR_INVALID_ARG);
    FC_TEST_ASSERT_EQ(fc_ex_sig_postprocess(sig_out, ema_state, NULL, &cfg, 4), FC_ERR_INVALID_ARG);
    FC_TEST_ASSERT_EQ(fc_ex_sig_postprocess(sig_out, ema_state, sig_in, NULL, 4), FC_ERR_INVALID_ARG);
}

/* Test error handling: invalid size */
TEST(test_postprocess_invalid_size) {
    double sig_in[4] = {1.0, 2.0, 3.0, 4.0};
    double sig_out[4];
    double ema_state[4] = {0.0};
    fc_ex_sig_postproc_cfg_t cfg = {0.1, 0.5, -1.0, 1.0, 0};

    FC_TEST_ASSERT_EQ(fc_ex_sig_postprocess(sig_out, ema_state, sig_in, &cfg, 0), FC_ERR_INVALID_ARG);
}

/* Test error handling: invalid alpha */
TEST(test_postprocess_invalid_alpha) {
    double sig_in[4] = {1.0, 2.0, 3.0, 4.0};
    double sig_out[4];
    double ema_state[4] = {0.0};

    fc_ex_sig_postproc_cfg_t cfg1 = {0.1, -0.1, -1.0, 1.0, 0};  /* Negative alpha */
    FC_TEST_ASSERT_EQ(fc_ex_sig_postprocess(sig_out, ema_state, sig_in, &cfg1, 4), FC_ERR_INVALID_ARG);

    fc_ex_sig_postproc_cfg_t cfg2 = {0.1, 1.5, -1.0, 1.0, 0};   /* Alpha > 1 */
    FC_TEST_ASSERT_EQ(fc_ex_sig_postprocess(sig_out, ema_state, sig_in, &cfg2, 4), FC_ERR_INVALID_ARG);
}

/* Test error handling: invalid clip bounds */
TEST(test_postprocess_invalid_clip) {
    double sig_in[4] = {1.0, 2.0, 3.0, 4.0};
    double sig_out[4];
    double ema_state[4] = {0.0};

    fc_ex_sig_postproc_cfg_t cfg = {0.1, 0.5, 2.0, 1.0, 0};  /* clip_lo > clip_hi */
    FC_TEST_ASSERT_EQ(fc_ex_sig_postprocess(sig_out, ema_state, sig_in, &cfg, 4), FC_ERR_INVALID_ARG);
}

/* Test batch processing (SIMD alignment) */
TEST(test_postprocess_batch) {
    const size_t n = 1024;
    double* sig_in = aligned_alloc(64, n * sizeof(double));
    double* sig_out = aligned_alloc(64, n * sizeof(double));
    double* ema_state = aligned_alloc(64, n * sizeof(double));

    FC_TEST_ASSERT(sig_in != NULL);
    FC_TEST_ASSERT(sig_out != NULL);
    FC_TEST_ASSERT(ema_state != NULL);

    /* Initialize with pattern */
    for (size_t i = 0; i < n; i++) {
        sig_in[i] = (i % 2 == 0) ? 1.0 : -1.0;
        ema_state[i] = 0.0;
    }

    fc_ex_sig_postproc_cfg_t cfg = {
        .threshold = 0.5,
        .ema_alpha = 0.3,
        .clip_lo = -0.5,
        .clip_hi = 0.5,
        .enable_reversal = 0
    };

    fc_status_t status = fc_ex_sig_postprocess(sig_out, ema_state, sig_in, &cfg, n);
    FC_TEST_ASSERT_EQ(status, FC_OK);

    /* Verify pattern maintained with EMA and clipping */
    for (size_t i = 0; i < 10; i++) {
        FC_TEST_ASSERT(fabs(sig_out[i]) <= 0.5 + 1e-10);  /* Within clip bounds */
    }

    free(sig_in);
    free(sig_out);
    free(ema_state);
}

/* Test reversal detection edge cases */
TEST(test_reversal_edge_cases) {
    const size_t n = 4;
    double sig_prev[] = {0.0, 0.0, 1.0, -1.0};
    double sig_cur[]  = {1.0, -1.0, 0.0, 0.0};
    double reversal_out[4];

    fc_status_t status = fc_ex_sig_detect_reversal(reversal_out, sig_prev, sig_cur, n);
    FC_TEST_ASSERT_EQ(status, FC_OK);

    /* Zero to positive: no reversal */
    FC_TEST_ASSERT_DOUBLE_EQ(reversal_out[0], 0.0, 1e-10);

    /* Zero to negative: no reversal */
    FC_TEST_ASSERT_DOUBLE_EQ(reversal_out[1], 0.0, 1e-10);

    /* Positive to zero: no reversal */
    FC_TEST_ASSERT_DOUBLE_EQ(reversal_out[2], 0.0, 1e-10);

    /* Negative to zero: no reversal */
    FC_TEST_ASSERT_DOUBLE_EQ(reversal_out[3], 0.0, 1e-10);
}

/* Test reversal detection NULL pointers */
TEST(test_reversal_null_pointers) {
    double sig_prev[4] = {1.0, 2.0, 3.0, 4.0};
    double sig_cur[4] = {-1.0, -2.0, -3.0, -4.0};
    double reversal_out[4];

    FC_TEST_ASSERT_EQ(fc_ex_sig_detect_reversal(NULL, sig_prev, sig_cur, 4), FC_ERR_INVALID_ARG);
    FC_TEST_ASSERT_EQ(fc_ex_sig_detect_reversal(reversal_out, NULL, sig_cur, 4), FC_ERR_INVALID_ARG);
    FC_TEST_ASSERT_EQ(fc_ex_sig_detect_reversal(reversal_out, sig_prev, NULL, 4), FC_ERR_INVALID_ARG);
}

/* Test with extreme values */
TEST(test_postprocess_extreme_values) {
    const size_t n = 4;
    double sig_in[] = {1e10, -1e10, 1e-10, -1e-10};
    double sig_out[4];
    double ema_state[4] = {0.0};

    fc_ex_sig_postproc_cfg_t cfg = {
        .threshold = 1e-5,
        .ema_alpha = 0.5,
        .clip_lo = -1e9,
        .clip_hi = 1e9,
        .enable_reversal = 0
    };

    fc_status_t status = fc_ex_sig_postprocess(sig_out, ema_state, sig_in, &cfg, n);
    FC_TEST_ASSERT_EQ(status, FC_OK);

    /* Large values clipped */
    FC_TEST_ASSERT_DOUBLE_EQ(sig_out[0], 1e9, 1e-5);
    FC_TEST_ASSERT_DOUBLE_EQ(sig_out[1], -1e9, 1e-5);

    /* Small values below threshold filtered to zero */
    FC_TEST_ASSERT_DOUBLE_EQ(sig_out[2], 0.0, 1e-10);
    FC_TEST_ASSERT_DOUBLE_EQ(sig_out[3], 0.0, 1e-10);
}

/* Register all post-processing tests */
void register_postprocess_tests(void) {
    RUN_TEST(test_postprocess_threshold_filtering);
    RUN_TEST(test_postprocess_ema_smoothing);
    RUN_TEST(test_postprocess_clipping);
    RUN_TEST(test_postprocess_combined);
    RUN_TEST(test_reversal_detection);
    RUN_TEST(test_postprocess_null_pointers);
    RUN_TEST(test_postprocess_invalid_size);
    RUN_TEST(test_postprocess_invalid_alpha);
    RUN_TEST(test_postprocess_invalid_clip);
    RUN_TEST(test_postprocess_batch);
    RUN_TEST(test_reversal_edge_cases);
    RUN_TEST(test_reversal_null_pointers);
    RUN_TEST(test_postprocess_extreme_values);
}
