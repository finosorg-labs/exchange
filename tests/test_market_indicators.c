/**
 * @file test_market_indicators.c
 * @brief Unit tests for realtime market indicators
 */

#include "error.h"
#include "market_indicators.h"
#include "simd_detect.h"
#include "test_framework.h"
#include <float.h>
#include <math.h>

#define EPSILON   1e-10
#define SECOND_NS 1000000000LL

static fc_market_trade_t trade(
    uint32_t symbol,
    double price,
    double volume,
    double buy,
    double sell,
    int64_t ts
) {
    fc_market_trade_t t = {
        .symbol_id    = symbol,
        .price        = price,
        .volume       = volume,
        .buy_volume   = buy,
        .sell_volume  = sell,
        .timestamp_ns = ts,
    };
    return t;
}

TEST(test_market_indicators_create_destroy) {
    fc_market_indicators_ctx_t* ctx =
        fc_market_indicators_create(10, 60 * SECOND_NS, FC_MARKET_INDICATORS_PRECISION_KAHAN);
    FC_TEST_ASSERT_NOT_NULL(ctx);
    fc_market_indicators_destroy(ctx);

    FC_TEST_ASSERT_NULL(
        fc_market_indicators_create(0, 60 * SECOND_NS, FC_MARKET_INDICATORS_PRECISION_KAHAN)
    );
    FC_TEST_ASSERT_NULL(fc_market_indicators_create(10, 0, FC_MARKET_INDICATORS_PRECISION_KAHAN));
    // NOLINTNEXTLINE(clang-analyzer-optin.core.EnumCastOutOfRange)
    FC_TEST_ASSERT_NULL(
        fc_market_indicators_create(10, 60 * SECOND_NS, (fc_market_indicators_precision_mode_t) 99)
    );
}

TEST(test_market_indicators_single_trade) {
    fc_market_indicators_ctx_t* ctx =
        fc_market_indicators_create(2, 60 * SECOND_NS, FC_MARKET_INDICATORS_PRECISION_KAHAN);
    FC_TEST_ASSERT_NOT_NULL(ctx);

    fc_market_trade_t t = trade(0, 100.0, 10.0, 6.0, 4.0, 5 * SECOND_NS);
    ASSERT_EQ(fc_market_indicators_update(ctx, &t), FC_OK);

    fc_market_indicators_t out;
    ASSERT_EQ(fc_market_indicators_get(ctx, 0, &out), FC_OK);
    ASSERT_TRUE(out.initialized);
    FC_TEST_ASSERT_DOUBLE_EQ(out.vwap, 100.0, EPSILON);
    FC_TEST_ASSERT_DOUBLE_EQ(out.twap, 100.0, EPSILON);
    FC_TEST_ASSERT_DOUBLE_EQ(out.volatility, 0.0, EPSILON);
    FC_TEST_ASSERT_DOUBLE_EQ(out.buy_sell_pressure_ratio, 1.5, EPSILON);
    ASSERT_EQ(out.trade_count, 1);
    FC_TEST_ASSERT_DOUBLE_EQ(out.total_volume, 10.0, EPSILON);
    FC_TEST_ASSERT_DOUBLE_EQ(out.total_amount, 1000.0, EPSILON);

    fc_market_indicators_destroy(ctx);
}

