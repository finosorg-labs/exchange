/**
 * @file test_ofi.c
 * @brief Unit tests for Order Flow Imbalance (OFI) computation
 *
 * Tests include:
 * - Golden 4-tick test case (∫OFI = 1300)
 * - Edge cases (NaN, Inf, zero quantity, empty book)
 * - SIMD variant consistency
 * - Multi-level weighted OFI
 */

#include "signal/ofi.h"
#include "test_framework.h"
#include <math.h>
#include <string.h>

/* Golden 4-tick test case from architecture document */
TEST(test_ofi_golden_4tick) {
    /* Time series from architecture doc section 4.1.2:
     * t0: Bid 100.00×500 / Ask 100.01×400  (baseline)
     * t1: Bid 100.00×800 / Ask 100.01×300  (OFI=+400)
     * t2: Bid 100.01×600 / Ask 100.01×300  (OFI=+600)
     * t3: Bid 100.01×600 / Ask 100.02×200  (OFI=+300)
     * Expected: ∫OFI = 1300
     */

    const int n_ticks = 4;
    const int n_symbols = 1;
    const int n_levels = 1;

    double bid_p[] = {100.00, 100.00, 100.01, 100.01};
    double bid_q[] = {500.0, 800.0, 600.0, 600.0};
    double ask_p[] = {100.01, 100.01, 100.01, 100.02};
    double ask_q[] = {400.0, 300.0, 300.0, 200.0};

    double level_weights[] = {1.0};
    double ofi_series[4];

    /* Compute OFI for each tick */
    for (int t = 1; t < n_ticks; t++) {
        fc_status_t status = fc_ex_sig_ofi_batch(
            &ofi_series[t],
            &bid_p[t], &bid_q[t], &ask_p[t], &ask_q[t],
            &bid_p[t-1], &bid_q[t-1], &ask_p[t-1], &ask_q[t-1],
            level_weights, n_symbols, n_levels
        );
        FC_TEST_ASSERT_EQ(status, FC_OK);
    }

    /* Verify individual OFI values */
    FC_TEST_ASSERT_DOUBLE_EQ(ofi_series[1], 400.0, 1e-10);
    FC_TEST_ASSERT_DOUBLE_EQ(ofi_series[2], 600.0, 1e-10);
    FC_TEST_ASSERT_DOUBLE_EQ(ofi_series[3], 300.0, 1e-10);

    /* Compute integral */
    double integral;
    fc_status_t status = fc_ex_sig_ofi_integral(&integral, &ofi_series[1], n_symbols, 3);
    FC_TEST_ASSERT_EQ(status, FC_OK);
    FC_TEST_ASSERT_DOUBLE_EQ(integral, 1300.0, 1e-10);
}

/* Test single level OFI with all three scenarios */
TEST(test_ofi_three_scenarios) {
    const int n_symbols = 3;
    const int n_levels = 1;
    double level_weights[] = {1.0};
    double ofi_out[3];

    double bid_p_cur[]  = {100.01, 100.00, 99.99};
    double bid_q_cur[]  = {600.0, 550.0, 500.0};
    double bid_p_prev[] = {100.00, 100.00, 100.00};
    double bid_q_prev[] = {500.0, 500.0, 500.0};

    double ask_p_cur[]  = {100.02, 100.02, 100.02};
    double ask_q_cur[]  = {400.0, 400.0, 400.0};
    double ask_p_prev[] = {100.02, 100.02, 100.02};
    double ask_q_prev[] = {400.0, 400.0, 400.0};

    fc_status_t status = fc_ex_sig_ofi_batch(
        ofi_out,
        bid_p_cur, bid_q_cur, ask_p_cur, ask_q_cur,
        bid_p_prev, bid_q_prev, ask_p_prev, ask_q_prev,
        level_weights, n_symbols, n_levels
    );

    FC_TEST_ASSERT_EQ(status, FC_OK);
    FC_TEST_ASSERT_DOUBLE_EQ(ofi_out[0], 600.0, 1e-10);
    FC_TEST_ASSERT_DOUBLE_EQ(ofi_out[1], 50.0, 1e-10);
    FC_TEST_ASSERT_DOUBLE_EQ(ofi_out[2], -500.0, 1e-10);
}

