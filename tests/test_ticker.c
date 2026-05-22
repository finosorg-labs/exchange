/**
 * @file test_ticker.c
 * @brief Unit tests for ticker module
 */

#include "test_framework.h"
#include "ticker.h"
#include "error.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define EPSILON 1e-10

TEST(test_ticker_create_destroy) {
    int64_t periods[] = {60000000000LL, 300000000000LL};
    fc_ticker_ctx_t *ctx =
        fc_ticker_create(100, 2, periods, FC_TICKER_PRECISION_KAHAN);
    ASSERT_NOT_NULL(ctx);

    uint32_t num_symbols = 0, num_periods = 0;
    int result = fc_ticker_get_stats(ctx, &num_symbols, &num_periods);
    ASSERT_EQ(result, 0);
    ASSERT_EQ(num_symbols, 100);
    ASSERT_EQ(num_periods, 2);

    fc_ticker_destroy(ctx);
}

TEST(test_ticker_create_invalid_args) {
    int64_t periods[] = {60000000000LL};

    fc_ticker_ctx_t *ctx = fc_ticker_create(0, 1, periods, FC_TICKER_PRECISION_KAHAN);
    ASSERT_NULL(ctx);

    ctx = fc_ticker_create(100, 0, periods, FC_TICKER_PRECISION_KAHAN);
    ASSERT_NULL(ctx);

    ctx = fc_ticker_create(100, 1, NULL, FC_TICKER_PRECISION_KAHAN);
    ASSERT_NULL(ctx);

    ctx = fc_ticker_create(100, 1, periods, FC_TICKER_PRECISION_BIGFLOAT);
    ASSERT_NULL(ctx);
}

TEST(test_ticker_single_tick_update) {
    int64_t periods[] = {60000000000LL};
    fc_ticker_ctx_t *ctx = fc_ticker_create(10, 1, periods, FC_TICKER_PRECISION_KAHAN);
    ASSERT_NOT_NULL(ctx);

    fc_tick_t tick = {
        .symbol_id = 0,
        .price = 100.5,
        .volume = 1000.0,
        .amount = 100500.0,
        .timestamp_ns = 1000000000LL
    };

    int result = fc_ticker_update(ctx, &tick);
    ASSERT_EQ(result, 0);

    fc_ohlcv_t ohlcv;
    result = fc_ticker_get_ohlcv(ctx, 0, 0, &ohlcv);
    ASSERT_EQ(result, 0);
    ASSERT_EQ(ohlcv.initialized, 1);
    FC_TEST_ASSERT_DOUBLE_EQ(ohlcv.open, 100.5, EPSILON);
    FC_TEST_ASSERT_DOUBLE_EQ(ohlcv.high, 100.5, EPSILON);
    FC_TEST_ASSERT_DOUBLE_EQ(ohlcv.low, 100.5, EPSILON);
    FC_TEST_ASSERT_DOUBLE_EQ(ohlcv.close, 100.5, EPSILON);
    FC_TEST_ASSERT_DOUBLE_EQ(ohlcv.volume, 1000.0, EPSILON);
    FC_TEST_ASSERT_DOUBLE_EQ(ohlcv.amount, 100500.0, EPSILON);
    ASSERT_EQ(ohlcv.tick_count, 1);

    fc_ticker_destroy(ctx);
}

