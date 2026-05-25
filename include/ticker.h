/**
 * @file ticker.h
 * @brief Tick data aggregation to OHLCV (Kline) data
 *
 * Aggregates real-time tick data into OHLCV (Open, High, Low, Close, Volume) format
 * for multiple time periods. Supports batch processing of multiple symbols with SIMD
 * optimization for parallel updates.
 *
 * Key features:
 * - Sliding window for High/Low tracking
 * - Kahan summation for Volume/Amount to prevent precision loss
 * - Batch processing for 5000+ symbols with <1ms latency
 * - Multiple time period support (1m, 5m, 15m, 1d, etc.)
 * - SoA (Structure of Arrays) layout for SIMD optimization
 *
 * Performance target: Full market (5000 symbols) aggregation < 1ms
 */

#ifndef FC_TICKER_H
#define FC_TICKER_H

#include "platform.h"
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Tick data structure
 *
 * Represents a single tick (trade) event from the exchange.
 */
typedef struct {
    uint32_t symbol_id;   /**< Symbol identifier (index into symbol array) */
    double price;         /**< Trade price */
    double volume;        /**< Trade volume */
    double amount;        /**< Trade amount (price * volume) */
    int64_t timestamp_ns; /**< Timestamp in nanoseconds since epoch */
} fc_tick_t;

/**
 * @brief OHLCV data structure
 *
 * Represents aggregated OHLCV data for a single time period.
 * Uses Kahan summation for volume and amount to maintain precision.
 */
typedef struct {
    double open;             /**< Opening price */
    double high;             /**< Highest price */
    double low;              /**< Lowest price */
    double close;            /**< Closing price */
    double volume;           /**< Total volume */
    double amount;           /**< Total amount */
    double volume_kahan_c;   /**< Kahan summation compensation for volume */
    double amount_kahan_c;   /**< Kahan summation compensation for amount */
    int64_t period_start_ns; /**< Period start timestamp (nanoseconds) */
    int64_t period_end_ns;   /**< Period end timestamp (nanoseconds) */
    uint32_t tick_count;     /**< Number of ticks in this period */
    uint8_t initialized;     /**< 1 if OHLCV has been initialized, 0 otherwise */
} fc_ohlcv_t;

/**
 * @brief Ticker aggregator context
 *
 * Maintains state for aggregating ticks into OHLCV data across multiple symbols
 * and time periods. Opaque structure - use accessor functions.
 */
typedef struct fc_ticker_ctx fc_ticker_ctx_t;

/**
 * @brief Precision mode for volume/amount accumulation
 */
typedef enum {
    FC_TICKER_PRECISION_KAHAN    = 0, /**< Kahan summation (recommended, ~10% overhead) */
    FC_TICKER_PRECISION_STANDARD = 1, /**< Standard floating-point addition */
    FC_TICKER_PRECISION_BIGFLOAT = 2  /**< Arbitrary precision (future, not implemented) */
} fc_ticker_precision_mode_t;

/**
 * @brief Create a ticker aggregator context
 *
 * @param num_symbols Number of symbols to track
 * @param num_periods Number of time periods per symbol (e.g., 1m, 5m, 15m)
 * @param period_durations_ns Array of period durations in nanoseconds
 * @param precision_mode Precision mode for accumulation
 * @return Ticker context pointer, or NULL on failure
 *
 * Time complexity: O(num_symbols * num_periods)
 * Space complexity: O(num_symbols * num_periods)
 * Thread safety: Not thread-safe (caller must synchronize)
 */
fc_ticker_ctx_t* fc_ticker_create(
    uint32_t num_symbols,
    uint32_t num_periods,
    const int64_t* period_durations_ns,
    fc_ticker_precision_mode_t precision_mode
);

/**
 * @brief Destroy a ticker aggregator context
 *
 * @param ctx Ticker context to destroy
 *
 * Time complexity: O(1)
 * Thread safety: Not thread-safe
 */
void fc_ticker_destroy(fc_ticker_ctx_t* ctx);

/**
 * @brief Update OHLCV with a single tick
 *
 * @param ctx Ticker context
 * @param tick Tick data to process
 * @return 0 on success, negative error code on failure
 *
 * Error codes:
 * - FC_ERR_INVALID_ARG: ctx or tick is NULL, or symbol_id exceeds num_symbols
 *
 * Time complexity: O(num_periods)
 * Thread safety: Not thread-safe
 */
int fc_ticker_update(fc_ticker_ctx_t* ctx, const fc_tick_t* tick);

/**
 * @brief Batch update OHLCV with multiple ticks
 *
 * Processes multiple ticks in a batch for better cache locality and potential
 * SIMD optimization. Ticks do not need to be sorted by symbol or time.
 *
 * If any tick fails validation, the function stops processing and returns an error.
 * All ticks processed before the error remain applied.
 *
 * @param ctx Ticker context
 * @param ticks Array of tick data
 * @param num_ticks Number of ticks in the array
 * @return 0 on success (all ticks processed), negative error code on first failure
 *
 * Error codes:
 * - FC_ERR_INVALID_ARG: ctx or ticks is NULL, or a tick has invalid data
 *
 * Time complexity: O(num_ticks * num_periods)
 * Thread safety: Not thread-safe
 */
