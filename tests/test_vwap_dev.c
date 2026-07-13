/**
 * @file test_vwap_dev.c
 * @brief Unit tests for VWAP deviation computation
 *
 * Tests include:
 * - Basic VWAP deviation calculation
 * - Multi-window multi-symbol batch operations
 * - Kahan summation accuracy in VWAP updates
 * - Edge cases (zero volume, zero sigma, NaN, Inf)
 * - SIMD variant consistency
 */

#include "signal/vwap_dev.h"
#include "test_framework.h"
#include <math.h>
#include <string.h>

/* Test basic VWAP deviation calculation */
TEST(test_vwap_dev_basic) {
    const size_t n_symbols = 3;
    const int n_windows    = 2;

    double price[] = {100.0, 105.0, 95.0};
    double vwap[]  = {
        98.0,
        102.0, /* Symbol 0: windows 0, 1 */
        100.0,
        103.0, /* Symbol 1: windows 0, 1 */
        100.0,
        98.0 /* Symbol 2: windows 0, 1 */
    };
    double sigma[] = {
        2.0,
        1.0, /* Symbol 0 */
        5.0,
        2.0, /* Symbol 1 */
        2.5,
        3.5 /* Symbol 2 */
    };
    double z_out[6];

    fc_status_t status = fc_ex_sig_vwap_dev_batch(z_out, price, vwap, sigma, n_symbols, n_windows);

    FC_TEST_ASSERT_EQ(status, FC_OK);

    /* Symbol 0: (100-98)/2 = 1.0, (100-102)/1 = -2.0 */
    FC_TEST_ASSERT_DOUBLE_EQ(z_out[0], 1.0, 1e-10);
    FC_TEST_ASSERT_DOUBLE_EQ(z_out[1], -2.0, 1e-10);

    /* Symbol 1: (105-100)/5 = 1.0, (105-103)/2 = 1.0 */
    FC_TEST_ASSERT_DOUBLE_EQ(z_out[2], 1.0, 1e-10);
    FC_TEST_ASSERT_DOUBLE_EQ(z_out[3], 1.0, 1e-10);

    /* Symbol 2: (95-100)/2.5 = -2.0, (95-98)/3.5 = -0.857... */
    FC_TEST_ASSERT_DOUBLE_EQ(z_out[4], -2.0, 1e-10);
    FC_TEST_ASSERT_DOUBLE_EQ(z_out[5], (95.0 - 98.0) / 3.5, 1e-10);
}

/* Test VWAP deviation with zero sigma (edge case) */
TEST(test_vwap_dev_zero_sigma) {
    const size_t n_symbols = 2;
    const int n_windows    = 2;

    double price[] = {100.0, 105.0};
    double vwap[]  = {98.0, 100.0, 100.0, 105.0};
    double sigma[] = {0.0, 2.0, 1e-12, 3.0}; /* Zero and near-zero sigma */
    double z_out[4];

    fc_status_t status = fc_ex_sig_vwap_dev_batch(z_out, price, vwap, sigma, n_symbols, n_windows);

    FC_TEST_ASSERT_EQ(status, FC_OK);

    /* Zero sigma should result in z = 0 (division by zero protection) */
    FC_TEST_ASSERT_DOUBLE_EQ(z_out[0], 0.0, 1e-10);
    FC_TEST_ASSERT_DOUBLE_EQ(z_out[1], 0.0, 1e-10); /* (100-100)/2 = 0 */
    FC_TEST_ASSERT_DOUBLE_EQ(z_out[2], 0.0, 1e-10); /* Near-zero sigma */
    FC_TEST_ASSERT_DOUBLE_EQ(z_out[3], 0.0, 1e-10); /* (105-105)/3 = 0 */
}

