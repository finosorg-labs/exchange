/**
 * @file spread.h
 * @brief Effective spread and Amihud illiquidity signal computation
 *
 * Effective spread measures the actual transaction cost by comparing trade price
 * to the micro-price (volume-weighted midpoint):
 *
 *     eff_spread = 2 × |P_trade - mp|
 *
 * Amihud illiquidity measures price impact per unit volume:
 *
 *     illiq = |r| / Volume
 *
 * High Amihud values indicate high price impact (low liquidity), suggesting
 * market-making opportunities.
 *
 * Performance:
 * - Batch (C+SIMD): ~5-10ns per symbol (AVX-512)
 *
 * @note All batch functions require 64-byte aligned output buffers for optimal SIMD performance
 */

#ifndef FC_EX_SIGNAL_SPREAD_H
#define FC_EX_SIGNAL_SPREAD_H

#include "error.h"
#include <platform.h>

/* Forward declaration of arena type */
typedef struct fc_arena fc_arena_t;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Compute effective spread for a batch of symbols
 *
 * Calculates effective spread as twice the absolute difference between trade price
 * and micro-price:
 *
 *     eff_out[i] = 2 × |trade_price[i] - micro_price[i]|
 *
 * The effective spread represents the actual transaction cost, accounting for
 * the volume-weighted midpoint rather than simple midpoint.
 *
 * @param[out] eff_out       Effective spread output (n elements, 64-byte aligned recommended)
 * @param[in]  trade_price   Trade prices (n elements)
 * @param[in]  micro_price   Micro-prices (n elements)
 * @param[in]  n             Number of symbols
 *
 * @return FC_OK on success, error code otherwise:
 *         - FC_ERR_INVALID_ARG: any pointer is NULL or n is 0
 *
 * @note Thread-safe (no shared state)
 * @note Time complexity: O(n)
 * @note Space complexity: O(1) auxiliary
 * @note SIMD: Auto-dispatches to AVX-512/AVX2/SSE4.2/Scalar based on CPU capabilities
 */
FC_API fc_status_t fc_ex_sig_eff_spread_batch(
    double* eff_out,
    const double* trade_price,
    const double* micro_price,
    size_t n
);

/**
 * @brief Compute Amihud illiquidity for a batch of symbols
 *
 * Calculates Amihud illiquidity as absolute return divided by volume:
 *
 *     illiq_out[i] = |returns[i]| / volume[i]
 *
 * If volume[i] <= 0, illiq_out[i] is set to NaN.
 *
 * High Amihud values indicate high price impact per unit volume (low liquidity),
 * which may signal market-making opportunities with wider spreads.
 *
 * @param[out] illiq_out     Amihud illiquidity output (n elements, 64-byte aligned recommended)
 * @param[in]  returns       Returns (price changes) (n elements)
 * @param[in]  volume        Trade volumes (n elements)
 * @param[in]  n             Number of symbols
 *
 * @return FC_OK on success, error code otherwise:
 *         - FC_ERR_INVALID_ARG: any pointer is NULL or n is 0
 *
 * @note Thread-safe (no shared state)
 * @note Time complexity: O(n)
 * @note Space complexity: O(1) auxiliary
 * @note SIMD: Auto-dispatches to AVX-512/AVX2/SSE4.2/Scalar based on CPU capabilities
 */
FC_API fc_status_t
fc_ex_sig_amihud_batch(double* illiq_out, const double* returns, const double* volume, size_t n);

/**
 * @brief Compute rolling window mean of effective spread
 *
 * Computes effective spread for each tick, then applies rolling window mean
 * to smooth the time series. This is useful for identifying trend changes in
 * transaction costs over time.
 *
 * Workflow:
 *   1. Compute eff[i] = 2 × |trade_price[i] - micro_price[i]|
 *   2. Compute rolling_mean[i] = mean(eff[max(0, i-window_size+1) : i])
 *
 * Memory management: Uses arena allocator for temporary buffers (zero heap allocation).
 * The arena can be reused across multiple calls by calling fc_arena_reset() between calls,
 * eliminating allocation overhead.
 *
 * Example usage pattern for batch processing:
 * @code
 *   fc_arena_t* arena = fc_arena_create(max_n * sizeof(double));
 *   for (int i = 0; i < num_symbols; i++) {
 *       fc_ex_sig_eff_spread_rolling_mean(..., arena, ...);
 *       fc_arena_reset(arena);  // O(1) reset, no malloc/free
 *   }
 *   fc_arena_destroy(arena);
 * @endcode
 *
 * @param[out] rolling_mean_out  Rolling mean of effective spread (n elements)
 * @param[in]  trade_price       Trade prices (n elements)
 * @param[in]  micro_price       Micro-prices (n elements)
 * @param[in]  arena             Arena allocator for temporary buffer (must not be NULL)
 * @param[in]  n                 Number of ticks
 * @param[in]  window_size       Rolling window size (must be > 0 and <= n)
 *
 * @return FC_OK on success, error code otherwise:
 *         - FC_ERR_INVALID_ARG: any pointer is NULL, n is 0, or window_size invalid
 *         - FC_ERR_OUT_OF_MEMORY: arena has insufficient space (needs n * sizeof(double) bytes)
 *
 * @note Thread-safe (no shared state, but arena must be thread-local or externally synchronized)
 * @note Time complexity: O(n)
 * @note Space complexity: O(1) heap allocation (uses arena for temporary buffer)
 * @note Uses stats module's rolling mean (M02-07) for numerical stability
 * @note Arena requirement: at least n * sizeof(double) bytes available
 */