TEST(test_ticker_multiple_ticks_same_period) {
    int64_t periods[] = {60000000000LL};
    fc_ticker_ctx_t *ctx = fc_ticker_create(10, 1, periods, FC_TICKER_PRECISION_KAHAN);
    ASSERT_NOT_NULL(ctx);

    fc_tick_t ticks[] = {
        {0, 100.0, 100.0, 10000.0, 1000000000LL},
        {0, 105.0, 200.0, 21000.0, 2000000000LL},
        {0, 98.0, 150.0, 14700.0, 3000000000LL},
        {0, 102.0, 300.0, 30600.0, 4000000000LL}
    };

    for (int i = 0; i < 4; i++) {
        int result = fc_ticker_update(ctx, &ticks[i]);
        ASSERT_EQ(result, 0);
    }

    fc_ohlcv_t ohlcv;
    int result = fc_ticker_get_ohlcv(ctx, 0, 0, &ohlcv);
    ASSERT_EQ(result, 0);
    FC_TEST_ASSERT_DOUBLE_EQ(ohlcv.open, 100.0, EPSILON);
    FC_TEST_ASSERT_DOUBLE_EQ(ohlcv.high, 105.0, EPSILON);
    FC_TEST_ASSERT_DOUBLE_EQ(ohlcv.low, 98.0, EPSILON);
    FC_TEST_ASSERT_DOUBLE_EQ(ohlcv.close, 102.0, EPSILON);
    FC_TEST_ASSERT_DOUBLE_EQ(ohlcv.volume, 750.0, EPSILON);
    FC_TEST_ASSERT_DOUBLE_EQ(ohlcv.amount, 76300.0, EPSILON);
    ASSERT_EQ(ohlcv.tick_count, 4);

    fc_ticker_destroy(ctx);
}

TEST(test_ticker_period_rollover) {
    int64_t periods[] = {60000000000LL};
    fc_ticker_ctx_t *ctx = fc_ticker_create(10, 1, periods, FC_TICKER_PRECISION_KAHAN);
    ASSERT_NOT_NULL(ctx);

    fc_tick_t tick1 = {0, 100.0, 100.0, 10000.0, 1000000000LL};
    fc_tick_t tick2 = {0, 105.0, 200.0, 21000.0, 61000000000LL};

    int result = fc_ticker_update(ctx, &tick1);
    ASSERT_EQ(result, 0);

    fc_ohlcv_t ohlcv;
    result = fc_ticker_get_ohlcv(ctx, 0, 0, &ohlcv);
    ASSERT_EQ(result, 0);
    FC_TEST_ASSERT_DOUBLE_EQ(ohlcv.open, 100.0, EPSILON);
    ASSERT_EQ(ohlcv.tick_count, 1);

    result = fc_ticker_update(ctx, &tick2);
    ASSERT_EQ(result, 0);

    result = fc_ticker_get_ohlcv(ctx, 0, 0, &ohlcv);
    ASSERT_EQ(result, 0);
    FC_TEST_ASSERT_DOUBLE_EQ(ohlcv.open, 105.0, EPSILON);
    FC_TEST_ASSERT_DOUBLE_EQ(ohlcv.close, 105.0, EPSILON);
    ASSERT_EQ(ohlcv.tick_count, 1);

    fc_ticker_destroy(ctx);
}

TEST(test_ticker_multiple_symbols) {
    int64_t periods[] = {60000000000LL};
    fc_ticker_ctx_t *ctx = fc_ticker_create(3, 1, periods, FC_TICKER_PRECISION_KAHAN);
    ASSERT_NOT_NULL(ctx);

    fc_tick_t ticks[] = {
        {0, 100.0, 100.0, 10000.0, 1000000000LL},
        {1, 200.0, 200.0, 40000.0, 1000000000LL},
        {2, 300.0, 300.0, 90000.0, 1000000000LL},
        {0, 105.0, 150.0, 15750.0, 2000000000LL}
    };

    for (int i = 0; i < 4; i++) {
        int result = fc_ticker_update(ctx, &ticks[i]);
        ASSERT_EQ(result, 0);
    }

    fc_ohlcv_t ohlcv;
    int result = fc_ticker_get_ohlcv(ctx, 0, 0, &ohlcv);
    ASSERT_EQ(result, 0);
    FC_TEST_ASSERT_DOUBLE_EQ(ohlcv.open, 100.0, EPSILON);
    FC_TEST_ASSERT_DOUBLE_EQ(ohlcv.close, 105.0, EPSILON);
    ASSERT_EQ(ohlcv.tick_count, 2);

    result = fc_ticker_get_ohlcv(ctx, 1, 0, &ohlcv);
    ASSERT_EQ(result, 0);
    FC_TEST_ASSERT_DOUBLE_EQ(ohlcv.open, 200.0, EPSILON);
    ASSERT_EQ(ohlcv.tick_count, 1);

    result = fc_ticker_get_ohlcv(ctx, 2, 0, &ohlcv);
    ASSERT_EQ(result, 0);
    FC_TEST_ASSERT_DOUBLE_EQ(ohlcv.open, 300.0, EPSILON);
    ASSERT_EQ(ohlcv.tick_count, 1);

    fc_ticker_destroy(ctx);
}

