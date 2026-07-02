/**
 * @file index_rebal.h
 * @brief Index rebalancing strategy
 *
 * Strategy-level index rebalancing implementation for tracking passive fund flows.
 *
 * This module provides:
 * - Batch index NAV calculation (weighted sum of component prices)
 * - Rebalancing quantity calculation for index tracking
 * - Support for multiple indices with variable number of constituents
 *
 * Key functionality:
 *   NAV = Σ (price_i × weight_i) for each index
 *   target_position_i = (tracking_aum × weight_i) / price_i
 *   rebalancing_qty_i = target_position_i - current_position_i
 *
 * Use case: Front-running passive index fund rebalancing events
 * - Predict rebalancing timing based on index constituent changes
 * - Calculate target positions that passive funds must achieve
 * - Execute trades ahead of anticipated passive fund flows
 *
 * Performance characteristics:
 * - Batch processing for multiple indices simultaneously
 * - SIMD-optimized weighted sum with Kahan summation for precision
 * - Zero heap allocation on hot path
 */

#ifndef FC_EX_STRAT_INDEX_REBAL_H
#define FC_EX_STRAT_INDEX_REBAL_H

#include "error.h"
#include "platform.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Calculate index NAV and optional rebalancing quantities
 *
 * Computes Net Asset Value (NAV) for multiple indices and optionally calculates
 * the rebalancing quantities needed to track each index given current positions.
 *
 * NAV calculation:
 *   NAV_j = Σ(i=0 to n_constituents[j]-1) price[j,i] × weight[j,i]
 *
 * Rebalancing calculation (when current_qty and tracking_aum provided):
 *   target_qty[j,i] = (tracking_aum[j] × weight[j,i]) / price[j,i]
 *   rebal_qty[j,i] = target_qty[j,i] - current_qty[j,i]
 *
 * Data layout: All arrays use row-major 2D layout where each index j has
 * max_constituents slots. Only the first n_constituents[j] elements are used.
 * Example for n_index=2, max_constituents=3:
 *   prices = [idx0_c0, idx0_c1, idx0_c2, idx1_c0, idx1_c1, idx1_c2]
 *
 * @param[out] nav_out NAV for each index (length n_index)
 * @param[out] rebal_qty_out Rebalancing quantities (length n_index × max_constituents),
 *                           optional (pass NULL to skip). Values are continuous (not rounded).
 *                           Rounding to lot size should be done in Go layer.
 * @param[in] prices Component prices (length n_index × max_constituents)
 * @param[in] weights Target component weights (length n_index × max_constituents),
 *                    must sum to 1.0 for each index (not validated)
 * @param[in] current_qty Current position quantities (length n_index × max_constituents),
 *                        required if rebal_qty_out is non-NULL, otherwise pass NULL
 * @param[in] tracking_aum Tracking fund AUM for each index (length n_index),
 *                         required if rebal_qty_out is non-NULL, otherwise pass NULL
 * @param[in] n_constituents Number of constituents for each index (length n_index),
 *                           must be > 0 and <= max_constituents
 * @param[in] n_index Number of indices to process (must be > 0)
 * @param[in] max_constituents Maximum constituents per index (must be > 0)
 *
 * @return FC_OK on success, error code otherwise
 *
 * Time complexity: O(n_index × avg(n_constituents))
 * Space complexity: O(1) additional space
 * Thread safety: Thread-safe (no global state)
 *
 * Input validation:
 * - nav_out must not be NULL
 * - prices, weights, n_constituents must not be NULL
 * - n_index must be > 0
 * - max_constituents must be > 0
 * - n_constituents[j] must be > 0 and <= max_constituents for all j
 * - If rebal_qty_out is non-NULL, current_qty and tracking_aum must not be NULL
 * - prices[j,i] must be > 0 when calculating rebalancing quantities (to avoid division by zero)
 *
 * Performance notes:
 * - For n_index × max_constituents >= 1000, batch processing amortizes cgo overhead
 * - SIMD optimization with Kahan compensated summation for NAV calculation
 * - Uses AVX-512 > AVX2 > SSE4.2 > scalar dispatch
 * - All input arrays must be C-allocated and properly aligned for zero-copy cgo
 *
 * @note NaN/Inf in prices or weights will propagate to outputs
 * @note Weights are not validated to sum to 1.0 (caller responsibility)
 * @note Rebalancing quantities are continuous values; rounding to lot size
 *       should be performed in the Go orchestration layer
 */
FC_API fc_status_t fc_ex_strat_index_nav(
    double* nav_out,
    double* rebal_qty_out,
    const double* prices,
    const double* weights,
    const double* current_qty,
    const double* tracking_aum,
    const int* n_constituents,
    size_t n_index,
    int max_constituents
);

#ifdef __cplusplus
}
#endif

#endif /* FC_EX_STRAT_INDEX_REBAL_H */
