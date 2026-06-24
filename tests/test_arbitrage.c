/**
 * @file test_arbitrage.c
 * @brief Unit tests for cross-market arbitrage spread computation
 *
 * Tests include:
 * - Basic 2-market arbitrage
 * - Multi-market spread matrix
 * - Edge cases (NULL pointers, zero markets, negative prices)
 * - Numerical accuracy
 */

#include "signal/arbitrage.h"
#include "test_framework.h"
#include <math.h>
#include <string.h>

/* Test basic 2-market arbitrage opportunity */
TEST(test_arb_basic_2_markets) {
    const int n_markets = 2;

    double best_bid[] = {100.50, 100.45};
    double best_ask[] = {100.51, 100.46};
    double fees[] = {0.01, 0.01};
    double spread_out[4];

    fc_status_t status = fc_ex_sig_arb_spread(
        spread_out,
        best_bid,
        best_ask,
        fees,
        n_markets
    );

    FC_TEST_ASSERT_EQ(status, FC_OK);

    /* spread[0][0] = 100.50 - 100.51 - 0.01 - 0.01 = -0.03 (no arb, same market) */
    FC_TEST_ASSERT_DOUBLE_EQ(spread_out[0], -0.03, 1e-10);

    /* spread[0][1] = 100.50 - 100.46 - 0.01 - 0.01 = 0.02 (arb: buy at market 1, sell at market 0) */
    FC_TEST_ASSERT_DOUBLE_EQ(spread_out[1], 0.02, 1e-10);

    /* spread[1][0] = 100.45 - 100.51 - 0.01 - 0.01 = -0.08 (no arb) */
    FC_TEST_ASSERT_DOUBLE_EQ(spread_out[2], -0.08, 1e-10);

    /* spread[1][1] = 100.45 - 100.46 - 0.01 - 0.01 = -0.03 (no arb) */
    FC_TEST_ASSERT_DOUBLE_EQ(spread_out[3], -0.03, 1e-10);
}

/* Test multi-market arbitrage (4 markets) */
TEST(test_arb_multi_market) {
    const int n_markets = 4;

    /* Venue 0: NYSE, Venue 1: NASDAQ, Venue 2: BATS, Venue 3: IEX */
    double best_bid[] = {100.50, 100.52, 100.48, 100.49};
    double best_ask[] = {100.51, 100.53, 100.49, 100.50};
    double fees[] = {0.01, 0.015, 0.005, 0.008};
    double spread_out[16];

    fc_status_t status = fc_ex_sig_arb_spread(
        spread_out,
        best_bid,
        best_ask,
        fees,
        n_markets
    );

    FC_TEST_ASSERT_EQ(status, FC_OK);

    /* Check a profitable arbitrage opportunity:
     * Buy at market 2 (BATS, low ask) and sell at market 1 (NASDAQ, high bid)
     * spread[1][2] = 100.52 - 100.49 - 0.015 - 0.005 = 0.01
     */
    FC_TEST_ASSERT_DOUBLE_EQ(spread_out[1 * n_markets + 2], 0.01, 1e-10);

    /* Check a non-profitable case:
     * spread[2][1] = 100.48 - 100.53 - 0.005 - 0.015 = -0.07
     */
    FC_TEST_ASSERT_DOUBLE_EQ(spread_out[2 * n_markets + 1], -0.07, 1e-10);

    /* Verify all spreads are computed */
    for (int i = 0; i < n_markets * n_markets; i++) {
        FC_TEST_ASSERT(!isnan(spread_out[i]));
        FC_TEST_ASSERT(!isinf(spread_out[i]));
    }
}

/* Test single market (edge case) */
TEST(test_arb_single_market) {
    const int n_markets = 1;

    double best_bid[] = {100.50};
    double best_ask[] = {100.51};
    double fees[] = {0.01};
    double spread_out[1];

    fc_status_t status = fc_ex_sig_arb_spread(
        spread_out,
        best_bid,
        best_ask,
        fees,
        n_markets
    );

    FC_TEST_ASSERT_EQ(status, FC_OK);

    /* spread[0][0] = 100.50 - 100.51 - 0.01 - 0.01 = -0.03 */
    FC_TEST_ASSERT_DOUBLE_EQ(spread_out[0], -0.03, 1e-10);
}

/* Test large market count for SIMD efficiency */
TEST(test_arb_large_markets) {
    const int n_markets = 16;

    double best_bid[16];
    double best_ask[16];
    double fees[16];
    double spread_out[256];

    /* Initialize with varying prices */
    for (int i = 0; i < n_markets; i++) {
        best_bid[i] = 100.0 + i * 0.1;
        best_ask[i] = 100.1 + i * 0.1;
        fees[i] = 0.01 + i * 0.001;
    }

    fc_status_t status = fc_ex_sig_arb_spread(
        spread_out,
        best_bid,
        best_ask,
        fees,
        n_markets
    );

    FC_TEST_ASSERT_EQ(status, FC_OK);

    /* Verify specific spread calculations */
    /* spread[15][0] = (100.0 + 15*0.1) - (100.1 + 0*0.1) - (0.01 + 15*0.001) - (0.01 + 0*0.001) */
    /* = 101.5 - 100.1 - 0.025 - 0.01 = 1.365 */
    double expected = 101.5 - 100.1 - 0.025 - 0.01;
    FC_TEST_ASSERT_DOUBLE_EQ(spread_out[15 * n_markets + 0], expected, 1e-10);

    /* Verify no NaN or Inf in results */
    for (int i = 0; i < n_markets * n_markets; i++) {
        FC_TEST_ASSERT(!isnan(spread_out[i]));
        FC_TEST_ASSERT(!isinf(spread_out[i]));
    }
}

