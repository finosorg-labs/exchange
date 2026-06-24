/**
 * @file test_feature.c
 * @brief Unit tests for feature extraction from order book
 */

#include "signal/feature.h"
#include "test_framework.h"
#include "platform.h"
#include "error.h"
#include "mem_aligned.h"
#include <math.h>
#include <string.h>

TEST(test_feature_count) {
    FC_TEST_ASSERT_EQ(fc_ex_sig_feature_count(1), 17);
    FC_TEST_ASSERT_EQ(fc_ex_sig_feature_count(5), 49);
    FC_TEST_ASSERT_EQ(fc_ex_sig_feature_count(10), 89);
}

TEST(test_feature_basic_single_symbol) {
    /* Single symbol, 5 levels */
    const int n_levels = 5;
    double features[64];

    /* SoA layout: bid_p[level0_sym0, level1_sym0, ...] */
    double bid_p[5] = {100.0, 99.9, 99.8, 99.7, 99.6};
    double bid_q[5] = {500.0, 400.0, 300.0, 200.0, 100.0};
    double ask_p[5] = {100.5, 100.6, 100.7, 100.8, 100.9};
    double ask_q[5] = {450.0, 350.0, 250.0, 150.0, 50.0};

    fc_status_t status = fc_ex_sig_feature_extract(features, bid_p, bid_q, ask_p, ask_q, 1, n_levels);

    FC_TEST_ASSERT_EQ(status, FC_OK);

    /* Verify core features [0-8] */
    FC_TEST_ASSERT_DOUBLE_EQ(features[0], 100.0, 1e-10);  /* best_bid_p */
    FC_TEST_ASSERT_DOUBLE_EQ(features[1], 100.5, 1e-10);  /* best_ask_p */
    FC_TEST_ASSERT_DOUBLE_EQ(features[2], 500.0, 1e-10);  /* best_bid_q */
    FC_TEST_ASSERT_DOUBLE_EQ(features[3], 450.0, 1e-10);  /* best_ask_q */
    FC_TEST_ASSERT_DOUBLE_EQ(features[4], 100.25, 1e-10); /* mid_price */

    /* micro_price = (100.0 * 450.0 + 100.5 * 500.0) / 950.0 */
    double expected_micro = (100.0 * 450.0 + 100.5 * 500.0) / 950.0;
    FC_TEST_ASSERT_DOUBLE_EQ(features[5], expected_micro, 1e-10);

    FC_TEST_ASSERT_DOUBLE_EQ(features[6], 0.5, 1e-10); /* spread_abs */

    /* spread_rel = 0.5 / 100.25 */
    FC_TEST_ASSERT_DOUBLE_EQ(features[7], 0.5 / 100.25, 1e-10);

    /* depth_imbal = (1500 - 1250) / 2750 */
    double total_bid = 500.0 + 400.0 + 300.0 + 200.0 + 100.0;
    double total_ask = 450.0 + 350.0 + 250.0 + 150.0 + 50.0;
    double expected_imbal = (total_bid - total_ask) / (total_bid + total_ask);
    FC_TEST_ASSERT_DOUBLE_EQ(features[8], expected_imbal, 1e-10);

    /* Verify first level features [9-16] */
    FC_TEST_ASSERT_DOUBLE_EQ(features[9], 100.0, 1e-10);   /* bid_p[0] */
    FC_TEST_ASSERT_DOUBLE_EQ(features[10], 500.0, 1e-10);  /* bid_q[0] */
    FC_TEST_ASSERT_DOUBLE_EQ(features[11], 100.5, 1e-10);  /* ask_p[0] */
    FC_TEST_ASSERT_DOUBLE_EQ(features[12], 450.0, 1e-10);  /* ask_q[0] */
    FC_TEST_ASSERT_DOUBLE_EQ(features[13], 500.0/1500.0, 1e-10);  /* bid_ratio */
    FC_TEST_ASSERT_DOUBLE_EQ(features[14], 450.0/1250.0, 1e-10);  /* ask_ratio */
    FC_TEST_ASSERT_DOUBLE_EQ(features[15], 0.0, 1e-10);    /* bid_gap (k=0) */
    FC_TEST_ASSERT_DOUBLE_EQ(features[16], 0.0, 1e-10);    /* ask_gap (k=0) */
}

