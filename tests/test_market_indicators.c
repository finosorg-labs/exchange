/**
 * @file test_market_indicators.c
 * @brief Unit tests for realtime market indicators
 */

#include "test_framework.h"
#include "market_indicators.h"
#include "error.h"
#include <math.h>
#include <float.h>

#define EPSILON 1e-10
#define SECOND_NS 1000000000LL

static fc_market_trade_t trade(uint32_t symbol, double price, double volume, double buy, double sell, int64_t ts) {
    fc_market_trade_t t = {
        .symbol_id = symbol,
        .price = price,
        .volume = volume,
        .buy_volume = buy,
        .sell_volume = sell,
        .timestamp_ns = ts,
    };
    return t;
}

TEST(test_market_indicators_create_destroy) {
    fc_market_indicators_ctx_t* ctx =
        fc_market_indicators_create(10, 60 * SECOND_NS, FC_MARKET_INDICATORS_PRECISION_KAHAN);
    FC_TEST_ASSERT_NOT_NULL(ctx);
    fc_market_indicators_destroy(ctx);

    FC_TEST_ASSERT_NULL(fc_market_indicators_create(0, 60 * SECOND_NS, FC_MARKET_INDICATORS_PRECISION_KAHAN));
    FC_TEST_ASSERT_NULL(fc_market_indicators_create(10, 0, FC_MARKET_INDICATORS_PRECISION_KAHAN));
    FC_TEST_ASSERT_NULL(fc_market_indicators_create(10, 60 * SECOND_NS, 99));
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

    double r1 = log(110.0 / 100.0);
    double r2 = log(121.0 / 110.0);
    double mean = (r1 + r2) / 2.0;
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
    fc_market_trade_t next = trade(0, 200.0, 2.0, 2.0, 0.0, 12 * SECOND_NS);
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

    fc_market_trade_t valid = trade(0, 100.0, 10.0, 5.0, 5.0, SECOND_NS);
    fc_market_trade_t bad_symbol = trade(2, 100.0, 10.0, 5.0, 5.0, SECOND_NS);
    fc_market_trade_t nan_price = trade(0, NAN, 10.0, 5.0, 5.0, SECOND_NS);
    fc_market_trade_t inf_price = trade(0, INFINITY, 10.0, 5.0, 5.0, SECOND_NS);
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
}
