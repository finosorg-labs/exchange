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

static void test_ticker_create_destroy(void) {
    int64_t periods[] = {60000000000LL, 300000000000LL};
    fc_ticker_ctx_t *ctx =
        fc_ticker_create(100, 2, periods, FC_TICKER_PRECISION_KAHAN);
    FC_TEST_ASSERT_MSG(ctx != NULL, "Failed to create ticker context");

    uint32_t num_symbols = 0, num_periods = 0;
    int result = fc_ticker_get_stats(ctx, &num_symbols, &num_periods);
    FC_TEST_ASSERT_MSG(result == 0, "Failed to get stats");
    FC_TEST_ASSERT_MSG(num_symbols == 100, "Incorrect num_symbols");
    FC_TEST_ASSERT_MSG(num_periods == 2, "Incorrect num_periods");

    fc_ticker_destroy(ctx);
}

static void test_ticker_create_invalid_args(void) {
    int64_t periods[] = {60000000000LL};

    fc_ticker_ctx_t *ctx = fc_ticker_create(0, 1, periods, FC_TICKER_PRECISION_KAHAN);
    FC_TEST_ASSERT_MSG(ctx == NULL, "Should fail with num_symbols=0");

    ctx = fc_ticker_create(100, 0, periods, FC_TICKER_PRECISION_KAHAN);
    FC_TEST_ASSERT_MSG(ctx == NULL, "Should fail with num_periods=0");

    ctx = fc_ticker_create(100, 1, NULL, FC_TICKER_PRECISION_KAHAN);
    FC_TEST_ASSERT_MSG(ctx == NULL, "Should fail with NULL periods");

    ctx = fc_ticker_create(100, 1, periods, FC_TICKER_PRECISION_BIGFLOAT);
    FC_TEST_ASSERT_MSG(ctx == NULL, "Should fail with unsupported precision mode");
}

static void test_ticker_single_tick_update(void) {
    int64_t periods[] = {60000000000LL};
    fc_ticker_ctx_t *ctx = fc_ticker_create(10, 1, periods, FC_TICKER_PRECISION_KAHAN);
    FC_TEST_ASSERT_MSG(ctx != NULL, "Failed to create ticker context");

    fc_tick_t tick = {
        .symbol_id = 0,
        .price = 100.5,
        .volume = 1000.0,
        .amount = 100500.0,
        .timestamp_ns = 1000000000LL
    };

    int result = fc_ticker_update(ctx, &tick);
    FC_TEST_ASSERT_MSG(result == 0, "Failed to update tick");

    fc_ohlcv_t ohlcv;
    result = fc_ticker_get_ohlcv(ctx, 0, 0, &ohlcv);
    FC_TEST_ASSERT_MSG(result == 0, "Failed to get OHLCV");
    FC_TEST_ASSERT_MSG(ohlcv.initialized == 1, "OHLCV not initialized");
    FC_TEST_ASSERT_DOUBLE_EQ(ohlcv.open, 100.5, EPSILON);
    FC_TEST_ASSERT_DOUBLE_EQ(ohlcv.high, 100.5, EPSILON);
    FC_TEST_ASSERT_DOUBLE_EQ(ohlcv.low, 100.5, EPSILON);
    FC_TEST_ASSERT_DOUBLE_EQ(ohlcv.close, 100.5, EPSILON);
    FC_TEST_ASSERT_DOUBLE_EQ(ohlcv.volume, 1000.0, EPSILON);
    FC_TEST_ASSERT_DOUBLE_EQ(ohlcv.amount, 100500.0, EPSILON);
    FC_TEST_ASSERT_MSG(ohlcv.tick_count == 1, "Incorrect tick_count");

    fc_ticker_destroy(ctx);
}

static void test_ticker_multiple_ticks_same_period(void) {
    int64_t periods[] = {60000000000LL};
    fc_ticker_ctx_t *ctx = fc_ticker_create(10, 1, periods, FC_TICKER_PRECISION_KAHAN);
    FC_TEST_ASSERT_MSG(ctx != NULL, "Failed to create ticker context");

    fc_tick_t ticks[] = {
        {0, 100.0, 100.0, 10000.0, 1000000000LL},
        {0, 105.0, 200.0, 21000.0, 2000000000LL},
        {0, 98.0, 150.0, 14700.0, 3000000000LL},
        {0, 102.0, 300.0, 30600.0, 4000000000LL}
    };

    for (int i = 0; i < 4; i++) {
        int result = fc_ticker_update(ctx, &ticks[i]);
        FC_TEST_ASSERT_MSG(result == 0, "Failed to update tick");
    }

    fc_ohlcv_t ohlcv;
    int result = fc_ticker_get_ohlcv(ctx, 0, 0, &ohlcv);
    FC_TEST_ASSERT_MSG(result == 0, "Failed to get OHLCV");
    FC_TEST_ASSERT_DOUBLE_EQ(ohlcv.open, 100.0, EPSILON);
    FC_TEST_ASSERT_DOUBLE_EQ(ohlcv.high, 105.0, EPSILON);
    FC_TEST_ASSERT_DOUBLE_EQ(ohlcv.low, 98.0, EPSILON);
    FC_TEST_ASSERT_DOUBLE_EQ(ohlcv.close, 102.0, EPSILON);
    FC_TEST_ASSERT_DOUBLE_EQ(ohlcv.volume, 750.0, EPSILON);
    FC_TEST_ASSERT_DOUBLE_EQ(ohlcv.amount, 76300.0, EPSILON);
    FC_TEST_ASSERT_MSG(ohlcv.tick_count == 4, "Incorrect tick_count");

    fc_ticker_destroy(ctx);
}