int fc_ticker_update_batch(fc_ticker_ctx_t* ctx, const fc_tick_t* ticks, size_t num_ticks);

/**
 * @brief Get OHLCV data for a specific symbol and period
 *
 * @param ctx Ticker context
 * @param symbol_id Symbol identifier
 * @param period_idx Period index (0 to num_periods-1)
 * @param out_ohlcv Output OHLCV data (caller-allocated)
 * @return 0 on success, negative error code on failure
 *
 * Error codes:
 * - FC_ERR_INVALID_ARG: ctx or out_ohlcv is NULL, or symbol_id/period_idx out of range
 *
 * Time complexity: O(1)
 * Thread safety: Not thread-safe
 */
int fc_ticker_get_ohlcv(
    const fc_ticker_ctx_t* ctx,
    uint32_t symbol_id,
    uint32_t period_idx,
    fc_ohlcv_t* out_ohlcv
);

/**
 * @brief Get all OHLCV data for a specific symbol (all periods)
 *
 * @param ctx Ticker context
 * @param symbol_id Symbol identifier
 * @param out_ohlcv_array Output array (caller-allocated, size = num_periods)
 * @return 0 on success, negative error code on failure
 *
 * Error codes:
 * - FC_ERR_INVALID_ARG: ctx or out_ohlcv_array is NULL, or symbol_id out of range
 *
 * Time complexity: O(num_periods)
 * Thread safety: Not thread-safe
 */
int fc_ticker_get_symbol_ohlcv(
    const fc_ticker_ctx_t* ctx,
    uint32_t symbol_id,
    fc_ohlcv_t* out_ohlcv_array
);

/**
 * @brief Reset OHLCV data for a specific symbol and period
 *
 * Clears the OHLCV state, useful when starting a new period.
 *
 * @param ctx Ticker context
 * @param symbol_id Symbol identifier
 * @param period_idx Period index
 * @return 0 on success, negative error code on failure
 *
 * Time complexity: O(1)
 * Thread safety: Not thread-safe
 */
int fc_ticker_reset_ohlcv(fc_ticker_ctx_t* ctx, uint32_t symbol_id, uint32_t period_idx);

/**
 * @brief Reset all OHLCV data
 *
 * @param ctx Ticker context
 * @return 0 on success, negative error code on failure
 *
 * Time complexity: O(num_symbols * num_periods)
 * Thread safety: Not thread-safe
 */
int fc_ticker_reset_all(fc_ticker_ctx_t* ctx);

/**
 * @brief Get context statistics
 *
 * @param ctx Ticker context
 * @param out_num_symbols Output: number of symbols
 * @param out_num_periods Output: number of periods
 * @return 0 on success, negative error code on failure
 */
int fc_ticker_get_stats(
    const fc_ticker_ctx_t* ctx,
    uint32_t* out_num_symbols,
    uint32_t* out_num_periods
);

/**
 * @brief SIMD-optimized multi-array reduction for OHLCV processing
 *
 * Processes 4 arrays simultaneously to compute max, min, and two sums in a single pass.
 * This is the core shared operation between tick aggregation and K-line merging.
 *
 * Uses runtime CPU detection to select optimal SIMD implementation:
 * - AVX-512: processes 8 doubles per instruction
 * - AVX2: processes 4 doubles per instruction
 * - SSE4.2: processes 2 doubles per instruction
 * - Scalar: fallback for non-x86 or older CPUs
 *
 * @param arr1 First array (typically prices or highs)
 * @param arr2 Second array (typically prices or lows)
 * @param arr3 Third array (typically volumes)
 * @param arr4 Fourth array (typically amounts)
 * @param count Number of elements in each array
 * @param out_max Output: maximum value from arr1
 * @param out_min Output: minimum value from arr2
 * @param out_sum3 Output: sum of arr3
 * @param out_sum4 Output: sum of arr4
 *
 * Time complexity: O(count)
 * Performance: ~8x faster than scalar for count >= 32 on AVX2
 * Thread safety: Thread-safe (read-only)
 *
 * Example usage in ticker.c:
 *   fc_ticker_simd_reduce_4arrays(prices, prices, volumes, amounts, count,
 *                                  &high, &low, &vol_sum, &amt_sum);
 *
 * Example usage in ticker_merge.c:
 *   fc_ticker_simd_reduce_4arrays(highs, lows, volumes, amounts, count,
 *                                  &out_high, &out_low, &out_vol, &out_amt);
 */
void fc_ticker_simd_reduce_4arrays(
    const double* arr1,
    const double* arr2,
    const double* arr3,
    const double* arr4,
    size_t count,
    double* out_max,
    double* out_min,
    double* out_sum3,
    double* out_sum4
);

#ifdef __cplusplus
}
#endif

#endif /* FC_TICKER_H */