TEST(test_market_indicators_multiple_trades) {
    fc_market_indicators_ctx_t* ctx =
        fc_market_indicators_create(1, 60 * SECOND_NS, FC_MARKET_INDICATORS_PRECISION_KAHAN);
    FC_TEST_ASSERT_NOT_NULL(ctx);

    fc_market_trade_t trades[] = {
        trade(0, 100.0, 10.0, 5.0, 5.0, 0),
        trade(0, 110.0, 20.0, 10.0, 10.0, 10 * SECOND_NS),
        trade(0, 121.0, 10.0, 10.0, 0.0, 30 * SECOND_NS),
    };
    ASSERT_EQ(fc_market_indicators_update_batch(ctx, trades, 3), FC_OK);

    fc_market_indicators_t out;
    ASSERT_EQ(fc_market_indicators_get(ctx, 0, &out), FC_OK);
    FC_TEST_ASSERT_DOUBLE_EQ(out.vwap, 110.25, EPSILON);
    FC_TEST_ASSERT_DOUBLE_EQ(out.twap, (100.0 * 10.0 + 110.0 * 20.0) / 30.0, EPSILON);
    FC_TEST_ASSERT_DOUBLE_EQ(out.buy_sell_pressure_ratio, 25.0 / 15.0, EPSILON);

    double r1           = log(110.0 / 100.0);
    double r2           = log(121.0 / 110.0);
    double mean         = (r1 + r2) / 2.0;
    double expected_vol = sqrt(((r1 - mean) * (r1 - mean) + (r2 - mean) * (r2 - mean)) / 2.0);
    FC_TEST_ASSERT_DOUBLE_EQ(out.volatility, expected_vol, 1e-8);

    fc_market_indicators_destroy(ctx);
}

TEST(test_market_indicators_bigfloat_precision) {
    fc_market_indicators_ctx_t* ctx =
        fc_market_indicators_create(1, 60 * SECOND_NS, FC_MARKET_INDICATORS_PRECISION_BIGFLOAT);
    FC_TEST_ASSERT_NOT_NULL(ctx);

    fc_market_trade_t trades[] = {
        trade(0, 100.0, 10.0, 5.0, 5.0, 0),
        trade(0, 110.0, 20.0, 10.0, 10.0, 10 * SECOND_NS),
    };
    ASSERT_EQ(fc_market_indicators_update_batch(ctx, trades, 2), FC_OK);

    fc_market_indicators_t out;
    ASSERT_EQ(fc_market_indicators_get(ctx, 0, &out), FC_OK);
    FC_TEST_ASSERT_DOUBLE_EQ(out.vwap, (100.0 * 10.0 + 110.0 * 20.0) / 30.0, EPSILON);
    FC_TEST_ASSERT_DOUBLE_EQ(out.twap, 100.0, EPSILON);
    FC_TEST_ASSERT_DOUBLE_EQ(out.buy_sell_pressure_ratio, 1.0, EPSILON);

    fc_market_indicators_destroy(ctx);
}

TEST(test_market_indicators_multiple_symbols) {
    fc_market_indicators_ctx_t* ctx =
        fc_market_indicators_create(3, 60 * SECOND_NS, FC_MARKET_INDICATORS_PRECISION_STANDARD);
    FC_TEST_ASSERT_NOT_NULL(ctx);

    fc_market_trade_t trades[] = {
        trade(0, 100.0, 10.0, 10.0, 0.0, SECOND_NS),
        trade(1, 200.0, 5.0, 2.0, 3.0, SECOND_NS),
    };
    ASSERT_EQ(fc_market_indicators_update_batch(ctx, trades, 2), FC_OK);

    fc_market_indicators_t out0, out1, out2;
    ASSERT_EQ(fc_market_indicators_get(ctx, 0, &out0), FC_OK);
    ASSERT_EQ(fc_market_indicators_get(ctx, 1, &out1), FC_OK);
    ASSERT_EQ(fc_market_indicators_get(ctx, 2, &out2), FC_OK);
    FC_TEST_ASSERT_DOUBLE_EQ(out0.vwap, 100.0, EPSILON);
    FC_TEST_ASSERT_DOUBLE_EQ(out1.vwap, 200.0, EPSILON);
    ASSERT_FALSE(out2.initialized);

    fc_market_indicators_destroy(ctx);
}

TEST(test_market_indicators_window_rollover) {
    fc_market_indicators_ctx_t* ctx =
        fc_market_indicators_create(1, 10 * SECOND_NS, FC_MARKET_INDICATORS_PRECISION_KAHAN);
    FC_TEST_ASSERT_NOT_NULL(ctx);

    fc_market_trade_t first = trade(0, 100.0, 10.0, 5.0, 5.0, SECOND_NS);
    fc_market_trade_t next  = trade(0, 200.0, 2.0, 2.0, 0.0, 12 * SECOND_NS);
    ASSERT_EQ(fc_market_indicators_update(ctx, &first), FC_OK);
    ASSERT_EQ(fc_market_indicators_update(ctx, &next), FC_OK);

    fc_market_indicators_t out;
    ASSERT_EQ(fc_market_indicators_get(ctx, 0, &out), FC_OK);
    FC_TEST_ASSERT_DOUBLE_EQ(out.vwap, 200.0, EPSILON);
    ASSERT_EQ(out.trade_count, 1);
    ASSERT_EQ(out.window_start_ns, 10 * SECOND_NS);

    fc_market_indicators_destroy(ctx);
}

