/**
 * @file test_ticker_merge.c
 * @brief Tests for K-line merging functionality
 */

#include "test_framework.h"
#include "ticker_merge.h"
#include "error.h"
#include <math.h>
#include <stdio.h>

#define EPSILON 1e-9

static int callback_invoked = 0;
static uint32_t callback_symbol_id = 0;
static uint32_t callback_period_idx = 0;
static fc_ohlcv_t callback_ohlcv;

static void test_callback(
    uint32_t symbol_id,
    uint32_t derived_period_idx,
    const fc_ohlcv_t* ohlcv,
    void* user_data
) {
    callback_invoked++;
    callback_symbol_id  = symbol_id;
    callback_period_idx = derived_period_idx;
    callback_ohlcv      = *ohlcv;
    (void) user_data;
}

TEST(test_ticker_merge_create_destroy) {
    int64_t base_period        = 60000000000LL; // 1 min
    int64_t derived_periods[3] = {300000000000LL, 900000000000LL, 3600000000000LL}; // 5min, 15min, 60min

    fc_ticker_merge_ctx_t* ctx = fc_ticker_merge_create(
        10, base_period, derived_periods, 3, FC_TICKER_PRECISION_STANDARD, NULL, NULL
    );

    ASSERT_NOT_NULL(ctx);

    uint32_t num_symbols;
    int64_t base_period_out;
    uint32_t num_derived;

    int ret = fc_ticker_merge_get_stats(ctx, &num_symbols, &base_period_out, &num_derived);
    ASSERT_EQ(ret, FC_OK);
    ASSERT_EQ(num_symbols, 10);
    ASSERT_EQ(base_period_out, base_period);
    ASSERT_EQ(num_derived, 3);

    fc_ticker_merge_destroy(ctx);
}

TEST(test_ticker_merge_invalid_args) {
    int64_t base_period        = 60000000000LL;
    int64_t derived_periods[1] = {300000000000LL};

    fc_ticker_merge_ctx_t* ctx = fc_ticker_merge_create(0, base_period, derived_periods, 1, FC_TICKER_PRECISION_STANDARD, NULL, NULL);
    ASSERT_NULL(ctx);

    ctx = fc_ticker_merge_create(10, 0, derived_periods, 1, FC_TICKER_PRECISION_STANDARD, NULL, NULL);
    ASSERT_NULL(ctx);

    ctx = fc_ticker_merge_create(10, base_period, NULL, 1, FC_TICKER_PRECISION_STANDARD, NULL, NULL);
    ASSERT_NULL(ctx);

    int64_t bad_derived[1] = {100000000000LL}; // Not a multiple of base_period
    ctx = fc_ticker_merge_create(10, base_period, bad_derived, 1, FC_TICKER_PRECISION_STANDARD, NULL, NULL);
    ASSERT_NULL(ctx);
}

TEST(test_ticker_merge_base_period_only) {
    int64_t base_period = 60000000000LL; // 1 min

    fc_ticker_merge_ctx_t* ctx =
        fc_ticker_merge_create(2, base_period, NULL, 0, FC_TICKER_PRECISION_STANDARD, NULL, NULL);
    ASSERT_NOT_NULL(ctx);

    fc_tick_t tick = {
        .symbol_id    = 0,
        .timestamp_ns = 1000000000000LL,
        .price        = 100.0,
        .volume       = 10.0,
        .amount       = 1000.0,
    };

    int ret = fc_ticker_merge_update(ctx, &tick);
    ASSERT_EQ(ret, FC_OK);

    fc_ohlcv_t ohlcv;
    ret = fc_ticker_merge_get_base_ohlcv(ctx, 0, &ohlcv);
    ASSERT_EQ(ret, FC_OK);
    ASSERT_EQ(ohlcv.initialized, 1);
    FC_TEST_ASSERT_DOUBLE_EQ(ohlcv.open, 100.0, EPSILON);
    FC_TEST_ASSERT_DOUBLE_EQ(ohlcv.close, 100.0, EPSILON);
    FC_TEST_ASSERT_DOUBLE_EQ(ohlcv.volume, 10.0, EPSILON);

    fc_ticker_merge_destroy(ctx);
}

TEST(test_ticker_merge_5min_from_1min) {
    int64_t base_period        = 60000000000LL;  // 1 min
    int64_t derived_periods[1] = {300000000000LL}; // 5 min

    callback_invoked = 0;

    fc_ticker_merge_ctx_t* ctx = fc_ticker_merge_create(
        1, base_period, derived_periods, 1, FC_TICKER_PRECISION_STANDARD, test_callback, NULL
    );
    ASSERT_NOT_NULL(ctx);

    // Send 5 minutes of ticks (one per minute) + one more to complete the 5th minute
    for (int i = 0; i < 6; i++) {
        fc_tick_t tick = {
            .symbol_id    = 0,
            .timestamp_ns = 1000000000000LL + (i * base_period) + 1000000LL,
            .price        = 100.0 + i,
            .volume       = 10.0,
            .amount       = (100.0 + i) * 10.0,
        };

        int ret = fc_ticker_merge_update(ctx, &tick);
        ASSERT_EQ(ret, FC_OK);
    }

    // Callback should be invoked once after 5 base periods
    ASSERT_EQ(callback_invoked, 1);
    ASSERT_EQ(callback_symbol_id, 0);
    ASSERT_EQ(callback_period_idx, 0);

    // Check merged K-line
    FC_TEST_ASSERT_DOUBLE_EQ(callback_ohlcv.open, 100.0, EPSILON);   // First bar's open
    FC_TEST_ASSERT_DOUBLE_EQ(callback_ohlcv.close, 104.0, EPSILON);  // Last bar's close
    FC_TEST_ASSERT_DOUBLE_EQ(callback_ohlcv.high, 104.0, EPSILON);   // Max high
    FC_TEST_ASSERT_DOUBLE_EQ(callback_ohlcv.low, 100.0, EPSILON);    // Min low
    FC_TEST_ASSERT_DOUBLE_EQ(callback_ohlcv.volume, 50.0, EPSILON);  // Sum of volumes
    ASSERT_EQ(callback_ohlcv.tick_count, 5);        // Sum of tick counts

    fc_ticker_merge_destroy(ctx);
}