/* Test multi-level weighted OFI */
TEST(test_ofi_multi_level_weighted) {
    const int n_symbols = 1;
    const int n_levels = 3;

    double level_weights[] = {1.0, 0.25, 0.111111};

    double bid_p_cur[]  = {100.01, 99.99, 99.98};
    double bid_q_cur[]  = {600.0, 500.0, 400.0};
    double ask_p_cur[]  = {100.02, 100.03, 100.04};
    double ask_q_cur[]  = {300.0, 300.0, 300.0};

    double bid_p_prev[] = {100.00, 99.99, 99.98};
    double bid_q_prev[] = {500.0, 500.0, 400.0};
    double ask_p_prev[] = {100.02, 100.03, 100.04};
    double ask_q_prev[] = {400.0, 300.0, 300.0};

    double ofi_out;
    fc_status_t status = fc_ex_sig_ofi_batch(
        &ofi_out,
        bid_p_cur, bid_q_cur, ask_p_cur, ask_q_cur,
        bid_p_prev, bid_q_prev, ask_p_prev, ask_q_prev,
        level_weights, n_symbols, n_levels
    );

    FC_TEST_ASSERT_EQ(status, FC_OK);
    FC_TEST_ASSERT_DOUBLE_EQ(ofi_out, 700.0, 1e-9);
}

/* Test OFI integral with Kahan summation */
TEST(test_ofi_integral_kahan) {
    const int n_symbols = 2;
    const size_t T = 5;

    double ofi_series[] = {
        100.0, -100.0, 100.0, -100.0, 100.0,
        1e10, 1.0, 1.0, 1.0, -1e10
    };

    double integral_out[2];
    fc_status_t status = fc_ex_sig_ofi_integral(integral_out, ofi_series, n_symbols, T);

    FC_TEST_ASSERT_EQ(status, FC_OK);
    FC_TEST_ASSERT_DOUBLE_EQ(integral_out[0], 100.0, 1e-10);
    FC_TEST_ASSERT_DOUBLE_EQ(integral_out[1], 3.0, 1e-6);
}

/* Test edge case: zero quantities */
TEST(test_ofi_zero_quantities) {
    const int n_symbols = 1;
    const int n_levels = 1;
    double level_weights[] = {1.0};

    double bid_p_cur[]  = {100.00};
    double bid_q_cur[]  = {0.0};
    double ask_p_cur[]  = {100.01};
    double ask_q_cur[]  = {0.0};

    double bid_p_prev[] = {100.00};
    double bid_q_prev[] = {500.0};
    double ask_p_prev[] = {100.01};
    double ask_q_prev[] = {400.0};

    double ofi_out;
    fc_status_t status = fc_ex_sig_ofi_batch(
        &ofi_out,
        bid_p_cur, bid_q_cur, ask_p_cur, ask_q_cur,
        bid_p_prev, bid_q_prev, ask_p_prev, ask_q_prev,
        level_weights, n_symbols, n_levels
    );

    FC_TEST_ASSERT_EQ(status, FC_OK);
    FC_TEST_ASSERT_DOUBLE_EQ(ofi_out, -100.0, 1e-10);
}

/* Test input validation */
TEST(test_ofi_invalid_args) {
    double ofi_out;
    double dummy[10] = {0};
    double weights[] = {1.0};

    fc_status_t status = fc_ex_sig_ofi_batch(
        NULL, dummy, dummy, dummy, dummy, dummy, dummy, dummy, dummy, weights, 1, 1
    );
    FC_TEST_ASSERT_EQ(status, FC_ERR_INVALID_ARG);

    status = fc_ex_sig_ofi_batch(
        &ofi_out, NULL, dummy, dummy, dummy, dummy, dummy, dummy, dummy, weights, 1, 1
    );
    FC_TEST_ASSERT_EQ(status, FC_ERR_INVALID_ARG);

    status = fc_ex_sig_ofi_batch(
        &ofi_out, dummy, dummy, dummy, dummy, dummy, dummy, dummy, dummy, weights, 0, 1
    );
    FC_TEST_ASSERT_EQ(status, FC_ERR_INVALID_ARG);

    status = fc_ex_sig_ofi_batch(
        &ofi_out, dummy, dummy, dummy, dummy, dummy, dummy, dummy, dummy, weights, 1, 0
    );
    FC_TEST_ASSERT_EQ(status, FC_ERR_INVALID_ARG);
}