TEST(test_market_indicators_validation) {
    fc_market_indicators_ctx_t* ctx =
        fc_market_indicators_create(1, 60 * SECOND_NS, FC_MARKET_INDICATORS_PRECISION_KAHAN);
    FC_TEST_ASSERT_NOT_NULL(ctx);

    fc_market_trade_t valid      = trade(0, 100.0, 10.0, 5.0, 5.0, SECOND_NS);
    fc_market_trade_t bad_symbol = trade(2, 100.0, 10.0, 5.0, 5.0, SECOND_NS);
    fc_market_trade_t nan_price  = trade(0, NAN, 10.0, 5.0, 5.0, SECOND_NS);
    fc_market_trade_t inf_price  = trade(0, INFINITY, 10.0, 5.0, 5.0, SECOND_NS);
    fc_market_trade_t neg_volume = trade(0, 100.0, -1.0, 0.0, 0.0, SECOND_NS);
    fc_market_trade_t zero_price = trade(0, 0.0, 1.0, 0.0, 0.0, SECOND_NS);

    ASSERT_EQ(fc_market_indicators_update(NULL, &valid), FC_ERR_INVALID_ARG);
    ASSERT_EQ(fc_market_indicators_update(ctx, NULL), FC_ERR_INVALID_ARG);
    ASSERT_EQ(fc_market_indicators_update(ctx, &bad_symbol), FC_ERR_INVALID_ARG);
    ASSERT_EQ(fc_market_indicators_update(ctx, &nan_price), FC_ERR_NAN_INPUT);
    ASSERT_EQ(fc_market_indicators_update(ctx, &inf_price), FC_ERR_INVALID_ARG);
    ASSERT_EQ(fc_market_indicators_update(ctx, &neg_volume), FC_ERR_INVALID_ARG);
    ASSERT_EQ(fc_market_indicators_update(ctx, &zero_price), FC_ERR_INVALID_ARG);

    ASSERT_EQ(fc_market_indicators_update(ctx, &valid), FC_OK);
    fc_market_trade_t old = trade(0, 101.0, 1.0, 1.0, 0.0, 0);
    ASSERT_EQ(fc_market_indicators_update(ctx, &old), FC_ERR_INVALID_ARG);

    fc_market_indicators_destroy(ctx);
}

TEST(test_market_indicators_ranking) {
    fc_market_indicators_ctx_t* ctx =
        fc_market_indicators_create(4, 60 * SECOND_NS, FC_MARKET_INDICATORS_PRECISION_KAHAN);
    FC_TEST_ASSERT_NOT_NULL(ctx);

    fc_market_trade_t trades[] = {
        trade(0, 100.0, 10.0, 5.0, 5.0, SECOND_NS),
        trade(1, 200.0, 10.0, 5.0, 5.0, SECOND_NS),
        trade(2, 150.0, 10.0, 5.0, 5.0, SECOND_NS),
    };
    ASSERT_EQ(fc_market_indicators_update_batch(ctx, trades, 3), FC_OK);

    uint32_t symbols[4];
    double values[4];
    size_t count = 0;
    ASSERT_EQ(
        fc_market_indicators_rank(ctx, FC_MARKET_INDICATOR_VWAP, symbols, values, 3, &count, 1),
        FC_OK
    );
    ASSERT_EQ(count, 3);
    ASSERT_EQ(symbols[0], 1);
    ASSERT_EQ(symbols[1], 2);
    ASSERT_EQ(symbols[2], 0);
    FC_TEST_ASSERT_DOUBLE_EQ(values[0], 200.0, EPSILON);

    fc_market_indicators_destroy(ctx);
}

