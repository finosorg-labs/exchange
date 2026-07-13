/**
 * @file test_ticker_merge.c
 * @brief Tests for K-line merging functionality
 */

#include "error.h"
#include "test_framework.h"
#include "ticker_merge.h"
#include <math.h>
#include <stdio.h>

#define EPSILON 1e-9

static int callback_invoked         = 0;
static uint32_t callback_symbol_id  = 0;
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
    int64_t derived_periods[3] = {
        300000000000LL, 900000000000LL, 3600000000000LL
    }; // 5min, 15min, 60min

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

    fc_ticker_merge_ctx_t* ctx = fc_ticker_merge_create(
        0, base_period, derived_periods, 1, FC_TICKER_PRECISION_STANDARD, NULL, NULL
    );
    ASSERT_NULL(ctx);

    ctx =
        fc_ticker_merge_create(10, 0, derived_periods, 1, FC_TICKER_PRECISION_STANDARD, NULL, NULL);
    ASSERT_NULL(ctx);

    ctx =
        fc_ticker_merge_create(10, base_period, NULL, 1, FC_TICKER_PRECISION_STANDARD, NULL, NULL);
    ASSERT_NULL(ctx);

    int64_t bad_derived[1] = {100000000000LL}; // Not a multiple of base_period
    ctx                    = fc_ticker_merge_create(
        10, base_period, bad_derived, 1, FC_TICKER_PRECISION_STANDARD, NULL, NULL
    );
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
    int64_t base_period        = 60000000000LL;    // 1 min
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
    FC_TEST_ASSERT_DOUBLE_EQ(callback_ohlcv.open, 100.0, EPSILON);  // First bar's open
    FC_TEST_ASSERT_DOUBLE_EQ(callback_ohlcv.close, 104.0, EPSILON); // Last bar's close
    FC_TEST_ASSERT_DOUBLE_EQ(callback_ohlcv.high, 104.0, EPSILON);  // Max high
    FC_TEST_ASSERT_DOUBLE_EQ(callback_ohlcv.low, 100.0, EPSILON);   // Min low
    FC_TEST_ASSERT_DOUBLE_EQ(callback_ohlcv.volume, 50.0, EPSILON); // Sum of volumes
    ASSERT_EQ(callback_ohlcv.tick_count, 5);                        // Sum of tick counts

    fc_ticker_merge_destroy(ctx);
}