TEST(test_ticker_batch_update) {
    int64_t periods[] = {60000000000LL};
    fc_ticker_ctx_t *ctx = fc_ticker_create(2, 1, periods, FC_TICKER_PRECISION_KAHAN);
    ASSERT_NOT_NULL(ctx);

    fc_tick_t ticks[] = {
        {0, 100.0, 100.0, 10000.0, 1000000000LL},
        {1, 200.0, 200.0, 40000.0, 1000000000LL},
        {0, 105.0, 150.0, 15750.0, 2000000000LL},
        {1, 205.0, 250.0, 51250.0, 2000000000LL}
    };

    int result = fc_ticker_update_batch(ctx, ticks, 4);
    ASSERT_EQ(result, 0);

    fc_ohlcv_t ohlcv;
    result = fc_ticker_get_ohlcv(ctx, 0, 0, &ohlcv);
    ASSERT_EQ(result, 0);
    ASSERT_EQ(ohlcv.tick_count, 2);

    result = fc_ticker_get_ohlcv(ctx, 1, 0, &ohlcv);
    ASSERT_EQ(result, 0);
    ASSERT_EQ(ohlcv.tick_count, 2);

    fc_ticker_destroy(ctx);
}

TEST(test_ticker_invalid_period_duration) {
    int64_t periods[] = {0LL};
    fc_ticker_ctx_t *ctx = fc_ticker_create(10, 1, periods, FC_TICKER_PRECISION_KAHAN);
    ASSERT_NULL(ctx);

    int64_t negative_periods[] = {-60000000000LL};
    ctx = fc_ticker_create(10, 1, negative_periods, FC_TICKER_PRECISION_KAHAN);
    ASSERT_NULL(ctx);
}

TEST(test_ticker_invalid_tick_data) {
    int64_t periods[] = {60000000000LL};
    fc_ticker_ctx_t *ctx = fc_ticker_create(10, 1, periods, FC_TICKER_PRECISION_KAHAN);
    ASSERT_NOT_NULL(ctx);

    fc_tick_t tick_nan = {0, NAN, 100.0, 10000.0, 1000000000LL};
    int result = fc_ticker_update(ctx, &tick_nan);
    ASSERT_EQ(result, FC_ERR_INVALID_ARG);

    fc_tick_t tick_inf = {0, INFINITY, 100.0, 10000.0, 1000000000LL};
    result = fc_ticker_update(ctx, &tick_inf);
    ASSERT_EQ(result, FC_ERR_INVALID_ARG);

    fc_tick_t tick_negative = {0, -100.0, 100.0, 10000.0, 1000000000LL};
    result = fc_ticker_update(ctx, &tick_negative);
    ASSERT_EQ(result, FC_ERR_INVALID_ARG);

    fc_tick_t tick_negative_volume = {0, 100.0, -100.0, 10000.0, 1000000000LL};
    result = fc_ticker_update(ctx, &tick_negative_volume);
    ASSERT_EQ(result, FC_ERR_INVALID_ARG);

    fc_ticker_destroy(ctx);
}

static fc_test_fn ticker_tests[] = {
    test_ticker_create_destroy,
    test_ticker_create_invalid_args,
    test_ticker_single_tick_update,
    test_ticker_multiple_ticks_same_period,
    test_ticker_period_rollover,
    test_ticker_multiple_symbols,
    test_ticker_batch_update,
    test_ticker_invalid_period_duration,
    test_ticker_invalid_tick_data,
};

static fc_test_suite_t ticker_suite = {
    .name = "ticker",
    .description = "Tick data aggregation tests",
    .tests = ticker_tests,
    .num_tests = sizeof(ticker_tests) / sizeof(ticker_tests[0]),
};

void register_ticker_tests(void) {
    fc_test_register_suite(&ticker_suite);
}
