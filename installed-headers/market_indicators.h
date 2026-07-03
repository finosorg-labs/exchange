/**
 * @file market_indicators.h
 * @brief Realtime market indicator calculation
 *
 * Provides per-symbol realtime VWAP, TWAP, volatility, buy/sell pressure ratio,
 * and full-market ranking for batched market data processing.
 */

#ifndef FC_MARKET_INDICATORS_H
#define FC_MARKET_INDICATORS_H

#include "error.h"
#include "platform.h"
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct fc_market_indicators_ctx fc_market_indicators_ctx_t;

typedef enum {
    FC_MARKET_INDICATORS_PRECISION_KAHAN    = 0,
    FC_MARKET_INDICATORS_PRECISION_STANDARD = 1,
    FC_MARKET_INDICATORS_PRECISION_BIGFLOAT = 2
} fc_market_indicators_precision_mode_t;

typedef enum {
    FC_MARKET_INDICATOR_VWAP           = 0,
    FC_MARKET_INDICATOR_TWAP           = 1,
    FC_MARKET_INDICATOR_VOLATILITY     = 2,
    FC_MARKET_INDICATOR_PRESSURE_RATIO = 3
} fc_market_indicator_kind_t;

typedef struct {
    uint32_t symbol_id;
    double price;
    double volume;
    double buy_volume;
    double sell_volume;
    int64_t timestamp_ns;
} fc_market_trade_t;

typedef struct {
    double vwap;
    double twap;
    double volatility;
    double buy_sell_pressure_ratio;
    uint64_t trade_count;
    double total_volume;
    double total_amount;
    int64_t window_start_ns;
    int64_t window_end_ns;
    uint8_t initialized;
} fc_market_indicators_t;

/**
 * @brief Create a market indicators context.
 *
 * @param num_symbols Number of symbols to track.
 * @param window_duration_ns Tumbling window duration in nanoseconds.
 * @param precision_mode Accumulation precision mode.
 * @return Context pointer, or NULL on invalid input or allocation failure.
 *
 * Time complexity: O(num_symbols)
 * Space complexity: O(num_symbols)
 * Thread safety: Not thread-safe; caller must synchronize.
 */
fc_market_indicators_ctx_t* fc_market_indicators_create(
    uint32_t num_symbols,
    int64_t window_duration_ns,
    fc_market_indicators_precision_mode_t precision_mode
);

/**
 * @brief Destroy a market indicators context.
 *
 * @param ctx Context to destroy.
 *
 * Time complexity: O(1)
 * Thread safety: Not thread-safe.
 */
void fc_market_indicators_destroy(fc_market_indicators_ctx_t* ctx);

/**
 * @brief Update indicators with one trade.
 *
 * @param ctx Context to update.
 * @param trade Trade input.
 * @return FC_OK on success, error code on validation failure.
 *
 * Time complexity: O(1)
 * Space complexity: O(1)
 * Thread safety: Not thread-safe.
 */
fc_status_t fc_market_indicators_update(
    fc_market_indicators_ctx_t* ctx,
    const fc_market_trade_t* trade
);

/**
 * @brief Update indicators with a batch of trades.
 *
 * Stops at the first invalid trade. Previously processed trades remain applied.
 *
 * @param ctx Context to update.
 * @param trades Trade array.
 * @param num_trades Number of trades.
 * @return FC_OK on success, error code on first failure.
 *
 * Time complexity: O(num_trades)
 * Space complexity: O(1)
 * Thread safety: Not thread-safe.
 */
fc_status_t fc_market_indicators_update_batch(
    fc_market_indicators_ctx_t* ctx,
    const fc_market_trade_t* trades,
    size_t num_trades
);

/**
 * @brief Get indicators for one symbol.
 *
 * @param ctx Context to query.
 * @param symbol_id Symbol identifier.
 * @param out Output indicator values.
 * @return FC_OK on success, error code on validation failure.
 *
 * Time complexity: O(1)
 * Space complexity: O(1)
 * Thread safety: Not thread-safe.
 */
fc_status_t fc_market_indicators_get(
    const fc_market_indicators_ctx_t* ctx,
    uint32_t symbol_id,
    fc_market_indicators_t* out
);

/**
 * @brief Get indicators for all symbols.
 *
 * @param ctx Context to query.
 * @param out_array Output array of size num_symbols.
 * @return FC_OK on success, error code on validation failure.
 *
 * Time complexity: O(num_symbols)
 * Space complexity: O(1)
 * Thread safety: Not thread-safe.
 */
fc_status_t fc_market_indicators_get_all(
    const fc_market_indicators_ctx_t* ctx,
    fc_market_indicators_t* out_array
);

/**
 * @brief Rank symbols by an indicator.
 *
 * @param ctx Context to query.
 * @param kind Indicator to rank by.
 * @param out_symbol_ids Output symbol identifiers.
 * @param out_values Output indicator values.
 * @param max_results Maximum number of results to return.
 * @param out_count Number of results returned.
 * @param descending Non-zero for descending order, zero for ascending order.
 * @return FC_OK on success, error code on validation failure.
 *
 * Time complexity: O(num_symbols^2) for small full-market universes.
 * Space complexity: O(num_symbols)
 * Thread safety: Not thread-safe.
 */
fc_status_t fc_market_indicators_rank(
    const fc_market_indicators_ctx_t* ctx,
    fc_market_indicator_kind_t kind,
    uint32_t* out_symbol_ids,
    double* out_values,
    size_t max_results,
    size_t* out_count,
    int descending
);

fc_status_t fc_market_indicators_reset_symbol(fc_market_indicators_ctx_t* ctx, uint32_t symbol_id);

fc_status_t fc_market_indicators_reset_all(fc_market_indicators_ctx_t* ctx);

fc_status_t fc_market_indicators_get_stats(
    const fc_market_indicators_ctx_t* ctx,
    uint32_t* out_num_symbols
);

#ifdef __cplusplus
}
#endif

#endif /* FC_MARKET_INDICATORS_H */