FC_API fc_status_t fc_ex_sig_eff_spread_rolling_mean(
    double* rolling_mean_out,
    const double* trade_price,
    const double* micro_price,
    fc_arena_t* arena,
    size_t n,
    size_t window_size
);

/**
 * @brief Compute rolling window standard deviation of effective spread
 *
 * Computes effective spread for each tick, then applies rolling window stddev
 * to measure volatility of transaction costs. High stddev indicates unstable
 * liquidity conditions.
 *
 * Workflow:
 *   1. Compute eff[i] = 2 × |trade_price[i] - micro_price[i]|
 *   2. Compute rolling_stddev[i] = stddev(eff[max(0, i-window_size+1) : i])
 *
 * Memory management: Uses arena allocator for temporary buffers (zero heap allocation).
 * The arena can be reused across multiple calls by calling fc_arena_reset() between calls.
 *
 * @param[out] rolling_stddev_out  Rolling stddev of effective spread (n elements)
 * @param[in]  trade_price         Trade prices (n elements)
 * @param[in]  micro_price         Micro-prices (n elements)
 * @param[in]  arena               Arena allocator for temporary buffer (must not be NULL)
 * @param[in]  n                   Number of ticks
 * @param[in]  window_size         Rolling window size (must be > 1 and <= n)
 *
 * @return FC_OK on success, error code otherwise:
 *         - FC_ERR_INVALID_ARG: any pointer is NULL, n is 0, or window_size invalid
 *         - FC_ERR_OUT_OF_MEMORY: arena has insufficient space (needs n * sizeof(double) bytes)
 *
 * @note Thread-safe (no shared state, but arena must be thread-local or externally synchronized)
 * @note Time complexity: O(n)
 * @note Space complexity: O(1) heap allocation (uses arena for temporary buffer)
 * @note Uses stats module's rolling stddev (M02-07) with Welford's algorithm
 * @note Arena requirement: at least n * sizeof(double) bytes available
 */
FC_API fc_status_t fc_ex_sig_eff_spread_rolling_stddev(
    double* rolling_stddev_out,
    const double* trade_price,
    const double* micro_price,
    fc_arena_t* arena,
    size_t n,
    size_t window_size
);

/**
 * @brief Compute rolling window mean of Amihud illiquidity
 *
 * Computes Amihud illiquidity for each tick, then applies rolling window mean
 * to smooth the time series. This provides a smoothed measure of market impact.
 *
 * Workflow:
 *   1. Compute illiq[i] = |returns[i]| / volume[i]
 *   2. Compute rolling_mean[i] = mean(illiq[max(0, i-window_size+1) : i])
 *
 * Memory management: Uses arena allocator for temporary buffers (zero heap allocation).
 * The arena can be reused across multiple calls by calling fc_arena_reset() between calls.
 *
 * @param[out] rolling_mean_out  Rolling mean of Amihud illiquidity (n elements)
 * @param[in]  returns           Returns (price changes) (n elements)
 * @param[in]  volume            Trade volumes (n elements)
 * @param[in]  arena             Arena allocator for temporary buffer (must not be NULL)
 * @param[in]  n                 Number of ticks
 * @param[in]  window_size       Rolling window size (must be > 0 and <= n)
 *
 * @return FC_OK on success, error code otherwise:
 *         - FC_ERR_INVALID_ARG: any pointer is NULL, n is 0, or window_size invalid
 *         - FC_ERR_OUT_OF_MEMORY: arena has insufficient space (needs n * sizeof(double) bytes)
 *
 * @note Thread-safe (no shared state, but arena must be thread-local or externally synchronized)
 * @note Time complexity: O(n)
 * @note Space complexity: O(1) heap allocation (uses arena for temporary buffer)
 * @note NaN values in illiq (from zero volume) are propagated through rolling mean
 * @note Uses stats module's rolling mean (M02-07) for numerical stability
 * @note Arena requirement: at least n * sizeof(double) bytes available
 */
FC_API fc_status_t fc_ex_sig_amihud_rolling_mean(
    double* rolling_mean_out,
    const double* returns,
    const double* volume,
    fc_arena_t* arena,
    size_t n,
    size_t window_size
);

#ifdef __cplusplus
}
#endif

#endif /* FC_EX_SIGNAL_SPREAD_H */