TEST(test_feature_multi_symbol) {
    /* 3 symbols, 2 levels */
    const int n_symbols = 3;
    const int n_levels = 2;
    const size_t n_features = fc_ex_sig_feature_count(n_levels);

    double* features = fc_aligned_alloc(n_symbols * n_features * sizeof(double), 64);
    double* bid_p = fc_aligned_alloc(n_symbols * n_levels * sizeof(double), 64);
    double* bid_q = fc_aligned_alloc(n_symbols * n_levels * sizeof(double), 64);
    double* ask_p = fc_aligned_alloc(n_symbols * n_levels * sizeof(double), 64);
    double* ask_q = fc_aligned_alloc(n_symbols * n_levels * sizeof(double), 64);

    /* SoA layout: [level0_sym0, level0_sym1, level0_sym2, level1_sym0, level1_sym1, level1_sym2] */
    /* Symbol 0 */
    bid_p[0] = 100.0; bid_q[0] = 500.0; ask_p[0] = 100.5; ask_q[0] = 500.0;
    bid_p[3] = 99.9;  bid_q[3] = 400.0; ask_p[3] = 100.6; ask_q[3] = 400.0;

    /* Symbol 1 */
    bid_p[1] = 50.0;  bid_q[1] = 1000.0; ask_p[1] = 50.1; ask_q[1] = 900.0;
    bid_p[4] = 49.9;  bid_q[4] = 800.0;  ask_p[4] = 50.2; ask_q[4] = 700.0;

    /* Symbol 2 */
    bid_p[2] = 200.0; bid_q[2] = 100.0; ask_p[2] = 201.0; ask_q[2] = 100.0;
    bid_p[5] = 199.0; bid_q[5] = 80.0;  ask_p[5] = 202.0; ask_q[5] = 80.0;

    fc_status_t status = fc_ex_sig_feature_extract(features, bid_p, bid_q, ask_p, ask_q, n_symbols, n_levels);

    FC_TEST_ASSERT_EQ(status, FC_OK);

    /* Verify symbol 0 core features */
    FC_TEST_ASSERT_DOUBLE_EQ(features[0], 100.0, 1e-10);
    FC_TEST_ASSERT_DOUBLE_EQ(features[1], 100.5, 1e-10);
    FC_TEST_ASSERT_DOUBLE_EQ(features[4], 100.25, 1e-10);

    /* Verify symbol 1 core features */
    size_t sym1_offset = n_features;
    FC_TEST_ASSERT_DOUBLE_EQ(features[sym1_offset + 0], 50.0, 1e-10);
    FC_TEST_ASSERT_DOUBLE_EQ(features[sym1_offset + 1], 50.1, 1e-10);
    FC_TEST_ASSERT_DOUBLE_EQ(features[sym1_offset + 4], 50.05, 1e-10);

    /* Verify symbol 2 core features */
    size_t sym2_offset = 2 * n_features;
    FC_TEST_ASSERT_DOUBLE_EQ(features[sym2_offset + 0], 200.0, 1e-10);
    FC_TEST_ASSERT_DOUBLE_EQ(features[sym2_offset + 1], 201.0, 1e-10);
    FC_TEST_ASSERT_DOUBLE_EQ(features[sym2_offset + 4], 200.5, 1e-10);

    fc_aligned_free(features);
    fc_aligned_free(bid_p);
    fc_aligned_free(bid_q);
    fc_aligned_free(ask_p);
    fc_aligned_free(ask_q);
}

TEST(test_feature_edge_cases) {
    const int n_levels = 3;
    double features[64];

    /* Zero quantities */
    double bid_p[3] = {100.0, 99.9, 99.8};
    double bid_q[3] = {0.0, 0.0, 0.0};
    double ask_p[3] = {100.5, 100.6, 100.7};
    double ask_q[3] = {0.0, 0.0, 0.0};

    fc_status_t status = fc_ex_sig_feature_extract(features, bid_p, bid_q, ask_p, ask_q, 1, n_levels);

    FC_TEST_ASSERT_EQ(status, FC_OK);
    ASSERT_TRUE(isnan(features[5]));  /* micro_price should be NaN */
    ASSERT_TRUE(isnan(features[8]));  /* depth_imbal should be NaN */
}

TEST(test_feature_null_pointers) {
    double features[10];
    double bid_p[5] = {100.0, 99.9, 99.8, 99.7, 99.6};
    double bid_q[5] = {500.0, 400.0, 300.0, 200.0, 100.0};
    double ask_p[5] = {100.5, 100.6, 100.7, 100.8, 100.9};
    double ask_q[5] = {500.0, 400.0, 300.0, 200.0, 100.0};

    FC_TEST_ASSERT_EQ(fc_ex_sig_feature_extract(NULL, bid_p, bid_q, ask_p, ask_q, 1, 5), FC_ERR_INVALID_ARG);
    FC_TEST_ASSERT_EQ(fc_ex_sig_feature_extract(features, NULL, bid_q, ask_p, ask_q, 1, 5), FC_ERR_INVALID_ARG);
    FC_TEST_ASSERT_EQ(fc_ex_sig_feature_extract(features, bid_p, NULL, ask_p, ask_q, 1, 5), FC_ERR_INVALID_ARG);
    FC_TEST_ASSERT_EQ(fc_ex_sig_feature_extract(features, bid_p, bid_q, NULL, ask_q, 1, 5), FC_ERR_INVALID_ARG);
    FC_TEST_ASSERT_EQ(fc_ex_sig_feature_extract(features, bid_p, bid_q, ask_p, NULL, 1, 5), FC_ERR_INVALID_ARG);
}

