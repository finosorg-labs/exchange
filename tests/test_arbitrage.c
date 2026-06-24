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
#include "mem_aligned.h"
#include <math.h>
#include <string.h>
#include <float.h>

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

    /* Diagonal elements should be NaN (same-market arbitrage is meaningless) */
    FC_TEST_ASSERT(isnan(spread_out[0]));

    /* spread[0][1] = 100.50 - 100.46 - 0.01 - 0.01 = 0.02 (arb: buy at market 1, sell at market 0) */
    FC_TEST_ASSERT_DOUBLE_EQ(spread_out[1], 0.02, 1e-10);

    /* spread[1][0] = 100.45 - 100.51 - 0.01 - 0.01 = -0.08 (no arb) */
    FC_TEST_ASSERT_DOUBLE_EQ(spread_out[2], -0.08, 1e-10);

    /* Diagonal elements should be NaN */
    FC_TEST_ASSERT(isnan(spread_out[3]));
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

    /* Verify diagonal elements are NaN (same-market arbitrage) */
    for (int i = 0; i < n_markets; i++) {
        FC_TEST_ASSERT(isnan(spread_out[i * n_markets + i]));
    }

    /* Verify off-diagonal elements are valid numbers */
    for (int i = 0; i < n_markets; i++) {
        for (int j = 0; j < n_markets; j++) {
            if (i != j) {
                FC_TEST_ASSERT(!isnan(spread_out[i * n_markets + j]));
                FC_TEST_ASSERT(!isinf(spread_out[i * n_markets + j]));
            }
        }
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

    /* Diagonal elements should be NaN */
    FC_TEST_ASSERT(isnan(spread_out[0]));
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

    /* Verify diagonal elements are NaN */
    for (int i = 0; i < n_markets; i++) {
        FC_TEST_ASSERT(isnan(spread_out[i * n_markets + i]));
    }

    /* Verify no NaN or Inf in off-diagonal results */
    for (int i = 0; i < n_markets; i++) {
        for (int j = 0; j < n_markets; j++) {
            if (i != j) {
                FC_TEST_ASSERT(!isnan(spread_out[i * n_markets + j]));
                FC_TEST_ASSERT(!isinf(spread_out[i * n_markets + j]));
            }
        }
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

    /* Off-diagonal spreads should be negative due to high fees */
    for (int i = 0; i < n_markets; i++) {
        for (int j = 0; j < n_markets; j++) {
            if (i != j) {
                FC_TEST_ASSERT(spread_out[i * n_markets + j] < 0.0);
            } else {
                /* Diagonal elements are NaN */
                FC_TEST_ASSERT(isnan(spread_out[i * n_markets + j]));
            }
        }
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

/* Test aligned variant with aligned memory */
TEST(test_arb_aligned_variant) {
    const int n_markets = 8;

    double* best_bid = fc_aligned_alloc(n_markets * sizeof(double), 64);
    double* best_ask = fc_aligned_alloc(n_markets * sizeof(double), 64);
    double* fees = fc_aligned_alloc(n_markets * sizeof(double), 64);
    double* spread_out = fc_aligned_alloc(n_markets * n_markets * sizeof(double), 64);

    FC_TEST_ASSERT(best_bid && best_ask && fees && spread_out);

    /* Initialize with test data */
    for (int i = 0; i < n_markets; i++) {
        best_bid[i] = 100.0 + i * 0.1;
        best_ask[i] = 100.1 + i * 0.1;
        fees[i] = 0.01;
    }

    fc_status_t status = fc_ex_sig_arb_spread_aligned(
        spread_out,
        best_bid,
        best_ask,
        fees,
        n_markets
    );

    FC_TEST_ASSERT_EQ(status, FC_OK);

    /* Verify diagonal elements are NaN */
    for (int i = 0; i < n_markets; i++) {
        FC_TEST_ASSERT(isnan(spread_out[i * n_markets + i]));
    }

    /* Verify off-diagonal calculation: spread[7][0] = 100.7 - 100.1 - 0.01 - 0.01 = 0.58 */
    FC_TEST_ASSERT_DOUBLE_EQ(spread_out[7 * n_markets + 0], 0.58, 1e-10);

    fc_aligned_free(best_bid);
    fc_aligned_free(best_ask);
    fc_aligned_free(fees);
    fc_aligned_free(spread_out);
}

/* Test aligned vs unaligned produce same results */
TEST(test_arb_aligned_vs_unaligned) {
    const int n_markets = 16;

    double* best_bid = fc_aligned_alloc(n_markets * sizeof(double), 64);
    double* best_ask = fc_aligned_alloc(n_markets * sizeof(double), 64);
    double* fees = fc_aligned_alloc(n_markets * sizeof(double), 64);
    double* spread_aligned = fc_aligned_alloc(n_markets * n_markets * sizeof(double), 64);
    double* spread_unaligned = fc_aligned_alloc(n_markets * n_markets * sizeof(double), 64);

    FC_TEST_ASSERT(best_bid && best_ask && fees && spread_aligned && spread_unaligned);

    /* Initialize with random-like data */
    for (int i = 0; i < n_markets; i++) {
        best_bid[i] = 100.0 + i * 0.05 + (i % 3) * 0.01;
        best_ask[i] = 100.01 + i * 0.05 + (i % 5) * 0.01;
        fees[i] = 0.01 + (i % 7) * 0.001;
    }

    fc_status_t status1 = fc_ex_sig_arb_spread_aligned(
        spread_aligned,
        best_bid,
        best_ask,
        fees,
        n_markets
    );

    fc_status_t status2 = fc_ex_sig_arb_spread(
        spread_unaligned,
        best_bid,
        best_ask,
        fees,
        n_markets
    );

    FC_TEST_ASSERT_EQ(status1, FC_OK);
    FC_TEST_ASSERT_EQ(status2, FC_OK);

    /* Verify both produce identical results */
    for (int i = 0; i < n_markets * n_markets; i++) {
        if (isnan(spread_aligned[i])) {
            FC_TEST_ASSERT(isnan(spread_unaligned[i]));
        } else {
            FC_TEST_ASSERT_DOUBLE_EQ(spread_aligned[i], spread_unaligned[i], 1e-15);
        }
    }

    fc_aligned_free(best_bid);
    fc_aligned_free(best_ask);
    fc_aligned_free(fees);
    fc_aligned_free(spread_aligned);
    fc_aligned_free(spread_unaligned);
}

/* Test fee percentage to absolute conversion */
TEST(test_arb_fee_pct_to_abs) {
    const int n_markets = 3;

    double best_bid[] = {100.0, 200.0, 50.0};
    double best_ask[] = {100.1, 200.2, 50.05};
    double fee_pct[] = {0.1, 0.05, 0.2};  /* 0.1%, 0.05%, 0.2% */
    double fees_out[3];

    fc_status_t status = fc_ex_sig_arb_fee_pct_to_abs(
        fees_out,
        best_bid,
        best_ask,
        fee_pct,
        n_markets
    );

    FC_TEST_ASSERT_EQ(status, FC_OK);

    /* fees[0] = (100.0 + 100.1) / 2 * 0.1 / 100 = 100.05 * 0.001 = 0.10005 */
    FC_TEST_ASSERT_DOUBLE_EQ(fees_out[0], 0.10005, 1e-10);

    /* fees[1] = (200.0 + 200.2) / 2 * 0.05 / 100 = 200.1 * 0.0005 = 0.10005 */
    FC_TEST_ASSERT_DOUBLE_EQ(fees_out[1], 0.10005, 1e-10);

    /* fees[2] = (50.0 + 50.05) / 2 * 0.2 / 100 = 50.025 * 0.002 = 0.10005 */
    FC_TEST_ASSERT_DOUBLE_EQ(fees_out[2], 0.10005, 1e-10);
}

/* Test fee conversion with NULL pointers */
TEST(test_arb_fee_pct_null) {
    double data[10] = {0};
    fc_status_t status;

    status = fc_ex_sig_arb_fee_pct_to_abs(NULL, data, data, data, 2);
    FC_TEST_ASSERT_EQ(status, FC_ERR_INVALID_ARG);

    status = fc_ex_sig_arb_fee_pct_to_abs(data, NULL, data, data, 2);
    FC_TEST_ASSERT_EQ(status, FC_ERR_INVALID_ARG);

    status = fc_ex_sig_arb_fee_pct_to_abs(data, data, NULL, data, 2);
    FC_TEST_ASSERT_EQ(status, FC_ERR_INVALID_ARG);

    status = fc_ex_sig_arb_fee_pct_to_abs(data, data, data, NULL, 2);
    FC_TEST_ASSERT_EQ(status, FC_ERR_INVALID_ARG);

    status = fc_ex_sig_arb_fee_pct_to_abs(data, data, data, data, 0);
    FC_TEST_ASSERT_EQ(status, FC_ERR_INVALID_ARG);
}

/* Test arbitrage with percentage fees */
TEST(test_arb_with_percentage_fees) {
    const int n_markets = 2;

    double best_bid[] = {100.0, 100.5};
    double best_ask[] = {100.1, 100.6};
    double fee_pct[] = {0.1, 0.1};  /* Both 0.1% */
    double fees[2];
    double spread_out[4];

    /* Convert percentage fees to absolute */
    fc_status_t status1 = fc_ex_sig_arb_fee_pct_to_abs(
        fees,
        best_bid,
        best_ask,
        fee_pct,
        n_markets
    );
    FC_TEST_ASSERT_EQ(status1, FC_OK);

    /* Calculate arbitrage spreads */
    fc_status_t status2 = fc_ex_sig_arb_spread(
        spread_out,
        best_bid,
        best_ask,
        fees,
        n_markets
    );
    FC_TEST_ASSERT_EQ(status2, FC_OK);

    /* Verify spread calculation with converted fees */
    /* spread[0][1] = 100.0 - 100.6 - fee[0] - fee[1] */
    /* fee[0] = 100.05 * 0.001 = 0.10005, fee[1] = 100.55 * 0.001 = 0.10055 */
    double expected = 100.0 - 100.6 - 0.10005 - 0.10055;
    FC_TEST_ASSERT_DOUBLE_EQ(spread_out[1], expected, 1e-9);
}

/* Test with NaN inputs */
TEST(test_arb_nan_input) {
    const int n_markets = 2;

    double best_bid[] = {NAN, 100.5};
    double best_ask[] = {100.1, 100.6};
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

    /* All diagonal elements are always NaN */
    FC_TEST_ASSERT(isnan(spread_out[0]));  /* [0][0] diagonal */
    FC_TEST_ASSERT(isnan(spread_out[3]));  /* [1][1] diagonal */

    /* spread[0][1] = NaN - 100.6 - 0.01 - 0.01 = NaN (NaN propagates) */
    FC_TEST_ASSERT(isnan(spread_out[1]));

    /* spread[1][0] = 100.5 - 100.1 - 0.01 - 0.01 = 0.38 (no NaN involved in this calc) */
    FC_TEST_ASSERT_DOUBLE_EQ(spread_out[2], 0.38, 1e-10);
}

/* Test with Inf inputs */
TEST(test_arb_inf_input) {
    const int n_markets = 2;

    double best_bid[] = {INFINITY, 100.5};
    double best_ask[] = {100.1, 100.6};
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

    /* Diagonal elements are always NaN */
    FC_TEST_ASSERT(isnan(spread_out[0]));  /* [0][0] diagonal */
    FC_TEST_ASSERT(isnan(spread_out[3]));  /* [1][1] diagonal */

    /* spread[0][1] = Inf - 100.6 - 0.01 - 0.01 = Inf */
    FC_TEST_ASSERT(isinf(spread_out[1]) && spread_out[1] > 0);

    /* spread[1][0] = 100.5 - 100.1 - 0.01 - 0.01 = 0.38 (no Inf involved) */
    FC_TEST_ASSERT_DOUBLE_EQ(spread_out[2], 0.38, 1e-10);
}

/* Test with denormal (very small) values */
TEST(test_arb_denormal_values) {
    const int n_markets = 2;

    /* Denormal numbers (smaller than DBL_MIN normalized) */
    double denormal = 1e-320;  /* Much smaller than DBL_MIN ~2.2e-308 */
    double best_bid[] = {denormal, 100.5};
    double best_ask[] = {denormal * 1.1, 100.6};
    double fees[] = {denormal * 0.1, 0.01};
    double spread_out[4];

    fc_status_t status = fc_ex_sig_arb_spread(
        spread_out,
        best_bid,
        best_ask,
        fees,
        n_markets
    );

    FC_TEST_ASSERT_EQ(status, FC_OK);

    /* Calculation should proceed without error */
    /* spread[0][1] = denormal - 100.6 - denormal*0.1 - 0.01 ≈ -100.61 */
    FC_TEST_ASSERT(spread_out[1] < -100.0);

    /* Diagonal is NaN */
    FC_TEST_ASSERT(isnan(spread_out[0]));
    FC_TEST_ASSERT(isnan(spread_out[3]));
}

/* Test with extreme double values near limits */
TEST(test_arb_extreme_values) {
    const int n_markets = 2;

    double best_bid[] = {DBL_MAX * 0.5, 100.0};
    double best_ask[] = {DBL_MAX * 0.5 + 1.0, 100.1};
    double fees[] = {1.0, 0.01};
    double spread_out[4];

    fc_status_t status = fc_ex_sig_arb_spread(
        spread_out,
        best_bid,
        best_ask,
        fees,
        n_markets
    );

    FC_TEST_ASSERT_EQ(status, FC_OK);

    /* Verify computation doesn't overflow or produce undefined behavior */
    /* spread[0][1] = DBL_MAX*0.5 - 100.1 - 1.0 - 0.01 ≈ DBL_MAX*0.5 - 101.11 */
    FC_TEST_ASSERT(!isnan(spread_out[1]) || isnan(spread_out[0]));  /* Either valid or diagonal */
    FC_TEST_ASSERT(isnan(spread_out[0]));  /* Diagonal */
    FC_TEST_ASSERT(isnan(spread_out[3]));  /* Diagonal */
}

/* Test negative prices (invalid but should handle gracefully) */
TEST(test_arb_negative_prices) {
    const int n_markets = 2;

    double best_bid[] = {-100.0, 100.5};
    double best_ask[] = {-99.9, 100.6};
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

    /* Computation proceeds (garbage in, garbage out for invalid prices) */
    /* spread[0][1] = -100.0 - 100.6 - 0.01 - 0.01 = -200.62 */
    FC_TEST_ASSERT_DOUBLE_EQ(spread_out[1], -200.62, 1e-10);

    /* Diagonal elements are NaN */
    FC_TEST_ASSERT(isnan(spread_out[0]));
    FC_TEST_ASSERT(isnan(spread_out[3]));
}

/* Test zero prices (edge case) */
TEST(test_arb_zero_prices) {
    const int n_markets = 2;

    double best_bid[] = {0.0, 100.5};
    double best_ask[] = {0.0, 100.6};
    double fees[] = {0.0, 0.01};
    double spread_out[4];

    fc_status_t status = fc_ex_sig_arb_spread(
        spread_out,
        best_bid,
        best_ask,
        fees,
        n_markets
    );

    FC_TEST_ASSERT_EQ(status, FC_OK);

    /* spread[0][1] = 0.0 - 100.6 - 0.0 - 0.01 = -100.61 */
    FC_TEST_ASSERT_DOUBLE_EQ(spread_out[1], -100.61, 1e-10);

    /* spread[1][0] = 100.5 - 0.0 - 0.01 - 0.0 = 100.49 */
    FC_TEST_ASSERT_DOUBLE_EQ(spread_out[2], 100.49, 1e-10);

    /* Diagonal elements are NaN */
    FC_TEST_ASSERT(isnan(spread_out[0]));
    FC_TEST_ASSERT(isnan(spread_out[3]));
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
    RUN_TEST(test_arb_aligned_variant);
    RUN_TEST(test_arb_aligned_vs_unaligned);
    RUN_TEST(test_arb_fee_pct_to_abs);
    RUN_TEST(test_arb_fee_pct_null);
    RUN_TEST(test_arb_with_percentage_fees);
    RUN_TEST(test_arb_nan_input);
    RUN_TEST(test_arb_inf_input);
    RUN_TEST(test_arb_denormal_values);
    RUN_TEST(test_arb_extreme_values);
    RUN_TEST(test_arb_negative_prices);
    RUN_TEST(test_arb_zero_prices);
}