static void test_ticker_period_rollover(void) {
    int64_t periods[] = {60000000000LL};
    fc_ticker_ctx_t *ctx = fc_ticker_create(10, 1, periods, FC_TICKER_PRECISION_KAHAN);
    FC_TEST_ASSERT_MSG(ctx != NULL, "Failed to create ticker context");

    fc_tick_t tick1 = {0, 100.0, 100.0, 10000.0, 1000000000LL};
    fc_tick_t tick2 = {0, 105.0, 200.0, 21000.0, 61000000000LL};

    int result = fc_ticker_update(ctx, &tick1);
    FC_TEST_ASSERT_MSG(result == 0, "Failed to update tick1");

    fc_ohlcv_t ohlcv;
    result = fc_ticker_get_ohlcv(ctx, 0, 0, &ohlcv);
    FC_TEST_ASSERT_MSG(result == 0, "Failed to get OHLCV");
    FC_TEST_ASSERT_DOUBLE_EQ(ohlcv.open, 100.0, EPSILON);
    FC_TEST_ASSERT_MSG(ohlcv.tick_count == 1, "Incorrect tick_count for period 1");

    result = fc_ticker_update(ctx, &tick2);
    FC_TEST_ASSERT_MSG(result == 0, "Failed to update tick2");

    result = fc_ticker_get_ohlcv(ctx, 0, 0, &ohlcv);
    FC_TEST_ASSERT_MSG(result == 0, "Failed to get OHLCV");
    FC_TEST_ASSERT_DOUBLE_EQ(ohlcv.open, 105.0, EPSILON);
    FC_TEST_ASSERT_DOUBLE_EQ(ohlcv.close, 105.0, EPSILON);
    FC_TEST_ASSERT_MSG(ohlcv.tick_count == 1, "Incorrect tick_count for period 2");

    fc_ticker_destroy(ctx);
}

static void test_ticker_multiple_symbols(void) {
    int64_t periods[] = {60000000000LL};
    fc_ticker_ctx_t *ctx = fc_ticker_create(3, 1, periods, FC_TICKER_PRECISION_KAHAN);
    FC_TEST_ASSERT_MSG(ctx != NULL, "Failed to create ticker context");

    fc_tick_t ticks[] = {
        {0, 100.0, 100.0, 10000.0, 1000000000LL},
        {1, 200.0, 200.0, 40000.0, 1000000000LL},
        {2, 300.0, 300.0, 90000.0, 1000000000LL},
        {0, 105.0, 150.0, 15750.0, 2000000000LL}
    };

    for (int i = 0; i < 4; i++) {
        int result = fc_ticker_update(ctx, &ticks[i]);
        FC_TEST_ASSERT_MSG(result == 0, "Failed to update tick");
    }

    fc_ohlcv_t ohlcv;
    int result = fc_ticker_get_ohlcv(ctx, 0, 0, &ohlcv);
    FC_TEST_ASSERT_MSG(result == 0, "Failed to get OHLCV for symbol 0");
    FC_TEST_ASSERT_DOUBLE_EQ(ohlcv.open, 100.0, EPSILON);
    FC_TEST_ASSERT_DOUBLE_EQ(ohlcv.close, 105.0, EPSILON);
    FC_TEST_ASSERT_MSG(ohlcv.tick_count == 2, "Incorrect tick_count for symbol 0");

    result = fc_ticker_get_ohlcv(ctx, 1, 0, &ohlcv);
    FC_TEST_ASSERT_MSG(result == 0, "Failed to get OHLCV for symbol 1");
    FC_TEST_ASSERT_DOUBLE_EQ(ohlcv.open, 200.0, EPSILON);
    FC_TEST_ASSERT_MSG(ohlcv.tick_count == 1, "Incorrect tick_count for symbol 1");

    result = fc_ticker_get_ohlcv(ctx, 2, 0, &ohlcv);
    FC_TEST_ASSERT_MSG(result == 0, "Failed to get OHLCV for symbol 2");
    FC_TEST_ASSERT_DOUBLE_EQ(ohlcv.open, 300.0, EPSILON);
    FC_TEST_ASSERT_MSG(ohlcv.tick_count == 1, "Incorrect tick_count for symbol 2");

    fc_ticker_destroy(ctx);
}

static void test_ticker_batch_update(void) {
    int64_t periods[] = {60000000000LL};
    fc_ticker_ctx_t *ctx = fc_ticker_create(2, 1, periods, FC_TICKER_PRECISION_KAHAN);
    FC_TEST_ASSERT_MSG(ctx != NULL, "Failed to create ticker context");

    fc_tick_t ticks[] = {
        {0, 100.0, 100.0, 10000.0, 1000000000LL},
        {1, 200.0, 200.0, 40000.0, 1000000000LL},
        {0, 105.0, 150.0, 15750.0, 2000000000LL},
        {1, 205.0, 250.0, 51250.0, 2000000000LL}
    };

    int result = fc_ticker_update_batch(ctx, ticks, 4);
    FC_TEST_ASSERT_MSG(result == 4, "Failed to update batch");

    fc_ohlcv_t ohlcv;
    result = fc_ticker_get_ohlcv(ctx, 0, 0, &ohlcv);
    FC_TEST_ASSERT_MSG(result == 0, "Failed to get OHLCV for symbol 0");
    FC_TEST_ASSERT_MSG(ohlcv.tick_count == 2, "Incorrect tick_count for symbol 0");

    result = fc_ticker_get_ohlcv(ctx, 1, 0, &ohlcv);
    FC_TEST_ASSERT_MSG(result == 0, "Failed to get OHLCV for symbol 1");
    FC_TEST_ASSERT_MSG(ohlcv.tick_count == 2, "Incorrect tick_count for symbol 1");

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