TEST(test_feature_invalid_size) {
    double features[10];
    double bid_p[5] = {100.0, 99.9, 99.8, 99.7, 99.6};
    double bid_q[5] = {500.0, 400.0, 300.0, 200.0, 100.0};
    double ask_p[5] = {100.5, 100.6, 100.7, 100.8, 100.9};
    double ask_q[5] = {500.0, 400.0, 300.0, 200.0, 100.0};

    FC_TEST_ASSERT_EQ(fc_ex_sig_feature_extract(features, bid_p, bid_q, ask_p, ask_q, 0, 5), FC_ERR_INVALID_ARG);
    FC_TEST_ASSERT_EQ(fc_ex_sig_feature_extract(features, bid_p, bid_q, ask_p, ask_q, 1, 0), FC_ERR_INVALID_ARG);
    FC_TEST_ASSERT_EQ(fc_ex_sig_feature_extract(features, bid_p, bid_q, ask_p, ask_q, 1, -1), FC_ERR_INVALID_ARG);
    FC_TEST_ASSERT_EQ(fc_ex_sig_feature_extract(features, bid_p, bid_q, ask_p, ask_q, 1, 33), FC_ERR_INVALID_ARG);
}

TEST(test_feature_price_gaps) {
    const int n_levels = 3;
    double features[64];

    double bid_p[3] = {100.0, 99.8, 99.5};  /* gaps: 0.2, 0.3 */
    double bid_q[3] = {500.0, 400.0, 300.0};
    double ask_p[3] = {100.5, 100.7, 101.0}; /* gaps: 0.2, 0.3 */
    double ask_q[3] = {500.0, 400.0, 300.0};

    fc_status_t status = fc_ex_sig_feature_extract(features, bid_p, bid_q, ask_p, ask_q, 1, n_levels);

    FC_TEST_ASSERT_EQ(status, FC_OK);

    /* Level 0: features[15] = bid_gap = 0, features[16] = ask_gap = 0 */
    FC_TEST_ASSERT_DOUBLE_EQ(features[15], 0.0, 1e-10);
    FC_TEST_ASSERT_DOUBLE_EQ(features[16], 0.0, 1e-10);

    /* Level 1: features[23] = bid_gap = 100.0 - 99.8 = 0.2, features[24] = ask_gap = 100.7 - 100.5 = 0.2 */
    FC_TEST_ASSERT_DOUBLE_EQ(features[23], 0.2, 1e-10);
    FC_TEST_ASSERT_DOUBLE_EQ(features[24], 0.2, 1e-10);

    /* Level 2: features[31] = bid_gap = 99.8 - 99.5 = 0.3, features[32] = ask_gap = 101.0 - 100.7 = 0.3 */
    FC_TEST_ASSERT_DOUBLE_EQ(features[31], 0.3, 1e-10);
    FC_TEST_ASSERT_DOUBLE_EQ(features[32], 0.3, 1e-10);
}

TEST(test_feature_core_only) {
    const int n_symbols = 2;
    const int n_levels = 5;
    double features[18];

    /* SoA layout: 2 symbols, 5 levels */
    double bid_p[10] = {100.0, 50.0, 99.9, 49.9, 99.8, 49.8, 99.7, 49.7, 99.6, 49.6};
    double bid_q[10] = {500.0, 1000.0, 400.0, 800.0, 300.0, 600.0, 200.0, 400.0, 100.0, 200.0};
    double ask_p[10] = {100.5, 50.1, 100.6, 50.2, 100.7, 50.3, 100.8, 50.4, 100.9, 50.5};
    double ask_q[10] = {500.0, 900.0, 400.0, 700.0, 300.0, 500.0, 200.0, 300.0, 100.0, 100.0};

    fc_status_t status = fc_ex_sig_feature_extract_core(features, bid_p, bid_q, ask_p, ask_q, n_symbols, n_levels);

    FC_TEST_ASSERT_EQ(status, FC_OK);

    /* Symbol 0 */
    FC_TEST_ASSERT_DOUBLE_EQ(features[0], 100.0, 1e-10);
    FC_TEST_ASSERT_DOUBLE_EQ(features[1], 100.5, 1e-10);
    FC_TEST_ASSERT_DOUBLE_EQ(features[4], 100.25, 1e-10);

    /* Symbol 1 */
    FC_TEST_ASSERT_DOUBLE_EQ(features[9], 50.0, 1e-10);
    FC_TEST_ASSERT_DOUBLE_EQ(features[10], 50.1, 1e-10);
    FC_TEST_ASSERT_DOUBLE_EQ(features[13], 50.05, 1e-10);
}