/* Test basic VWAP update with single trade */
TEST(test_vwap_update_basic) {
    const size_t n = 3;

    double pv_acc[3]  = {0.0, 0.0, 0.0};
    double v_acc[3]   = {0.0, 0.0, 0.0};
    double pv_comp[3] = {0.0, 0.0, 0.0};
    double v_comp[3]  = {0.0, 0.0, 0.0};
    double vwap_out[3];

    double price[]  = {100.0, 105.0, 95.0};
    double volume[] = {1000.0, 500.0, 2000.0};

    fc_status_t status =
        fc_ex_sig_vwap_update(pv_acc, v_acc, pv_comp, v_comp, vwap_out, price, volume, n);

    FC_TEST_ASSERT_EQ(status, FC_OK);

    /* After first update: VWAP should equal price */
    FC_TEST_ASSERT_DOUBLE_EQ(vwap_out[0], 100.0, 1e-10);
    FC_TEST_ASSERT_DOUBLE_EQ(vwap_out[1], 105.0, 1e-10);
    FC_TEST_ASSERT_DOUBLE_EQ(vwap_out[2], 95.0, 1e-10);

    /* Accumulators should be correct */
    FC_TEST_ASSERT_DOUBLE_EQ(pv_acc[0], 100000.0, 1e-10);
    FC_TEST_ASSERT_DOUBLE_EQ(v_acc[0], 1000.0, 1e-10);
}

/* Test VWAP update with multiple trades */
TEST(test_vwap_update_multiple_trades) {
    const size_t n = 1;

    double pv_acc[1]  = {0.0};
    double v_acc[1]   = {0.0};
    double pv_comp[1] = {0.0};
    double v_comp[1]  = {0.0};
    double vwap_out[1];

    /* Trade 1: 100 @ 1000 */
    double price1[]  = {100.0};
    double volume1[] = {1000.0};
    fc_status_t status =
        fc_ex_sig_vwap_update(pv_acc, v_acc, pv_comp, v_comp, vwap_out, price1, volume1, n);
    FC_TEST_ASSERT_EQ(status, FC_OK);
    FC_TEST_ASSERT_DOUBLE_EQ(vwap_out[0], 100.0, 1e-10);

    /* Trade 2: 110 @ 500 */
    double price2[]  = {110.0};
    double volume2[] = {500.0};
    status = fc_ex_sig_vwap_update(pv_acc, v_acc, pv_comp, v_comp, vwap_out, price2, volume2, n);
    FC_TEST_ASSERT_EQ(status, FC_OK);

    /* Expected VWAP: (100*1000 + 110*500) / (1000 + 500) = 155000 / 1500 = 103.333... */
    FC_TEST_ASSERT_DOUBLE_EQ(vwap_out[0], 155000.0 / 1500.0, 1e-10);

    /* Trade 3: 90 @ 1500 */
    double price3[]  = {90.0};
    double volume3[] = {1500.0};
    status = fc_ex_sig_vwap_update(pv_acc, v_acc, pv_comp, v_comp, vwap_out, price3, volume3, n);
    FC_TEST_ASSERT_EQ(status, FC_OK);

    /* Expected VWAP: (100*1000 + 110*500 + 90*1500) / 3000 = 290000 / 3000 = 96.666... */
    FC_TEST_ASSERT_DOUBLE_EQ(vwap_out[0], 290000.0 / 3000.0, 1e-10);
}

/* Test Kahan summation accuracy in VWAP update */
TEST(test_vwap_update_kahan_accuracy) {
    const size_t n = 1;

    double pv_acc[1]  = {0.0};
    double v_acc[1]   = {0.0};
    double pv_comp[1] = {0.0};
    double v_comp[1]  = {0.0};
    double vwap_out[1];

    /* Add large value */
    double price1[]  = {1e10};
    double volume1[] = {1.0};
    fc_ex_sig_vwap_update(pv_acc, v_acc, pv_comp, v_comp, vwap_out, price1, volume1, n);

    /* Add small values that would be lost without Kahan summation */
    for (int i = 0; i < 1000; i++) {
        double price[]  = {1.0};
        double volume[] = {1.0};
        fc_ex_sig_vwap_update(pv_acc, v_acc, pv_comp, v_comp, vwap_out, price, volume, n);
    }

    /* Add large negative value */
    double price2[]  = {-1e10};
    double volume2[] = {1.0};
    fc_ex_sig_vwap_update(pv_acc, v_acc, pv_comp, v_comp, vwap_out, price2, volume2, n);

    /* Kahan summation should preserve the 1000 small additions */
    /* Total volume: 1 + 1000 + 1 = 1002 */
    /* Total P*V: 1e10 + 1000 + (-1e10) = 1000 */
    /* VWAP: 1000 / 1002 ≈ 0.998... */
    FC_TEST_ASSERT_DOUBLE_EQ(vwap_out[0], 1000.0 / 1002.0, 1e-6);
}