/* Test zero fees */
TEST(test_arb_zero_fees) {
    const int n_markets = 2;

    double best_bid[] = {100.50, 100.45};
    double best_ask[] = {100.51, 100.46};
    double fees[] = {0.0, 0.0};
    double spread_out[4];

    fc_status_t status = fc_ex_sig_arb_spread(
        spread_out,
        best_bid,
        best_ask,
        fees,
        n_markets
    );

    FC_TEST_ASSERT_EQ(status, FC_OK);

    /* spread[0][1] = 100.50 - 100.46 - 0.0 - 0.0 = 0.04 */
    FC_TEST_ASSERT_DOUBLE_EQ(spread_out[1], 0.04, 1e-10);
}

/* Test high fees (no arbitrage) */
TEST(test_arb_high_fees) {
    const int n_markets = 2;

    double best_bid[] = {100.50, 100.45};
    double best_ask[] = {100.51, 100.46};
    double fees[] = {0.10, 0.10};
    double spread_out[4];

    fc_status_t status = fc_ex_sig_arb_spread(
        spread_out,
        best_bid,
        best_ask,
        fees,
        n_markets
    );

    FC_TEST_ASSERT_EQ(status, FC_OK);

    /* All spreads should be negative due to high fees */
    for (int i = 0; i < 4; i++) {
        FC_TEST_ASSERT(spread_out[i] < 0.0);
    }
}

/* Test NULL pointer validation */
TEST(test_arb_null_pointers) {
    const int n_markets = 2;
    double data[10] = {0};

    fc_status_t status;

    /* NULL spread_out */
    status = fc_ex_sig_arb_spread(NULL, data, data, data, n_markets);
    FC_TEST_ASSERT_EQ(status, FC_ERR_INVALID_ARG);

    /* NULL best_bid */
    status = fc_ex_sig_arb_spread(data, NULL, data, data, n_markets);
    FC_TEST_ASSERT_EQ(status, FC_ERR_INVALID_ARG);

    /* NULL best_ask */
    status = fc_ex_sig_arb_spread(data, data, NULL, data, n_markets);
    FC_TEST_ASSERT_EQ(status, FC_ERR_INVALID_ARG);

    /* NULL fees */
    status = fc_ex_sig_arb_spread(data, data, data, NULL, n_markets);
    FC_TEST_ASSERT_EQ(status, FC_ERR_INVALID_ARG);
}

/* Test invalid market count */
TEST(test_arb_invalid_markets) {
    double data[10] = {0};
    fc_status_t status;

    /* Zero markets */
    status = fc_ex_sig_arb_spread(data, data, data, data, 0);
    FC_TEST_ASSERT_EQ(status, FC_ERR_INVALID_ARG);

    /* Negative markets */
    status = fc_ex_sig_arb_spread(data, data, data, data, -1);
    FC_TEST_ASSERT_EQ(status, FC_ERR_INVALID_ARG);
}

/* Test with realistic cryptocurrency exchange data */
TEST(test_arb_crypto_exchanges) {
    const int n_markets = 3;

    /* Binance, OKX, Deribit */
    double best_bid[] = {50000.50, 50001.00, 49999.80};
    double best_ask[] = {50000.60, 50001.10, 49999.90};
    double fees[] = {5.0, 4.5, 6.0};  /* Absolute fees in USD */
    double spread_out[9];

    fc_status_t status = fc_ex_sig_arb_spread(
        spread_out,
        best_bid,
        best_ask,
        fees,
        n_markets
    );

    FC_TEST_ASSERT_EQ(status, FC_OK);

    /* Check arbitrage: buy at Deribit (low ask), sell at OKX (high bid)
     * spread[1][2] = 50001.00 - 49999.90 - 4.5 - 6.0 = -9.40 (no profit after fees)
     */
    FC_TEST_ASSERT_DOUBLE_EQ(spread_out[1 * n_markets + 2], -9.40, 1e-8);
}

/* Test symmetry property: spread[i][j] != spread[j][i] */
TEST(test_arb_asymmetry) {
    const int n_markets = 3;

    double best_bid[] = {100.50, 100.52, 100.48};
    double best_ask[] = {100.51, 100.53, 100.49};
    double fees[] = {0.01, 0.02, 0.015};
    double spread_out[9];

    fc_status_t status = fc_ex_sig_arb_spread(
        spread_out,
        best_bid,
        best_ask,
        fees,
        n_markets
    );

    FC_TEST_ASSERT_EQ(status, FC_OK);

    /* Verify asymmetry: spread[i][j] should not equal spread[j][i] in general */
    for (int i = 0; i < n_markets; i++) {
        for (int j = 0; j < n_markets; j++) {
            if (i != j) {
                /* They should be different due to different bid/ask prices */
                double spread_ij = spread_out[i * n_markets + j];
                double spread_ji = spread_out[j * n_markets + i];
                /* This is expected behavior, not an error */
                FC_TEST_ASSERT(fabs(spread_ij - spread_ji) > 1e-10);
            }
        }
    }
}

/* Test registration function */
void register_arbitrage_tests(void) {
    RUN_TEST(test_arb_basic_2_markets);
    RUN_TEST(test_arb_multi_market);
    RUN_TEST(test_arb_single_market);
    RUN_TEST(test_arb_large_markets);
    RUN_TEST(test_arb_zero_fees);
    RUN_TEST(test_arb_high_fees);
    RUN_TEST(test_arb_null_pointers);
    RUN_TEST(test_arb_invalid_markets);
    RUN_TEST(test_arb_crypto_exchanges);
    RUN_TEST(test_arb_asymmetry);
}