TEST(test_market_indicators_reset) {
    fc_market_indicators_ctx_t* ctx =
        fc_market_indicators_create(2, 60 * SECOND_NS, FC_MARKET_INDICATORS_PRECISION_KAHAN);
    FC_TEST_ASSERT_NOT_NULL(ctx);

    fc_market_trade_t trades[] = {
        trade(0, 100.0, 10.0, 5.0, 5.0, SECOND_NS),
        trade(1, 200.0, 10.0, 5.0, 5.0, SECOND_NS),
    };
    ASSERT_EQ(fc_market_indicators_update_batch(ctx, trades, 2), FC_OK);
    ASSERT_EQ(fc_market_indicators_reset_symbol(ctx, 0), FC_OK);

    fc_market_indicators_t out;
    ASSERT_EQ(fc_market_indicators_get(ctx, 0, &out), FC_OK);
    ASSERT_FALSE(out.initialized);
    ASSERT_EQ(fc_market_indicators_get(ctx, 1, &out), FC_OK);
    ASSERT_TRUE(out.initialized);

    ASSERT_EQ(fc_market_indicators_reset_all(ctx), FC_OK);
    ASSERT_EQ(fc_market_indicators_get(ctx, 1, &out), FC_OK);
    ASSERT_FALSE(out.initialized);

    fc_market_indicators_destroy(ctx);
}

#if FC_ARCH_X86_64
TEST(test_market_indicators_batch_parity_simd_vs_scalar) {
    const uint32_t num_symbols = 64;
    const size_t num_trades    = 10000;

    fc_market_indicators_ctx_t* ctx = fc_market_indicators_create(
        num_symbols, 60 * SECOND_NS, FC_MARKET_INDICATORS_PRECISION_STANDARD
    );
    FC_TEST_ASSERT_NOT_NULL(ctx);

    fc_market_trade_t* trades = (fc_market_trade_t*) malloc(num_trades * sizeof(fc_market_trade_t));
    FC_TEST_ASSERT_NOT_NULL(trades);
    srand(42);
    for (size_t i = 0; i < num_trades; i++) {
        double vol             = 100.0 + (double) (rand() % 10000);
        double buy             = vol * (double) (rand() % 100) / 100.0;
        trades[i].symbol_id    = (uint32_t) (i % num_symbols);
        trades[i].price        = 100.0 + (double) (rand() % 10000) / 100.0;
        trades[i].volume       = vol;
        trades[i].buy_volume   = buy;
        trades[i].sell_volume  = vol - buy;
        trades[i].timestamp_ns = SECOND_NS + (int64_t) i * 1000000LL;
    }

    ASSERT_EQ(fc_market_indicators_update_batch(ctx, trades, num_trades), FC_OK);

    fc_market_indicators_t* simd_results =
        (fc_market_indicators_t*) malloc(num_symbols * sizeof(fc_market_indicators_t));
    FC_TEST_ASSERT_NOT_NULL(simd_results);
    ASSERT_EQ(fc_market_indicators_get_all(ctx, simd_results), FC_OK);

    ASSERT_EQ(fc_market_indicators_reset_all(ctx), FC_OK);

    fc_simd_level_t saved_level = g_fc_simd_level;
    g_fc_simd_level             = FC_SIMD_SCALAR;

    ASSERT_EQ(fc_market_indicators_update_batch(ctx, trades, num_trades), FC_OK);

    g_fc_simd_level = saved_level;

    fc_market_indicators_t* scalar_results =
        (fc_market_indicators_t*) malloc(num_symbols * sizeof(fc_market_indicators_t));
    FC_TEST_ASSERT_NOT_NULL(scalar_results);
    ASSERT_EQ(fc_market_indicators_get_all(ctx, scalar_results), FC_OK);

    for (uint32_t i = 0; i < num_symbols; i++) {
        if (!simd_results[i].initialized && !scalar_results[i].initialized)
            continue;
        ASSERT_EQ(simd_results[i].initialized, scalar_results[i].initialized);
        FC_TEST_ASSERT_DOUBLE_EQ(simd_results[i].vwap, scalar_results[i].vwap, 1e-12);
        FC_TEST_ASSERT_DOUBLE_EQ(simd_results[i].twap, scalar_results[i].twap, 1e-12);
        FC_TEST_ASSERT_DOUBLE_EQ(simd_results[i].volatility, scalar_results[i].volatility, 1e-12);
        FC_TEST_ASSERT_DOUBLE_EQ(
            simd_results[i].buy_sell_pressure_ratio,
            scalar_results[i].buy_sell_pressure_ratio,
            1e-12
        );
        ASSERT_EQ(simd_results[i].trade_count, scalar_results[i].trade_count);
        FC_TEST_ASSERT_DOUBLE_EQ(
            simd_results[i].total_volume, scalar_results[i].total_volume, 1e-12
        );
        FC_TEST_ASSERT_DOUBLE_EQ(
            simd_results[i].total_amount, scalar_results[i].total_amount, 1e-12
        );
    }

    free(scalar_results);
    free(simd_results);
    free(trades);
    fc_market_indicators_destroy(ctx);
}
#endif