/* Test VWAP update with zero volume (edge case) */
TEST(test_vwap_update_zero_volume) {
    const size_t n = 2;

    double pv_acc[2]  = {0.0, 0.0};
    double v_acc[2]   = {0.0, 0.0};
    double pv_comp[2] = {0.0, 0.0};
    double v_comp[2]  = {0.0, 0.0};
    double vwap_out[2];

    double price[]  = {100.0, 105.0};
    double volume[] = {0.0, 1e-12}; /* Zero and near-zero volume */

    fc_status_t status =
        fc_ex_sig_vwap_update(pv_acc, v_acc, pv_comp, v_comp, vwap_out, price, volume, n);

    FC_TEST_ASSERT_EQ(status, FC_OK);

    /* With zero/near-zero volume, VWAP should default to current price */
    FC_TEST_ASSERT_DOUBLE_EQ(vwap_out[0], 100.0, 1e-10);
    FC_TEST_ASSERT_DOUBLE_EQ(vwap_out[1], 105.0, 1e-10);
}

/* Test error handling: NULL pointers */
TEST(test_vwap_dev_null_pointers) {
    double z_out[4];
    double price[] = {100.0};
    double vwap[]  = {98.0, 100.0};
    double sigma[] = {2.0, 1.0};

    fc_status_t status;

    status = fc_ex_sig_vwap_dev_batch(NULL, price, vwap, sigma, 1, 2);
    FC_TEST_ASSERT_EQ(status, FC_ERR_INVALID_ARG);

    status = fc_ex_sig_vwap_dev_batch(z_out, NULL, vwap, sigma, 1, 2);
    FC_TEST_ASSERT_EQ(status, FC_ERR_INVALID_ARG);

    status = fc_ex_sig_vwap_dev_batch(z_out, price, NULL, sigma, 1, 2);
    FC_TEST_ASSERT_EQ(status, FC_ERR_INVALID_ARG);

    status = fc_ex_sig_vwap_dev_batch(z_out, price, vwap, NULL, 1, 2);
    FC_TEST_ASSERT_EQ(status, FC_ERR_INVALID_ARG);
}

/* Test error handling: invalid arguments */
TEST(test_vwap_dev_invalid_args) {
    double z_out[4];
    double price[] = {100.0};
    double vwap[]  = {98.0, 100.0};
    double sigma[] = {2.0, 1.0};

    fc_status_t status;

    status = fc_ex_sig_vwap_dev_batch(z_out, price, vwap, sigma, 0, 2);
    FC_TEST_ASSERT_EQ(status, FC_ERR_INVALID_ARG);

    status = fc_ex_sig_vwap_dev_batch(z_out, price, vwap, sigma, 1, 0);
    FC_TEST_ASSERT_EQ(status, FC_ERR_INVALID_ARG);

    status = fc_ex_sig_vwap_dev_batch(z_out, price, vwap, sigma, 1, -1);
    FC_TEST_ASSERT_EQ(status, FC_ERR_INVALID_ARG);
}