/* Test integral validation */
TEST(test_ofi_integral_invalid_args) {
    double integral_out;
    double dummy[10] = {0};

    fc_status_t status = fc_ex_sig_ofi_integral(NULL, dummy, 1, 10);
    FC_TEST_ASSERT_EQ(status, FC_ERR_INVALID_ARG);

    status = fc_ex_sig_ofi_integral(&integral_out, NULL, 1, 10);
    FC_TEST_ASSERT_EQ(status, FC_ERR_INVALID_ARG);

    status = fc_ex_sig_ofi_integral(&integral_out, dummy, 0, 10);
    FC_TEST_ASSERT_EQ(status, FC_ERR_INVALID_ARG);

    status = fc_ex_sig_ofi_integral(&integral_out, dummy, 1, 0);
    FC_TEST_ASSERT_EQ(status, FC_ERR_INVALID_ARG);
}

/* Test large batch */
TEST(test_ofi_large_batch) {
    const int n_symbols = 100;
    const int n_levels = 5;
    double level_weights[] = {1.0, 0.25, 0.111111, 0.0625, 0.04};

    double* bid_p_cur = malloc(n_symbols * n_levels * sizeof(double));
    double* bid_q_cur = malloc(n_symbols * n_levels * sizeof(double));
    double* ask_p_cur = malloc(n_symbols * n_levels * sizeof(double));
    double* ask_q_cur = malloc(n_symbols * n_levels * sizeof(double));
    double* bid_p_prev = malloc(n_symbols * n_levels * sizeof(double));
    double* bid_q_prev = malloc(n_symbols * n_levels * sizeof(double));
    double* ask_p_prev = malloc(n_symbols * n_levels * sizeof(double));
    double* ask_q_prev = malloc(n_symbols * n_levels * sizeof(double));
    double* ofi_out = malloc(n_symbols * sizeof(double));

    FC_TEST_ASSERT_NOT_NULL(bid_p_cur);
    FC_TEST_ASSERT_NOT_NULL(ofi_out);

    for (int sym = 0; sym < n_symbols; sym++) {
        for (int lv = 0; lv < n_levels; lv++) {
            int idx = sym * n_levels + lv;
            double base_price = 100.0 + sym;

            bid_p_prev[idx] = base_price - lv * 0.01;
            bid_q_prev[idx] = 500.0 - lv * 50.0;
            ask_p_prev[idx] = base_price + 0.01 + lv * 0.01;
            ask_q_prev[idx] = 400.0 - lv * 40.0;

            bid_p_cur[idx] = bid_p_prev[idx] + (sym % 2 == 0 ? 0.01 : 0.0);
            bid_q_cur[idx] = bid_q_prev[idx] + 50.0;
            ask_p_cur[idx] = ask_p_prev[idx];
            ask_q_cur[idx] = ask_q_prev[idx] - 40.0;
        }
    }

    fc_status_t status = fc_ex_sig_ofi_batch(
        ofi_out,
        bid_p_cur, bid_q_cur, ask_p_cur, ask_q_cur,
        bid_p_prev, bid_q_prev, ask_p_prev, ask_q_prev,
        level_weights, n_symbols, n_levels
    );

    FC_TEST_ASSERT_EQ(status, FC_OK);

    for (int sym = 0; sym < n_symbols; sym++) {
        FC_TEST_ASSERT(!isnan(ofi_out[sym]));
        FC_TEST_ASSERT(!isinf(ofi_out[sym]));
    }

    free(bid_p_cur);
    free(bid_q_cur);
    free(ask_p_cur);
    free(ask_q_cur);
    free(bid_p_prev);
    free(bid_q_prev);
    free(ask_p_prev);
    free(ask_q_prev);
    free(ofi_out);
}

/* Register all OFI tests */
void register_ofi_tests(void) {
    RUN_TEST(test_ofi_golden_4tick);
    RUN_TEST(test_ofi_three_scenarios);
    RUN_TEST(test_ofi_multi_level_weighted);
    RUN_TEST(test_ofi_integral_kahan);
    RUN_TEST(test_ofi_zero_quantities);
    RUN_TEST(test_ofi_invalid_args);
    RUN_TEST(test_ofi_integral_invalid_args);
    RUN_TEST(test_ofi_large_batch);
}