TEST(test_market_indicators_batch_nan_mid) {
    const size_t num_trades = 200;
    const size_t fail_idx   = 137;

    fc_market_indicators_ctx_t* ctx =
        fc_market_indicators_create(1, 60 * SECOND_NS, FC_MARKET_INDICATORS_PRECISION_KAHAN);
    FC_TEST_ASSERT_NOT_NULL(ctx);

    fc_market_trade_t* trades = (fc_market_trade_t*) malloc(num_trades * sizeof(fc_market_trade_t));
    FC_TEST_ASSERT_NOT_NULL(trades);
    for (size_t i = 0; i < num_trades; i++) {
        trades[i] =
            trade(0, 100.0 + (double) i, 10.0, 5.0, 5.0, SECOND_NS + (int64_t) i * 1000000LL);
    }
    trades[fail_idx].price = NAN;

    ASSERT_EQ(fc_market_indicators_update_batch(ctx, trades, num_trades), FC_ERR_NAN_INPUT);

    fc_market_indicators_t actual;
    ASSERT_EQ(fc_market_indicators_get(ctx, 0, &actual), FC_OK);

    fc_market_indicators_ctx_t* ref_ctx =
        fc_market_indicators_create(1, 60 * SECOND_NS, FC_MARKET_INDICATORS_PRECISION_KAHAN);
    FC_TEST_ASSERT_NOT_NULL(ref_ctx);
    ASSERT_EQ(fc_market_indicators_update_batch(ref_ctx, trades, fail_idx), FC_OK);

    fc_market_indicators_t ref;
    ASSERT_EQ(fc_market_indicators_get(ref_ctx, 0, &ref), FC_OK);

    ASSERT_EQ(actual.trade_count, fail_idx);
    ASSERT_EQ(actual.trade_count, ref.trade_count);
    FC_TEST_ASSERT_DOUBLE_EQ(actual.vwap, ref.vwap, 1e-12);
    FC_TEST_ASSERT_DOUBLE_EQ(actual.twap, ref.twap, 1e-12);
    FC_TEST_ASSERT_DOUBLE_EQ(actual.total_volume, ref.total_volume, 1e-12);

    free(trades);
    fc_market_indicators_destroy(ref_ctx);
    fc_market_indicators_destroy(ctx);
}