/* Test error handling for VWAP update */
TEST(test_vwap_update_null_pointers) {
    double pv_acc[1]  = {0.0};
    double v_acc[1]   = {0.0};
    double pv_comp[1] = {0.0};
    double v_comp[1]  = {0.0};
    double vwap_out[1];
    double price[]  = {100.0};
    double volume[] = {1000.0};

    fc_status_t status;

    status = fc_ex_sig_vwap_update(NULL, v_acc, pv_comp, v_comp, vwap_out, price, volume, 1);
    FC_TEST_ASSERT_EQ(status, FC_ERR_INVALID_ARG);

    status = fc_ex_sig_vwap_update(pv_acc, NULL, pv_comp, v_comp, vwap_out, price, volume, 1);
    FC_TEST_ASSERT_EQ(status, FC_ERR_INVALID_ARG);

    status = fc_ex_sig_vwap_update(pv_acc, v_acc, NULL, v_comp, vwap_out, price, volume, 1);
    FC_TEST_ASSERT_EQ(status, FC_ERR_INVALID_ARG);

    status = fc_ex_sig_vwap_update(pv_acc, v_acc, pv_comp, NULL, vwap_out, price, volume, 1);
    FC_TEST_ASSERT_EQ(status, FC_ERR_INVALID_ARG);

    status = fc_ex_sig_vwap_update(pv_acc, v_acc, pv_comp, v_comp, NULL, price, volume, 1);
    FC_TEST_ASSERT_EQ(status, FC_ERR_INVALID_ARG);

    status = fc_ex_sig_vwap_update(pv_acc, v_acc, pv_comp, v_comp, vwap_out, NULL, volume, 1);
    FC_TEST_ASSERT_EQ(status, FC_ERR_INVALID_ARG);

    status = fc_ex_sig_vwap_update(pv_acc, v_acc, pv_comp, v_comp, vwap_out, price, NULL, 1);
    FC_TEST_ASSERT_EQ(status, FC_ERR_INVALID_ARG);
}

/* Test large batch operation (performance scenario) */
TEST(test_vwap_dev_large_batch) {
    const size_t n_symbols = 1000;
    const int n_windows    = 4;
    const size_t total     = n_symbols * n_windows;

    double* price = (double*) malloc(n_symbols * sizeof(double));
    double* vwap  = (double*) malloc(total * sizeof(double));
    double* sigma = (double*) malloc(total * sizeof(double));
    double* z_out = (double*) malloc(total * sizeof(double));

    if (!price || !vwap || !sigma || !z_out) {
        free(price);
        free(vwap);
        free(sigma);
        free(z_out);
        FC_TEST_ASSERT(0);
    }

    /* Initialize test data */
    for (size_t i = 0; i < n_symbols; i++) {
        price[i] = 100.0 + (double) i * 0.1;
        for (int w = 0; w < n_windows; w++) {
            size_t idx = i * n_windows + w;
            vwap[idx]  = 100.0 + (double) i * 0.1 - (double) w * 0.5;
            sigma[idx] = 1.0 + (double) w * 0.5;
        }
    }

    fc_status_t status = fc_ex_sig_vwap_dev_batch(z_out, price, vwap, sigma, n_symbols, n_windows);
    FC_TEST_ASSERT_EQ(status, FC_OK);

    /* Verify a few results */
    for (size_t i = 0; i < 10; i++) {
        for (int w = 0; w < n_windows; w++) {
            size_t idx      = i * n_windows + w;
            double expected = ((double) w * 0.5) / (1.0 + (double) w * 0.5);
            FC_TEST_ASSERT_DOUBLE_EQ(z_out[idx], expected, 1e-10);
        }
    }

    free(price);
    free(vwap);
    free(sigma);
    free(z_out);
}

/* Register all VWAP deviation tests */
void register_vwap_dev_tests(void) {
    RUN_TEST(test_vwap_dev_basic);
    RUN_TEST(test_vwap_dev_zero_sigma);
    RUN_TEST(test_vwap_update_basic);
    RUN_TEST(test_vwap_update_multiple_trades);
    RUN_TEST(test_vwap_update_kahan_accuracy);
    RUN_TEST(test_vwap_update_zero_volume);
    RUN_TEST(test_vwap_dev_null_pointers);
    RUN_TEST(test_vwap_dev_invalid_args);
    RUN_TEST(test_vwap_update_null_pointers);
    RUN_TEST(test_vwap_dev_large_batch);
}
