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

#ifdef __cplusplus
}
#endif

#endif /* FC_EX_SIGNAL_SPREAD_H */
