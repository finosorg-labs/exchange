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

#include <error.h>
#include <platform.h>

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
 * Diagonal elements (i == j) are meaningless but computed for simplicity.
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

FC_END_DECLS

#endif /* FC_EX_SIG_ARBITRAGE_H */