TEST(test_market_indicators_batch_bad_symbol_mid) {
    const size_t num_trades = 500;
    const size_t fail_idx   = 411;

    fc_market_indicators_ctx_t* ctx =
        fc_market_indicators_create(2, 60 * SECOND_NS, FC_MARKET_INDICATORS_PRECISION_KAHAN);
    FC_TEST_ASSERT_NOT_NULL(ctx);

    fc_market_trade_t* trades = (fc_market_trade_t*) malloc(num_trades * sizeof(fc_market_trade_t));
    FC_TEST_ASSERT_NOT_NULL(trades);
    for (size_t i = 0; i < num_trades; i++) {
        trades[i] = trade(
            (uint32_t) (i % 2),
            100.0 + (double) i,
            10.0,
            5.0,
            5.0,
            SECOND_NS + (int64_t) i * 1000000LL
        );
    }
    trades[fail_idx].symbol_id = 99;

    ASSERT_EQ(fc_market_indicators_update_batch(ctx, trades, num_trades), FC_ERR_INVALID_ARG);

    fc_market_indicators_t actual;
    ASSERT_EQ(fc_market_indicators_get(ctx, 0, &actual), FC_OK);
    ASSERT_TRUE(actual.initialized);

    fc_market_indicators_ctx_t* ref_ctx =
        fc_market_indicators_create(2, 60 * SECOND_NS, FC_MARKET_INDICATORS_PRECISION_KAHAN);
    FC_TEST_ASSERT_NOT_NULL(ref_ctx);
    ASSERT_EQ(fc_market_indicators_update_batch(ref_ctx, trades, fail_idx), FC_OK);

    for (uint32_t sym = 0; sym < 2; sym++) {
        fc_market_indicators_t ref;
        ASSERT_EQ(fc_market_indicators_get(ctx, sym, &actual), FC_OK);
        ASSERT_EQ(fc_market_indicators_get(ref_ctx, sym, &ref), FC_OK);
        ASSERT_EQ(actual.trade_count, ref.trade_count);
        FC_TEST_ASSERT_DOUBLE_EQ(actual.vwap, ref.vwap, 1e-12);
    }

    free(trades);
    fc_market_indicators_destroy(ref_ctx);
    fc_market_indicators_destroy(ctx);
}

TEST(test_market_indicators_batch_cross_window) {
    fc_market_indicators_ctx_t* ctx =
        fc_market_indicators_create(1, 10 * SECOND_NS, FC_MARKET_INDICATORS_PRECISION_KAHAN);
    FC_TEST_ASSERT_NOT_NULL(ctx);

    fc_market_trade_t trades[] = {
        trade(0, 100.0, 10.0, 5.0, 5.0, 5 * SECOND_NS),
        trade(0, 110.0, 20.0, 10.0, 10.0, 8 * SECOND_NS),
        trade(0, 200.0, 5.0, 3.0, 2.0, 12 * SECOND_NS),
        trade(0, 210.0, 5.0, 2.0, 3.0, 15 * SECOND_NS),
    };

    ASSERT_EQ(fc_market_indicators_update_batch(ctx, trades, 4), FC_OK);

    fc_market_indicators_t out;
    ASSERT_EQ(fc_market_indicators_get(ctx, 0, &out), FC_OK);
    ASSERT_TRUE(out.initialized);
    ASSERT_EQ(out.trade_count, 2);
    ASSERT_EQ(out.window_start_ns, 10 * SECOND_NS);
    FC_TEST_ASSERT_DOUBLE_EQ(out.total_volume, 10.0, EPSILON);
    FC_TEST_ASSERT_DOUBLE_EQ(out.vwap, (200.0 * 5.0 + 210.0 * 5.0) / 10.0, EPSILON);

    fc_market_indicators_destroy(ctx);
}

TEST(test_market_indicators_bigfloat_batch) {
    const size_t num_trades = 100;

    fc_market_indicators_ctx_t* ctx =
        fc_market_indicators_create(1, 60 * SECOND_NS, FC_MARKET_INDICATORS_PRECISION_BIGFLOAT);
    FC_TEST_ASSERT_NOT_NULL(ctx);

    fc_market_trade_t* trades = (fc_market_trade_t*) malloc(num_trades * sizeof(fc_market_trade_t));
    FC_TEST_ASSERT_NOT_NULL(trades);
    for (size_t i = 0; i < num_trades; i++) {
        trades[i] =
            trade(0, 100.0 + (double) i, 10.0, 5.0, 5.0, SECOND_NS + (int64_t) i * 1000000LL);
    }

    ASSERT_EQ(fc_market_indicators_update_batch(ctx, trades, num_trades), FC_OK);

    fc_market_indicators_t out;
    ASSERT_EQ(fc_market_indicators_get(ctx, 0, &out), FC_OK);
    ASSERT_TRUE(out.initialized);
    ASSERT_EQ(out.trade_count, num_trades);

    fc_market_indicators_ctx_t* ref_ctx =
        fc_market_indicators_create(1, 60 * SECOND_NS, FC_MARKET_INDICATORS_PRECISION_BIGFLOAT);
    FC_TEST_ASSERT_NOT_NULL(ref_ctx);
    for (size_t i = 0; i < num_trades; i++) {
        ASSERT_EQ(fc_market_indicators_update(ref_ctx, &trades[i]), FC_OK);
    }

    fc_market_indicators_t ref;
    ASSERT_EQ(fc_market_indicators_get(ref_ctx, 0, &ref), FC_OK);
    FC_TEST_ASSERT_DOUBLE_EQ(out.vwap, ref.vwap, 1e-12);
    FC_TEST_ASSERT_DOUBLE_EQ(out.twap, ref.twap, 1e-12);
    ASSERT_EQ(out.trade_count, ref.trade_count);

    free(trades);
    fc_market_indicators_destroy(ref_ctx);
    fc_market_indicators_destroy(ctx);
}