TEST(test_feature_depth_ratios) {
    const int n_levels = 2;
    double features[64];

    double bid_p[2] = {100.0, 99.9};
    double bid_q[2] = {600.0, 400.0};  /* total = 1000, ratios: 0.6, 0.4 */
    double ask_p[2] = {100.5, 100.6};
    double ask_q[2] = {300.0, 200.0};  /* total = 500, ratios: 0.6, 0.4 */

    fc_status_t status = fc_ex_sig_feature_extract(features, bid_p, bid_q, ask_p, ask_q, 1, n_levels);

    FC_TEST_ASSERT_EQ(status, FC_OK);

    /* Level 0 depth ratios: features[13] = 0.6, features[14] = 0.6 */
    FC_TEST_ASSERT_DOUBLE_EQ(features[13], 0.6, 1e-10);
    FC_TEST_ASSERT_DOUBLE_EQ(features[14], 0.6, 1e-10);

    /* Level 1 depth ratios: features[21] = 0.4, features[22] = 0.4 */
    FC_TEST_ASSERT_DOUBLE_EQ(features[21], 0.4, 1e-10);
    FC_TEST_ASSERT_DOUBLE_EQ(features[22], 0.4, 1e-10);
}

TEST(test_feature_batch_sizes) {
    size_t sizes[] = {1, 4, 8, 16, 100};
    size_t n_sizes = sizeof(sizes) / sizeof(sizes[0]);

    for (size_t s = 0; s < n_sizes; s++) {
        size_t n = sizes[s];
        const int n_levels = 5;
        const size_t n_features = fc_ex_sig_feature_count(n_levels);

        double* features = fc_aligned_alloc(n * n_features * sizeof(double), 64);
        double* bid_p = fc_aligned_alloc(n * n_levels * sizeof(double), 64);
        double* bid_q = fc_aligned_alloc(n * n_levels * sizeof(double), 64);
        double* ask_p = fc_aligned_alloc(n * n_levels * sizeof(double), 64);
        double* ask_q = fc_aligned_alloc(n * n_levels * sizeof(double), 64);

        for (size_t i = 0; i < n; i++) {
            for (int k = 0; k < n_levels; k++) {
                size_t idx = k * n + i;
                bid_p[idx] = 100.0 + (double)i * 0.1 - (double)k * 0.1;
                bid_q[idx] = 500.0 + (double)i - (double)k * 100.0;
                ask_p[idx] = bid_p[idx] + 0.5 + (double)k * 0.1;
                ask_q[idx] = 500.0 + (double)(n - i) - (double)k * 100.0;
            }
        }

        fc_status_t status = fc_ex_sig_feature_extract(features, bid_p, bid_q, ask_p, ask_q, n, n_levels);

        FC_TEST_ASSERT_EQ(status, FC_OK);

        /* Verify all symbols have valid core features */
        for (size_t i = 0; i < n; i++) {
            double* feat = features + i * n_features;
            ASSERT_FALSE(isnan(feat[0]));  /* best_bid_p */
            ASSERT_FALSE(isnan(feat[1]));  /* best_ask_p */
            ASSERT_FALSE(isnan(feat[4]));  /* mid_price */
            ASSERT_FALSE(isnan(feat[6]));  /* spread_abs */
            ASSERT_TRUE(feat[6] >= 0.0);   /* spread should be non-negative */
        }

        fc_aligned_free(features);
        fc_aligned_free(bid_p);
        fc_aligned_free(bid_q);
        fc_aligned_free(ask_p);
        fc_aligned_free(ask_q);
    }
}

void register_feature_tests(void) {
    RUN_TEST(test_feature_count);
    RUN_TEST(test_feature_basic_single_symbol);
    RUN_TEST(test_feature_multi_symbol);
    RUN_TEST(test_feature_edge_cases);
    RUN_TEST(test_feature_null_pointers);
    RUN_TEST(test_feature_invalid_size);
    RUN_TEST(test_feature_price_gaps);
    RUN_TEST(test_feature_core_only);
    RUN_TEST(test_feature_depth_ratios);
    RUN_TEST(test_feature_batch_sizes);
}