TEST(test_ticker_merge_multiple_periods) {
    int64_t base_period        = 60000000000LL;                                // 1 min
    int64_t derived_periods[2] = {300000000000LL, 900000000000LL}; // 5min, 15min

    callback_invoked = 0;

    fc_ticker_merge_ctx_t* ctx = fc_ticker_merge_create(
        1, base_period, derived_periods, 2, FC_TICKER_PRECISION_STANDARD, test_callback, NULL
    );
    ASSERT_NOT_NULL(ctx);

    // Send 15 minutes of ticks + one more to complete the 15th minute
    for (int i = 0; i < 16; i++) {
        fc_tick_t tick = {
            .symbol_id    = 0,
            .timestamp_ns = 1000000000000LL + (i * base_period) + 1000000LL,
            .price        = 100.0 + i,
            .volume       = 10.0,
            .amount       = (100.0 + i) * 10.0,
        };

        int ret = fc_ticker_merge_update(ctx, &tick);
        ASSERT_EQ(ret, FC_OK);
    }

    // Callback should be invoked 3 times for 5min (at 5, 10, 15) + 1 time for 15min (at 15)
    // Total: 4 callbacks
    ASSERT_EQ(callback_invoked, 4);

    fc_ticker_merge_destroy(ctx);
}

TEST(test_ticker_merge_multiple_symbols) {
    int64_t base_period        = 60000000000LL;
    int64_t derived_periods[1] = {300000000000LL};

    callback_invoked = 0;

    fc_ticker_merge_ctx_t* ctx = fc_ticker_merge_create(
        3, base_period, derived_periods, 1, FC_TICKER_PRECISION_STANDARD, test_callback, NULL
    );
    ASSERT_NOT_NULL(ctx);

    // Send ticks for 3 symbols, 5 minutes each + one more to complete
    for (uint32_t sym = 0; sym < 3; sym++) {
        for (int i = 0; i < 6; i++) {
            fc_tick_t tick = {
                .symbol_id    = sym,
                .timestamp_ns = 1000000000000LL + (i * base_period) + 1000000LL,
                .price        = 100.0 + sym * 10.0 + i,
                .volume       = 10.0,
                .amount       = (100.0 + sym * 10.0 + i) * 10.0,
            };

            int ret = fc_ticker_merge_update(ctx, &tick);
            ASSERT_EQ(ret, FC_OK);
        }
    }

    // Each symbol should trigger one 5min callback
    ASSERT_EQ(callback_invoked, 3);

    fc_ticker_merge_destroy(ctx);
}

TEST(test_ticker_merge_reset) {
    int64_t base_period        = 60000000000LL;
    int64_t derived_periods[1] = {300000000000LL};

    fc_ticker_merge_ctx_t* ctx = fc_ticker_merge_create(
        1, base_period, derived_periods, 1, FC_TICKER_PRECISION_STANDARD, NULL, NULL
    );
    ASSERT_NOT_NULL(ctx);

    fc_tick_t tick = {
        .symbol_id    = 0,
        .timestamp_ns = 1000000000000LL,
        .price        = 100.0,
        .volume       = 10.0,
        .amount       = 1000.0,
    };

    int ret = fc_ticker_merge_update(ctx, &tick);
    ASSERT_EQ(ret, FC_OK);

    fc_ohlcv_t ohlcv;
    ret = fc_ticker_merge_get_base_ohlcv(ctx, 0, &ohlcv);
    ASSERT_EQ(ret, FC_OK);
    ASSERT_EQ(ohlcv.initialized, 1);

    // Reset
    ret = fc_ticker_merge_reset(ctx, 0);
    ASSERT_EQ(ret, FC_OK);

    ret = fc_ticker_merge_get_base_ohlcv(ctx, 0, &ohlcv);
    ASSERT_EQ(ret, FC_OK);
    ASSERT_EQ(ohlcv.initialized, 0);

    fc_ticker_merge_destroy(ctx);
}

void register_ticker_merge_tests(void) {
    RUN_TEST(test_ticker_merge_create_destroy);
    RUN_TEST(test_ticker_merge_invalid_args);
    RUN_TEST(test_ticker_merge_base_period_only);
    RUN_TEST(test_ticker_merge_5min_from_1min);
    RUN_TEST(test_ticker_merge_multiple_periods);
    RUN_TEST(test_ticker_merge_multiple_symbols);
    RUN_TEST(test_ticker_merge_reset);
}
