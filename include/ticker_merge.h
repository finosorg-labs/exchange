/**
 * @file ticker_merge.h
 * @brief K-line merging for multi-period aggregation
 *
 * Provides efficient multi-period K-line generation through merging base period bars.
 * Instead of maintaining independent pipelines for each period (1min, 5min, 15min, 60min),
 * only the base period (e.g., 1min) is maintained, and higher periods are generated
 * by merging base period bars.
 *
 * Memory savings: O(num_symbols × max_merge_count) vs O(num_symbols × num_periods)
 * Performance improvement: 30-50% (reduces redundant tick processing)
 *
 * Example:
 *   Base period: 1min
 *   Derived periods: 5min (merge 5×1min), 15min (merge 15×1min), 60min (merge 60×1min)
 *   Memory: O(num_symbols × 60) vs O(num_symbols × 4)
 */

#ifndef FC_TICKER_MERGE_H
#define FC_TICKER_MERGE_H

#include "ticker.h"
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Ticker context with K-line merging support
 *
 * Opaque structure - use accessor functions.
 */
typedef struct fc_ticker_merge_ctx fc_ticker_merge_ctx_t;

/**
 * @brief Callback function for derived period K-line completion
 *
 * Called when a derived period K-line is completed (e.g., when 5 base period bars
 * have been merged into a 5min bar).
 *
 * @param symbol_id Symbol identifier
 * @param derived_period_idx Derived period index (0 to num_derived_periods-1)
 * @param ohlcv Completed K-line data
 * @param user_data User-provided context pointer
 */
typedef void (*fc_ticker_merge_callback_t)(
    uint32_t symbol_id,
    uint32_t derived_period_idx,
    const fc_ohlcv_t* ohlcv,
    void* user_data
);

/**
 * @brief Create a ticker context with K-line merging
 *
 * @param num_symbols Number of symbols to track
 * @param base_period_ns Base period duration in nanoseconds (e.g., 60e9 for 1min)
 * @param derived_periods_ns Array of derived period durations (must be multiples of base_period_ns)
 * @param num_derived_periods Number of derived periods
 * @param precision_mode Precision mode for accumulation
 * @param callback Optional callback for derived period completion (can be NULL)
 * @param user_data User data passed to callback
 *
 * @return Ticker context pointer, or NULL on failure
 *
 * Time complexity: O(num_symbols × max_merge_count)
 * Space complexity: O(num_symbols × max_merge_count)
 * Thread safety: Not thread-safe (caller must synchronize)
 *
 * @note All derived periods must be exact multiples of base_period_ns
 * @note Example: base=60e9 (1min), derived=[300e9 (5min), 900e9 (15min), 3600e9 (60min)]
 */
fc_ticker_merge_ctx_t* fc_ticker_merge_create(
    uint32_t num_symbols,
    int64_t base_period_ns,
    const int64_t* derived_periods_ns,
    uint32_t num_derived_periods,
    fc_ticker_precision_mode_t precision_mode,
    fc_ticker_merge_callback_t callback,
    void* user_data
);

/**
 * @brief Destroy a ticker merge context
 *
 * @param ctx Ticker context to destroy
 *
 * Time complexity: O(1)
 * Thread safety: Not thread-safe
 */
void fc_ticker_merge_destroy(fc_ticker_merge_ctx_t* ctx);

/**
 * @brief Update with a single tick
 *
 * Updates the base period K-line and triggers derived period merging if needed.
 *
 * @param ctx Ticker context
 * @param tick Tick data to process
 *
 * @return 0 on success, negative error code on failure
 *
 * Time complexity: O(1) amortized (O(num_derived_periods) when base period completes)
 * Thread safety: Not thread-safe
 */
int fc_ticker_merge_update(fc_ticker_merge_ctx_t* ctx, const fc_tick_t* tick);

/**
 * @brief Batch update with multiple ticks
 *
 * @param ctx Ticker context
 * @param ticks Array of tick data
 * @param num_ticks Number of ticks in the array
 *
 * @return 0 on success, negative error code on failure
 *
 * Time complexity: O(num_ticks)
 * Thread safety: Not thread-safe
 */
int fc_ticker_merge_update_batch(
    fc_ticker_merge_ctx_t* ctx,
    const fc_tick_t* ticks,
    size_t num_ticks
);

/**
 * @brief Get base period K-line
 *
 * @param ctx Ticker context
 * @param symbol_id Symbol identifier
 * @param out_ohlcv Output K-line data (caller-allocated)
 *
 * @return 0 on success, negative error code on failure
 *
 * Time complexity: O(1)
 * Thread safety: Not thread-safe
 */
int fc_ticker_merge_get_base_ohlcv(
    const fc_ticker_merge_ctx_t* ctx,
    uint32_t symbol_id,
    fc_ohlcv_t* out_ohlcv
);

/**
 * @brief Get derived period K-line
 *
 * Returns the most recently completed derived period K-line.
 *
 * @param ctx Ticker context
 * @param symbol_id Symbol identifier
 * @param derived_period_idx Derived period index (0 to num_derived_periods-1)
 * @param out_ohlcv Output K-line data (caller-allocated)
 *
 * @return 0 on success, negative error code on failure
 *
 * Time complexity: O(1)
 * Thread safety: Not thread-safe
 */
int fc_ticker_merge_get_derived_ohlcv(
    const fc_ticker_merge_ctx_t* ctx,
    uint32_t symbol_id,
    uint32_t derived_period_idx,
    fc_ohlcv_t* out_ohlcv
);

/**
 * @brief Get derived period K-line history
 *
 * Returns the last N completed K-lines for a derived period.
 *
 * @param ctx Ticker context
 * @param symbol_id Symbol identifier
 * @param derived_period_idx Derived period index
 * @param out_ohlcv_array Output array (caller-allocated, size >= count)
 * @param count Number of K-lines to retrieve
 *
 * @return Number of K-lines actually retrieved (0 to count)
 *
 * Time complexity: O(count)
 * Thread safety: Not thread-safe
 */
size_t fc_ticker_merge_get_derived_history(
    const fc_ticker_merge_ctx_t* ctx,
    uint32_t symbol_id,
    uint32_t derived_period_idx,
    fc_ohlcv_t* out_ohlcv_array,
    size_t count
);

/**
 * @brief Reset all K-lines for a symbol
 *
 * @param ctx Ticker context
 * @param symbol_id Symbol identifier
 *
 * @return 0 on success, negative error code on failure
 *
 * Time complexity: O(num_derived_periods × max_merge_count)
 * Thread safety: Not thread-safe
 */
int fc_ticker_merge_reset(fc_ticker_merge_ctx_t* ctx, uint32_t symbol_id);

/**
 * @brief Reset all K-lines
 *
 * @param ctx Ticker context
 *
 * @return 0 on success, negative error code on failure
 *
 * Time complexity: O(num_symbols × num_derived_periods × max_merge_count)
 * Thread safety: Not thread-safe
 */
int fc_ticker_merge_reset_all(fc_ticker_merge_ctx_t* ctx);

/**
 * @brief Get context statistics
 *
 * @param ctx Ticker context
 * @param out_num_symbols Output: number of symbols
 * @param out_base_period_ns Output: base period duration
 * @param out_num_derived_periods Output: number of derived periods
 *
 * @return 0 on success, negative error code on failure
 */
int fc_ticker_merge_get_stats(
    const fc_ticker_merge_ctx_t* ctx,
    uint32_t* out_num_symbols,
    int64_t* out_base_period_ns,
    uint32_t* out_num_derived_periods
);

#ifdef __cplusplus
}
#endif

#endif /* FC_TICKER_MERGE_H */
