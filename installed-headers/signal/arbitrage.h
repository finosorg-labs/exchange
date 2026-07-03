/**
 * @file arbitrage.h
 * @brief Cross-market arbitrage spread computation
 *
 * Calculates arbitrage opportunities across multiple trading markets by computing
 * price spreads. For each market pair (i, j), computes:
 *
 * spread[i][j] = bid_i - ask_j - fee_i - fee_j
 *
 * where:
 * - bid_i: best bid price at market i (where you can sell)
 * - ask_j: best ask price at market j (where you can buy)
 * - fee_i, fee_j: transaction fees at both markets
 *
 * Positive spread indicates arbitrage opportunity: buy at market j, sell at market i.
 * Window: ~10-500μs for latency arbitrage (speed competition).
 *
 * Typical use case: co-location + dark fiber for cross-exchange arbitrage
 * (e.g., NYSE vs NASDAQ, Binance vs OKX, CME vs CBOE).
 */

#ifndef FC_EX_SIG_ARBITRAGE_H
#define FC_EX_SIG_ARBITRAGE_H

#include "error.h"
#include "platform.h"

FC_BEGIN_DECLS

/**
 * @brief Compute cross-market arbitrage spread matrix
 *
 * Calculates pairwise arbitrage spreads for all market combinations.
 * Output is an n_markets × n_markets matrix where element [i][j] represents
 * the spread for buying at market j and selling at market i.
 *
 * Formula: spread[i][j] = bid_i - ask_j - fee_i - fee_j
 *
 * Positive values indicate profitable arbitrage opportunities (after fees).
 * Diagonal elements (i == j) represent same-market arbitrage and are set to NaN
 * to indicate they are meaningless.
 *
 * @param[out] spread_out   Spread matrix (n_markets × n_markets), row-major order
 * @param[in]  best_bid     Best bid price at each market (n_markets)
 * @param[in]  best_ask     Best ask price at each market (n_markets)
 * @param[in]  fees         Transaction fee at each market (n_markets)
 * @param[in]  n_markets     Number of trading markets
 *
 * @return FC_OK on success, error code otherwise
 *
 * @note All input arrays must be 64-byte aligned for optimal SIMD performance
 * @note Uses runtime SIMD dispatch (AVX-512 > AVX2 > SSE4.2 > Scalar)
 * @note Time complexity: O(n_markets²) with SIMD parallelism
 * @note Thread-safe: no shared mutable state
 * @note Fees should be absolute values (e.g., 0.001 for 0.1%), not percentages
 */
FC_API fc_status_t fc_ex_sig_arb_spread(
    double* spread_out,
    const double* best_bid,
    const double* best_ask,
    const double* fees,
    int n_markets
);

/**
 * @brief Compute cross-market arbitrage spread matrix (aligned variant)
 *
 * Same as fc_ex_sig_arb_spread but requires all input arrays to be 64-byte aligned
 * for optimal SIMD performance. Uses aligned load/store instructions (5-10% faster).
 *
 * @param[out] spread_out   Spread matrix (n_markets × n_markets), 64-byte aligned
 * @param[in]  best_bid     Best bid price at each market (n_markets), 64-byte aligned
 * @param[in]  best_ask     Best ask price at each market (n_markets), 64-byte aligned
 * @param[in]  fees         Transaction fee at each market (n_markets), 64-byte aligned
 * @param[in]  n_markets     Number of trading markets
 *
 * @return FC_OK on success, error code otherwise
 *
 * @note ALL input arrays MUST be 64-byte aligned (use fc_aligned_alloc)
 * @note Uses runtime SIMD dispatch (AVX-512 > AVX2 > SSE4.2 > Scalar)
 * @note Time complexity: O(n_markets²) with SIMD parallelism
 * @note Thread-safe: no shared mutable state
 * @note Diagonal elements are set to NaN (same-market arbitrage meaningless)
 */
FC_API fc_status_t fc_ex_sig_arb_spread_aligned(
    double* restrict spread_out,
    const double* restrict best_bid,
    const double* restrict best_ask,
    const double* restrict fees,
    int n_markets
);

/**
 * @brief Convert percentage-based fees to absolute values
 *
 * Helper function to convert percentage fees (e.g., 0.1 for 0.1%) to absolute
 * values based on current prices. Useful when fees are charged as a percentage
 * of trade value rather than fixed amounts.
 *
 * For each market i: fees_out[i] = (best_bid[i] + best_ask[i]) / 2 * fee_pct[i] / 100
 *
 * @param[out] fees_out     Absolute fee values (n_markets)
 * @param[in]  best_bid     Best bid prices (n_markets)
 * @param[in]  best_ask     Best ask prices (n_markets)
 * @param[in]  fee_pct      Fee percentages (n_markets), e.g., 0.1 for 0.1%
 * @param[in]  n_markets     Number of markets
 *
 * @return FC_OK on success, error code otherwise
 *
 * @note Uses mid-price (bid+ask)/2 as the basis for fee calculation
 * @note Thread-safe: no shared mutable state
 * @note Time complexity: O(n_markets)
 *
 * @example
 *   double bid[] = {100.0, 200.0};
 *   double ask[] = {100.1, 200.2};
 *   double pct[] = {0.1, 0.05};  // 0.1% and 0.05%
 *   double fees[2];
 *   fc_ex_sig_arb_fee_pct_to_abs(fees, bid, ask, pct, 2);
 *   // fees[0] = 100.05 * 0.1 / 100 = 0.10005
 *   // fees[1] = 200.1 * 0.05 / 100 = 0.10005
 */
FC_API fc_status_t fc_ex_sig_arb_fee_pct_to_abs(
    double* fees_out,
    const double* best_bid,
    const double* best_ask,
    const double* fee_pct,
    int n_markets
);

FC_END_DECLS

#endif /* FC_EX_SIG_ARBITRAGE_H */