TEST(test_market_indicators_batch_edge_sizes) {
    const size_t sizes[]   = {0, 1, 15, 16, 100, 4096, 4097};
    const size_t num_sizes = sizeof(sizes) / sizeof(sizes[0]);

    for (size_t s = 0; s < num_sizes; s++) {
        size_t n = sizes[s];

        fc_market_indicators_ctx_t* ctx =
            fc_market_indicators_create(2, 60 * SECOND_NS, FC_MARKET_INDICATORS_PRECISION_KAHAN);
        FC_TEST_ASSERT_NOT_NULL(ctx);

        fc_market_trade_t* trades = NULL;
        if (n > 0) {
            trades = (fc_market_trade_t*) malloc(n * sizeof(fc_market_trade_t));
            if (!trades) {
                fc_market_indicators_destroy(ctx);
                FC_TEST_ASSERT_NOT_NULL(NULL);
            }
            for (size_t i = 0; i < n; i++) {
                trades[i] = trade(
                    (uint32_t) (i % 2),
                    100.0 + (double) i,
                    10.0,
                    5.0,
                    5.0,
                    SECOND_NS + (int64_t) i * 1000000LL
                );
            }
        }

        fc_status_t status = fc_market_indicators_update_batch(ctx, trades, n);
        if (status != FC_OK) {
            free(trades);
            fc_market_indicators_destroy(ctx);
            ASSERT_EQ(status, FC_OK);
        }

        fc_market_indicators_t out;
        status = fc_market_indicators_get(ctx, 0, &out);
        if (status != FC_OK) {
            free(trades);
            fc_market_indicators_destroy(ctx);
            ASSERT_EQ(status, FC_OK);
        }
        if (n > 0) {
            // NOLINTNEXTLINE(clang-analyzer-unix.Malloc)
            ASSERT_TRUE(out.initialized);
            ASSERT_EQ(out.trade_count, (n + 1) >> 1);
        } else {
            ASSERT_FALSE(out.initialized);
        }

        free(trades);
        fc_market_indicators_destroy(ctx);
    }
}

void register_market_indicators_tests(void) {
    RUN_TEST(test_market_indicators_create_destroy);
    RUN_TEST(test_market_indicators_single_trade);
    RUN_TEST(test_market_indicators_multiple_trades);
    RUN_TEST(test_market_indicators_bigfloat_precision);
    RUN_TEST(test_market_indicators_multiple_symbols);
    RUN_TEST(test_market_indicators_window_rollover);
    RUN_TEST(test_market_indicators_validation);
    RUN_TEST(test_market_indicators_ranking);
    RUN_TEST(test_market_indicators_reset);
#if FC_ARCH_X86_64
    RUN_TEST(test_market_indicators_batch_parity_simd_vs_scalar);
#endif
    RUN_TEST(test_market_indicators_batch_nan_mid);
    RUN_TEST(test_market_indicators_batch_bad_symbol_mid);
    RUN_TEST(test_market_indicators_batch_cross_window);
    RUN_TEST(test_market_indicators_bigfloat_batch);
    RUN_TEST(test_market_indicators_batch_edge_sizes);
}