TEST(test_ticker_merge_multiple_periods) {
    int64_t base_period        = 60000000000LL;                    // 1 min
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

TEST(test_ticker_merge_bigfloat_precision) {
    int64_t base_period        = 60000000000LL;
    int64_t derived_periods[1] = {300000000000LL};

    fc_ticker_merge_ctx_t* ctx = fc_ticker_merge_create(
        1, base_period, derived_periods, 1, FC_TICKER_PRECISION_BIGFLOAT, NULL, NULL
    );
    ASSERT_NOT_NULL(ctx);

    // Send ticks to test BigFloat accumulation
    for (int i = 0; i < 6; i++) {
        fc_tick_t tick = {
            .symbol_id    = 0,
            .timestamp_ns = 1000000000000LL + (i * base_period) + 1000000LL,
            .price        = 100.0 + i,
            .volume       = 0.1,
            .amount       = (100.0 + i) * 0.1,
        };

        int ret = fc_ticker_merge_update(ctx, &tick);
        ASSERT_EQ(ret, FC_OK);
    }

    fc_ohlcv_t ohlcv;
    int ret = fc_ticker_merge_get_base_ohlcv(ctx, 0, &ohlcv);
    ASSERT_EQ(ret, FC_OK);
    ASSERT_EQ(ohlcv.initialized, 1);

    fc_ticker_merge_destroy(ctx);
}

TEST(test_ticker_merge_kahan_precision) {
    int64_t base_period = 60000000000LL;

    fc_ticker_merge_ctx_t* ctx =
        fc_ticker_merge_create(1, base_period, NULL, 0, FC_TICKER_PRECISION_KAHAN, NULL, NULL);
    ASSERT_NOT_NULL(ctx);

    // Send many small ticks to test Kahan summation
    for (int i = 0; i < 1000; i++) {
        fc_tick_t tick = {
            .symbol_id    = 0,
            .timestamp_ns = 1000000000000LL + (i * 1000000LL),
            .price        = 100.0,
            .volume       = 0.001,
            .amount       = 0.1,
        };

        int ret = fc_ticker_merge_update(ctx, &tick);
        ASSERT_EQ(ret, FC_OK);
    }

    fc_ohlcv_t ohlcv;
    int ret = fc_ticker_merge_get_base_ohlcv(ctx, 0, &ohlcv);
    ASSERT_EQ(ret, FC_OK);
    FC_TEST_ASSERT_DOUBLE_EQ(ohlcv.volume, 1.0, 1e-6);

    fc_ticker_merge_destroy(ctx);
}

TEST(test_ticker_merge_get_derived_ohlcv) {
    int64_t base_period        = 60000000000LL;
    int64_t derived_periods[1] = {300000000000LL};

    callback_invoked = 0;

    fc_ticker_merge_ctx_t* ctx = fc_ticker_merge_create(
        1, base_period, derived_periods, 1, FC_TICKER_PRECISION_STANDARD, test_callback, NULL
    );
    ASSERT_NOT_NULL(ctx);

    // Send 6 ticks to complete one 5-minute period
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

    ASSERT_EQ(callback_invoked, 1);

    // Get the derived OHLCV
    fc_ohlcv_t derived;
    int ret = fc_ticker_merge_get_derived_ohlcv(ctx, 0, 0, &derived);
    ASSERT_EQ(ret, FC_OK);
    FC_TEST_ASSERT_DOUBLE_EQ(derived.open, 100.0, EPSILON);
    FC_TEST_ASSERT_DOUBLE_EQ(derived.close, 104.0, EPSILON);

    fc_ticker_merge_destroy(ctx);
}

TEST(test_ticker_merge_get_derived_history) {
    int64_t base_period        = 60000000000LL;
    int64_t derived_periods[1] = {300000000000LL};

    fc_ticker_merge_ctx_t* ctx = fc_ticker_merge_create(
        1, base_period, derived_periods, 1, FC_TICKER_PRECISION_STANDARD, NULL, NULL
    );
    ASSERT_NOT_NULL(ctx);

    // Send 3 ticks to partially fill the ring buffer (need 5 for merge)
    for (int i = 0; i < 3; i++) {
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

    // Get history - should have 2 base period bars in the ring buffer
    fc_ohlcv_t history[10];
    size_t count = fc_ticker_merge_get_derived_history(ctx, 0, 0, history, 10);
    ASSERT_EQ(count, 2); // Should have 2 base bars waiting for merge

    fc_ticker_merge_destroy(ctx);
}

TEST(test_ticker_merge_reset_all) {
    int64_t base_period        = 60000000000LL;
    int64_t derived_periods[1] = {300000000000LL};

    fc_ticker_merge_ctx_t* ctx = fc_ticker_merge_create(
        3, base_period, derived_periods, 1, FC_TICKER_PRECISION_STANDARD, NULL, NULL
    );
    ASSERT_NOT_NULL(ctx);

    // Send ticks to all symbols
    for (uint32_t sym = 0; sym < 3; sym++) {
        fc_tick_t tick = {
            .symbol_id    = sym,
            .timestamp_ns = 1000000000000LL,
            .price        = 100.0 + sym,
            .volume       = 10.0,
            .amount       = (100.0 + sym) * 10.0,
        };

        int ret = fc_ticker_merge_update(ctx, &tick);
        ASSERT_EQ(ret, FC_OK);
    }

    // Reset all
    int ret = fc_ticker_merge_reset_all(ctx);
    ASSERT_EQ(ret, FC_OK);

    // Verify all symbols are reset
    for (uint32_t sym = 0; sym < 3; sym++) {
        fc_ohlcv_t ohlcv;
        ret = fc_ticker_merge_get_base_ohlcv(ctx, sym, &ohlcv);
        ASSERT_EQ(ret, FC_OK);
        ASSERT_EQ(ohlcv.initialized, 0);
    }

    fc_ticker_merge_destroy(ctx);
}

TEST(test_ticker_merge_invalid_tick_data) {
    int64_t base_period = 60000000000LL;

    fc_ticker_merge_ctx_t* ctx =
        fc_ticker_merge_create(1, base_period, NULL, 0, FC_TICKER_PRECISION_STANDARD, NULL, NULL);
    ASSERT_NOT_NULL(ctx);

    // Test NaN price
    fc_tick_t tick = {
        .symbol_id    = 0,
        .timestamp_ns = 1000000000000LL,
        .price        = NAN,
        .volume       = 10.0,
        .amount       = 1000.0,
    };
    int ret = fc_ticker_merge_update(ctx, &tick);
    ASSERT_EQ(ret, FC_ERR_INVALID_ARG);

    // Test negative price
    tick.price  = -100.0;
    tick.volume = 10.0;
    ret         = fc_ticker_merge_update(ctx, &tick);
    ASSERT_EQ(ret, FC_ERR_INVALID_ARG);

    // Test infinite volume
    tick.price  = 100.0;
    tick.volume = INFINITY;
    ret         = fc_ticker_merge_update(ctx, &tick);
    ASSERT_EQ(ret, FC_ERR_INVALID_ARG);

    fc_ticker_merge_destroy(ctx);
}

TEST(test_ticker_merge_batch_update) {
    int64_t base_period        = 60000000000LL;
    int64_t derived_periods[1] = {300000000000LL};

    fc_ticker_merge_ctx_t* ctx = fc_ticker_merge_create(
        1, base_period, derived_periods, 1, FC_TICKER_PRECISION_STANDARD, NULL, NULL
    );
    ASSERT_NOT_NULL(ctx);

    // Create batch of ticks
    fc_tick_t ticks[10];
    for (int i = 0; i < 10; i++) {
        ticks[i].symbol_id    = 0;
        ticks[i].timestamp_ns = 1000000000000LL + (i * 1000000LL);
        ticks[i].price        = 100.0 + i;
        ticks[i].volume       = 10.0;
        ticks[i].amount       = (100.0 + i) * 10.0;
    }

    int ret = fc_ticker_merge_update_batch(ctx, ticks, 10);
    ASSERT_EQ(ret, FC_OK);

    fc_ohlcv_t ohlcv;
    ret = fc_ticker_merge_get_base_ohlcv(ctx, 0, &ohlcv);
    ASSERT_EQ(ret, FC_OK);
    ASSERT_EQ(ohlcv.initialized, 1);

    fc_ticker_merge_destroy(ctx);
}

TEST(test_ticker_merge_get_stats_null_outputs) {
    int64_t base_period = 60000000000LL;

    fc_ticker_merge_ctx_t* ctx =
        fc_ticker_merge_create(5, base_period, NULL, 0, FC_TICKER_PRECISION_STANDARD, NULL, NULL);
    ASSERT_NOT_NULL(ctx);

    // Test with NULL outputs (should not crash)
    int ret = fc_ticker_merge_get_stats(ctx, NULL, NULL, NULL);
    ASSERT_EQ(ret, FC_OK);

    fc_ticker_merge_destroy(ctx);
}

TEST(test_ticker_merge_error_handling) {
    int64_t base_period = 60000000000LL;

    fc_ticker_merge_ctx_t* ctx =
        fc_ticker_merge_create(1, base_period, NULL, 0, FC_TICKER_PRECISION_STANDARD, NULL, NULL);
    ASSERT_NOT_NULL(ctx);

    fc_ohlcv_t ohlcv;

    // Test get_base_ohlcv with invalid symbol
    int ret = fc_ticker_merge_get_base_ohlcv(ctx, 999, &ohlcv);
    ASSERT_EQ(ret, FC_ERR_INVALID_ARG);

    // Test get_base_ohlcv with NULL output
    ret = fc_ticker_merge_get_base_ohlcv(ctx, 0, NULL);
    ASSERT_EQ(ret, FC_ERR_INVALID_ARG);

    // Test reset with invalid symbol
    ret = fc_ticker_merge_reset(ctx, 999);
    ASSERT_EQ(ret, FC_ERR_INVALID_ARG);

    // Test update with NULL tick
    ret = fc_ticker_merge_update(ctx, NULL);
    ASSERT_EQ(ret, FC_ERR_INVALID_ARG);

    // Test batch update with NULL ticks
    ret = fc_ticker_merge_update_batch(ctx, NULL, 10);
    ASSERT_EQ(ret, FC_ERR_INVALID_ARG);

    fc_ticker_merge_destroy(ctx);
}

TEST(test_ticker_merge_derived_period_error_handling) {
    int64_t base_period        = 60000000000LL;
    int64_t derived_periods[1] = {300000000000LL};

    fc_ticker_merge_ctx_t* ctx = fc_ticker_merge_create(
        1, base_period, derived_periods, 1, FC_TICKER_PRECISION_STANDARD, NULL, NULL
    );
    ASSERT_NOT_NULL(ctx);

    fc_ohlcv_t ohlcv;

    // Test get_derived_ohlcv with invalid symbol
    int ret = fc_ticker_merge_get_derived_ohlcv(ctx, 999, 0, &ohlcv);
    ASSERT_EQ(ret, FC_ERR_INVALID_ARG);

    // Test get_derived_ohlcv with invalid period index
    ret = fc_ticker_merge_get_derived_ohlcv(ctx, 0, 999, &ohlcv);
    ASSERT_EQ(ret, FC_ERR_INVALID_ARG);

    // Test get_derived_ohlcv with NULL output
    ret = fc_ticker_merge_get_derived_ohlcv(ctx, 0, 0, NULL);
    ASSERT_EQ(ret, FC_ERR_INVALID_ARG);

    // Test get_derived_history with invalid symbol
    fc_ohlcv_t history[10];
    size_t count = fc_ticker_merge_get_derived_history(ctx, 999, 0, history, 10);
    ASSERT_EQ(count, 0);

    // Test get_derived_history with invalid period index
    count = fc_ticker_merge_get_derived_history(ctx, 0, 999, history, 10);
    ASSERT_EQ(count, 0);

    // Test get_derived_history with NULL output
    count = fc_ticker_merge_get_derived_history(ctx, 0, 0, NULL, 10);
    ASSERT_EQ(count, 0);

    fc_ticker_merge_destroy(ctx);
}

TEST(test_ticker_merge_high_low_tracking) {
    int64_t base_period = 60000000000LL;

    fc_ticker_merge_ctx_t* ctx =
        fc_ticker_merge_create(1, base_period, NULL, 0, FC_TICKER_PRECISION_STANDARD, NULL, NULL);
    ASSERT_NOT_NULL(ctx);

    // Send ticks with varying prices in same period
    double prices[] = {100.0, 105.0, 95.0, 102.0, 98.0};
    for (int i = 0; i < 5; i++) {
        fc_tick_t tick = {
            .symbol_id    = 0,
            .timestamp_ns = 1000000000000LL + (i * 1000000LL),
            .price        = prices[i],
            .volume       = 10.0,
            .amount       = prices[i] * 10.0,
        };

        int ret = fc_ticker_merge_update(ctx, &tick);
        ASSERT_EQ(ret, FC_OK);
    }

    fc_ohlcv_t ohlcv;
    int ret = fc_ticker_merge_get_base_ohlcv(ctx, 0, &ohlcv);
    ASSERT_EQ(ret, FC_OK);
    FC_TEST_ASSERT_DOUBLE_EQ(ohlcv.open, 100.0, EPSILON);
    FC_TEST_ASSERT_DOUBLE_EQ(ohlcv.high, 105.0, EPSILON);
    FC_TEST_ASSERT_DOUBLE_EQ(ohlcv.low, 95.0, EPSILON);
    FC_TEST_ASSERT_DOUBLE_EQ(ohlcv.close, 98.0, EPSILON);

    fc_ticker_merge_destroy(ctx);
}

TEST(test_ticker_merge_price_fluctuation) {
    // Test case to cover line 79: out->low = bars[i].low
    // Create scenario where later bars have lower prices than first bar
    int64_t derived_periods[]  = {300000000000LL}; // 5 minutes in nanoseconds
    fc_ticker_merge_ctx_t* ctx = fc_ticker_merge_create(
        1, 60000000000LL, derived_periods, 1, FC_TICKER_PRECISION_STANDARD, NULL, NULL
    );
    ASSERT_NE(ctx, NULL);

    // Send 5 ticks with fluctuating prices: high -> low -> medium -> lower -> high
    double prices[] = {100.0, 95.0, 98.0, 90.0, 105.0};

    for (int i = 0; i < 5; i++) {
        fc_tick_t tick = {
            .symbol_id    = 0,
            .timestamp_ns = 1000000000LL + i * 60000000000LL, // 1 minute apart
            .price        = prices[i],
            .volume       = 100.0,
            .amount       = prices[i] * 100.0
        };

        int ret = fc_ticker_merge_update(ctx, &tick);
        ASSERT_EQ(ret, FC_OK);
    }

    // Send 6th tick to complete the 5-minute period
    fc_tick_t tick = {
        .symbol_id    = 0,
        .timestamp_ns = 1000000000LL + 5 * 60000000000LL,
        .price        = 102.0,
        .volume       = 100.0,
        .amount       = 10200.0
    };
    int ret = fc_ticker_merge_update(ctx, &tick);
    ASSERT_EQ(ret, FC_OK);

    // Verify the merged 5-minute bar has correct high/low
    fc_ohlcv_t derived_ohlcv;
    ret = fc_ticker_merge_get_derived_ohlcv(ctx, 0, 0, &derived_ohlcv);
    ASSERT_EQ(ret, FC_OK);

    // High should be 105.0 (from 5th tick)
    FC_TEST_ASSERT_DOUBLE_EQ(derived_ohlcv.high, 105.0, EPSILON);
    // Low should be 90.0 (from 4th tick, not the first tick's 100.0)
    FC_TEST_ASSERT_DOUBLE_EQ(derived_ohlcv.low, 90.0, EPSILON);
    // Open should be 100.0 (first tick)
    FC_TEST_ASSERT_DOUBLE_EQ(derived_ohlcv.open, 100.0, EPSILON);
    // Close should be 105.0 (last tick of the completed period, 5th tick)
    FC_TEST_ASSERT_DOUBLE_EQ(derived_ohlcv.close, 105.0, EPSILON);

    fc_ticker_merge_destroy(ctx);
}

TEST(test_ticker_merge_large_merge_count_256) {
    int64_t base_period       = 1000000000LL;     // 1 second
    int64_t derived_periods[] = {256000000000LL}; // 256 seconds

    fc_ticker_merge_ctx_t* ctx = fc_ticker_merge_create(
        1, base_period, derived_periods, 1, FC_TICKER_PRECISION_STANDARD, NULL, NULL
    );
    ASSERT_NOT_NULL(ctx);

    // Send 256 ticks to complete one derived period
    for (int i = 0; i < 256; i++) {
        fc_tick_t tick = {
            .symbol_id    = 0,
            .timestamp_ns = 1000000000LL + i * base_period,
            .price        = 100.0 + i,
            .volume       = 10.0,
            .amount       = (100.0 + i) * 10.0
        };
        int ret = fc_ticker_merge_update(ctx, &tick);
        ASSERT_EQ(ret, FC_OK);
    }

    // Trigger completion with 257th tick
    fc_tick_t tick = {
        .symbol_id    = 0,
        .timestamp_ns = 1000000000LL + 256 * base_period,
        .price        = 356.0,
        .volume       = 10.0,
        .amount       = 3560.0
    };
    int ret = fc_ticker_merge_update(ctx, &tick);
    ASSERT_EQ(ret, FC_OK);

    fc_ohlcv_t derived_ohlcv;
    ret = fc_ticker_merge_get_derived_ohlcv(ctx, 0, 0, &derived_ohlcv);
    ASSERT_EQ(ret, FC_OK);
    ASSERT_EQ(derived_ohlcv.initialized, 1);
    FC_TEST_ASSERT_DOUBLE_EQ(derived_ohlcv.open, 100.0, EPSILON);
    FC_TEST_ASSERT_DOUBLE_EQ(derived_ohlcv.close, 355.0, EPSILON);
    FC_TEST_ASSERT_DOUBLE_EQ(derived_ohlcv.high, 355.0, EPSILON);
    FC_TEST_ASSERT_DOUBLE_EQ(derived_ohlcv.low, 100.0, EPSILON);
    ASSERT_EQ(derived_ohlcv.tick_count, 256);

    fc_ticker_merge_destroy(ctx);
}

TEST(test_ticker_merge_large_merge_count_1440) {
    int64_t base_period       = 60000000000LL;      // 1 minute
    int64_t derived_periods[] = {86400000000000LL}; // 1 day (1440 minutes)

    fc_ticker_merge_ctx_t* ctx = fc_ticker_merge_create(
        1, base_period, derived_periods, 1, FC_TICKER_PRECISION_KAHAN, NULL, NULL
    );
    ASSERT_NOT_NULL(ctx);

    // Send 1440 ticks to complete one day
    for (int i = 0; i < 1440; i++) {
        fc_tick_t tick = {
            .symbol_id    = 0,
            .timestamp_ns = 1000000000000LL + i * base_period,
            .price        = 100.0 + (i % 100),
            .volume       = 1.0,
            .amount       = 100.0 + (i % 100)
        };
        int ret = fc_ticker_merge_update(ctx, &tick);
        ASSERT_EQ(ret, FC_OK);
    }

    // Trigger completion
    fc_tick_t tick = {
        .symbol_id    = 0,
        .timestamp_ns = 1000000000000LL + 1440 * base_period,
        .price        = 150.0,
        .volume       = 1.0,
        .amount       = 150.0
    };
    int ret = fc_ticker_merge_update(ctx, &tick);
    ASSERT_EQ(ret, FC_OK);

    fc_ohlcv_t derived_ohlcv;
    ret = fc_ticker_merge_get_derived_ohlcv(ctx, 0, 0, &derived_ohlcv);
    ASSERT_EQ(ret, FC_OK);
    ASSERT_EQ(derived_ohlcv.initialized, 1);
    ASSERT_EQ(derived_ohlcv.tick_count, 1440);
    FC_TEST_ASSERT_DOUBLE_EQ(derived_ohlcv.volume, 1440.0, EPSILON);

    fc_ticker_merge_destroy(ctx);
}

TEST(test_ticker_merge_large_merge_count_3600) {
    int64_t base_period       = 1000000000LL;      // 1 second
    int64_t derived_periods[] = {3600000000000LL}; // 1 hour (3600 seconds)

    fc_ticker_merge_ctx_t* ctx = fc_ticker_merge_create(
        2, base_period, derived_periods, 1, FC_TICKER_PRECISION_BIGFLOAT, NULL, NULL
    );
    ASSERT_NOT_NULL(ctx);

    // Send 3600 ticks for symbol 0
    for (int i = 0; i < 3600; i++) {
        fc_tick_t tick = {
            .symbol_id    = 0,
            .timestamp_ns = 1000000000000LL + i * base_period,
            .price        = 100.0,
            .volume       = 0.1,
            .amount       = 10.0
        };
        int ret = fc_ticker_merge_update(ctx, &tick);
        ASSERT_EQ(ret, FC_OK);
    }

    // Trigger completion
    fc_tick_t tick = {
        .symbol_id    = 0,
        .timestamp_ns = 1000000000000LL + 3600 * base_period,
        .price        = 100.0,
        .volume       = 0.1,
        .amount       = 10.0
    };
    int ret = fc_ticker_merge_update(ctx, &tick);
    ASSERT_EQ(ret, FC_OK);

    fc_ohlcv_t derived_ohlcv;
    ret = fc_ticker_merge_get_derived_ohlcv(ctx, 0, 0, &derived_ohlcv);
    ASSERT_EQ(ret, FC_OK);
    ASSERT_EQ(derived_ohlcv.initialized, 1);
    ASSERT_EQ(derived_ohlcv.tick_count, 3600);
    FC_TEST_ASSERT_DOUBLE_EQ(derived_ohlcv.volume, 360.0, 1e-6);

    fc_ticker_merge_destroy(ctx);
}

TEST(test_ticker_merge_all_precision_modes_large_count) {
    int64_t base_period       = 1000000000LL;     // 1 second
    int64_t derived_periods[] = {500000000000LL}; // 500 seconds

    fc_ticker_precision_mode_t modes[] = {
        FC_TICKER_PRECISION_STANDARD, FC_TICKER_PRECISION_KAHAN, FC_TICKER_PRECISION_BIGFLOAT
    };

    for (int mode_idx = 0; mode_idx < 3; mode_idx++) {
        fc_ticker_merge_ctx_t* ctx =
            fc_ticker_merge_create(1, base_period, derived_periods, 1, modes[mode_idx], NULL, NULL);
        ASSERT_NOT_NULL(ctx);

        // Send 500 ticks with small volumes to test precision
        for (int i = 0; i < 500; i++) {
            fc_tick_t tick = {
                .symbol_id    = 0,
                .timestamp_ns = 1000000000000LL + i * base_period,
                .price        = 100.0,
                .volume       = 0.001,
                .amount       = 0.1
            };
            int ret = fc_ticker_merge_update(ctx, &tick);
            ASSERT_EQ(ret, FC_OK);
        }

        // Trigger completion
        fc_tick_t tick = {
            .symbol_id    = 0,
            .timestamp_ns = 1000000000000LL + 500 * base_period,
            .price        = 100.0,
            .volume       = 0.001,
            .amount       = 0.1
        };
        int ret = fc_ticker_merge_update(ctx, &tick);
        ASSERT_EQ(ret, FC_OK);

        fc_ohlcv_t derived_ohlcv;
        ret = fc_ticker_merge_get_derived_ohlcv(ctx, 0, 0, &derived_ohlcv);
        ASSERT_EQ(ret, FC_OK);
        ASSERT_EQ(derived_ohlcv.initialized, 1);
        ASSERT_EQ(derived_ohlcv.tick_count, 500);

        // Kahan and BigFloat should be more accurate than standard
        if (modes[mode_idx] != FC_TICKER_PRECISION_STANDARD) {
            FC_TEST_ASSERT_DOUBLE_EQ(derived_ohlcv.volume, 0.5, 1e-9);
        }

        fc_ticker_merge_destroy(ctx);
    }
}

void register_ticker_merge_tests(void) {
    RUN_TEST(test_ticker_merge_create_destroy);
    RUN_TEST(test_ticker_merge_invalid_args);
    RUN_TEST(test_ticker_merge_base_period_only);
    RUN_TEST(test_ticker_merge_5min_from_1min);
    RUN_TEST(test_ticker_merge_multiple_periods);
    RUN_TEST(test_ticker_merge_multiple_symbols);
    RUN_TEST(test_ticker_merge_reset);
    RUN_TEST(test_ticker_merge_bigfloat_precision);
    RUN_TEST(test_ticker_merge_kahan_precision);
    RUN_TEST(test_ticker_merge_get_derived_ohlcv);
    RUN_TEST(test_ticker_merge_get_derived_history);
    RUN_TEST(test_ticker_merge_reset_all);
    RUN_TEST(test_ticker_merge_invalid_tick_data);
    RUN_TEST(test_ticker_merge_batch_update);
    RUN_TEST(test_ticker_merge_get_stats_null_outputs);
    RUN_TEST(test_ticker_merge_error_handling);
    RUN_TEST(test_ticker_merge_derived_period_error_handling);
    RUN_TEST(test_ticker_merge_high_low_tracking);
    RUN_TEST(test_ticker_merge_price_fluctuation);
    RUN_TEST(test_ticker_merge_large_merge_count_256);
    RUN_TEST(test_ticker_merge_large_merge_count_1440);
    RUN_TEST(test_ticker_merge_large_merge_count_3600);
    RUN_TEST(test_ticker_merge_all_precision_modes_large_count);
}
