/**
 * @file feature.h
 * @brief Feature vector extraction from order book state
 *
 * Extracts 50-200 dimensional feature vectors from order book levels for ML/Alpha generation.
 * Features include spreads, depth metrics, micro-price, imbalances, and level-wise statistics.
 *
 * Feature Set (per symbol):
 * - Core features (9): best bid/ask prices and quantities, mid-price, micro-price,
 *   spread (absolute/relative), depth imbalance
 * - Level features (4*n_levels): bid_p, bid_q, ask_p, ask_q for each level
 * - Price gap features (2*(n_levels-1)): inter-level price differences
 * - Depth ratio features (2*n_levels): relative depth at each level
 *
 * Total features = 9 + 8*n_levels
 * For n_levels=5: 49 features per symbol
 * For n_levels=10: 89 features per symbol
 *
 * Performance:
 * - Batch extraction: ~200ns per symbol (AVX-512)
 * - Signal layer budget: ~0.5-2μs total
 *
 * @note All batch functions require 64-byte aligned output buffers for optimal SIMD performance
 */

#ifndef FC_EX_SIGNAL_FEATURE_H
#define FC_EX_SIGNAL_FEATURE_H

/**
 * @brief Maximum number of order book levels supported
 *
 * This limit balances stack usage in SIMD implementations with practical order book depth.
 * Most exchanges provide 5-10 levels; 32 levels is sufficient for all practical use cases.
 */
#define FC_EX_FEATURE_MAX_LEVELS 32

#include "error.h"
#include <platform.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Calculate number of features extracted per symbol
 *
 * Given the number of order book levels, returns the total feature count.
 * Formula: 9 + 8*n_levels
 *
 * @param[in]  n_levels    Number of order book levels per symbol
 *
 * @return Number of features per symbol
 */
FC_INLINE size_t fc_ex_sig_feature_count(int n_levels) {
    return 9 + 8 * (size_t) n_levels;
}

/**
 * @brief Extract feature vectors from order book batch
 *
 * Extracts comprehensive features from multi-level order book snapshots for a batch of symbols.
 * Input data is in SoA (Structure of Arrays) format where all bid prices are contiguous,
 * followed by all bid quantities, etc.
 *
 * Feature Layout (per symbol):
 * [0-8]:    Core features (best_bid_p, best_ask_p, best_bid_q, best_ask_q,
 *           mid_price, micro_price, spread_abs, spread_rel, depth_imbal)
 * [9+]:     Level-wise features for each level k in [0, n_levels):
 *           - bid_p[k], bid_q[k], ask_p[k], ask_q[k]
 *           - bid_depth_ratio[k], ask_depth_ratio[k]
 *           - bid_price_gap[k] (for k > 0), ask_price_gap[k] (for k > 0)
 *
 * Input Layout (SoA - Structure of Arrays):
 * - bid_p: [symbol0_level0, symbol1_level0, ..., symbolN_level0,
 *           symbol0_level1, symbol1_level1, ..., symbolN_level1, ...]
 * - Index formula: bid_p[level_k * n_symbols + symbol_i]
 * - Similar layout for bid_q, ask_p, ask_q (all n_symbols * n_levels elements)
 *
 * @param[out] features_out  Feature matrix (n_symbols × n_features), 64-byte aligned recommended
 * @param[in]  bid_p         Bid prices (n_symbols × n_levels), SoA layout
 * @param[in]  bid_q         Bid quantities (n_symbols × n_levels)
 * @param[in]  ask_p         Ask prices (n_symbols × n_levels)
 * @param[in]  ask_q         Ask quantities (n_symbols × n_levels)
 * @param[in]  n_symbols     Number of symbols
 * @param[in]  n_levels      Number of order book levels per symbol
 *
 * @return FC_OK on success, error code otherwise:
 *         - FC_ERR_INVALID_ARG: any pointer is NULL, n_symbols is 0, n_levels < 1, or n_levels >
 * FC_EX_FEATURE_MAX_LEVELS
 *
 * @note Features with invalid computations (e.g., divide by zero) are set to NaN
 * @note Thread-safe (no shared state)
 * @note Time complexity: O(n_symbols * n_levels)
 * @note Space complexity: O(1) auxiliary
 * @note SIMD: Auto-dispatches to AVX-512/AVX2/SSE4.2/Scalar based on CPU capabilities
 */
FC_API fc_status_t fc_ex_sig_feature_extract(
    double* features_out,
    const double* bid_p,
    const double* bid_q,
    const double* ask_p,
    const double* ask_q,
    size_t n_symbols,
    int n_levels
);

/**
 * @brief Extract core features only (without level-wise details)
 *
 * Extracts only the 9 core features per symbol, which are the most important
 * high-level indicators. Useful for lightweight feature sets or preliminary filtering.
 *
 * Core Features (per symbol):
 * [0]: best_bid_p
 * [1]: best_ask_p
 * [2]: best_bid_q
 * [3]: best_ask_q
 * [4]: mid_price = (best_bid_p + best_ask_p) / 2
 * [5]: micro_price = (best_bid_p * best_ask_q + best_ask_p * best_bid_q) / (best_bid_q +
 * best_ask_q) [6]: spread_abs = best_ask_p - best_bid_p [7]: spread_rel = spread_abs / mid_price
 * [8]: depth_imbal = (total_bid_q - total_ask_q) / (total_bid_q + total_ask_q)
 *
 * @param[out] features_out  Core feature matrix (n_symbols × 9)
 * @param[in]  bid_p         Bid prices (n_symbols × n_levels)
 * @param[in]  bid_q         Bid quantities (n_symbols × n_levels)
 * @param[in]  ask_p         Ask prices (n_symbols × n_levels)
 * @param[in]  ask_q         Ask quantities (n_symbols × n_levels)
 * @param[in]  n_symbols     Number of symbols
 * @param[in]  n_levels      Number of order book levels per symbol
 *
 * @return FC_OK on success, FC_ERR_INVALID_ARG on invalid input
 *
 * @note Thread-safe (no shared state)
 * @note SIMD: Auto-dispatches based on CPU capabilities
 */
FC_API fc_status_t fc_ex_sig_feature_extract_core(
    double* features_out,
    const double* bid_p,
    const double* bid_q,
    const double* ask_p,
    const double* ask_q,
    size_t n_symbols,
    int n_levels
);

#ifdef __cplusplus
}
#endif

#endif /* FC_EX_SIGNAL_FEATURE_H */
