/**
 * @file vwap_dev.h
 * @brief VWAP deviation signal computation
 *
 * VWAP (Volume-Weighted Average Price) deviation is a mean-reversion signal that
 * measures how far the current price deviates from the volume-weighted average.
 * Deviations exceeding 2σ often indicate mean-reversion opportunities.
 *
 * Formula:
 *   VWAP = Σ(P·V) / Σ(V)    O(1) incremental update
 *   z = (P - VWAP_T) / σ_T   z-score deviation
 *
 * Multiple windows (1s/5s/1m/5m) can be computed in parallel for multi-timeframe
 * analysis. Effective in high-liquidity instruments; less reliable in trending markets.
 */

#ifndef FC_EX_SIG_VWAP_DEV_H
#define FC_EX_SIG_VWAP_DEV_H

#include "error.h"
#include "platform.h"

FC_BEGIN_DECLS

/**
 * @brief Compute VWAP deviation z-scores for multiple symbols and windows
 *
 * Calculates z-score deviation from VWAP for multiple symbols across multiple
 * time windows in a single batch operation. This enables multi-timeframe mean-
 * reversion analysis with SIMD acceleration.
 *
 * Formula: z = (P - VWAP_T) / σ_T
 *
 * Where:
 *   P: Current price
 *   VWAP_T: Volume-weighted average price over window T
 *   σ_T: Standard deviation over window T
 *
 * Trading signal: |z| > 2 typically indicates mean-reversion opportunity
 *
 * @param[out] z_out      Output z-scores (n_symbols × n_windows), row-major
 * @param[in]  price      Current prices (n_symbols)
 * @param[in]  vwap       VWAP for each window (n_symbols × n_windows), row-major
 * @param[in]  sigma      Standard deviation for each window (n_symbols × n_windows), row-major
 * @param[in]  n_symbols  Number of symbols
 * @param[in]  n_windows  Number of time windows per symbol
 *
 * @return FC_OK on success, FC_ERR_INVALID_ARG if inputs invalid or any pointer is NULL
 *
 * @note Division by zero protection: if σ_T ≤ eps (1e-10), z is set to 0
 * @note All arrays must be 64-byte aligned for optimal SIMD performance
 * @note Uses runtime SIMD dispatch (AVX-512 > AVX2 > SSE4.2 > Scalar)
 * @note Time complexity: O(n_symbols * n_windows) with SIMD parallelism
 * @note Thread-safe: no shared mutable state
 */
FC_API fc_status_t fc_ex_sig_vwap_dev_batch(
    double* z_out,
    const double* price,
    const double* vwap,
    const double* sigma,
    size_t n_symbols,
    int n_windows
);

/**
 * @brief Incrementally update VWAP accumulators with new price-volume pairs
 *
 * Updates VWAP using Kahan compensated summation for numerical stability in
 * long-running accumulators. This function maintains running sums of P×V and V
 * to enable O(1) VWAP calculation.
 *
 * Formula:
 *   pv_acc += P × V     (with Kahan compensation)
 *   v_acc += V          (with Kahan compensation)
 *   VWAP = pv_acc / v_acc
 *
 * Kahan summation ensures precision even with millions of updates by tracking
 * and compensating for floating-point rounding errors.
 *
 * @param[in,out] pv_acc   Σ(P×V) accumulators (n), updated in-place
 * @param[in,out] v_acc    Σ(V) accumulators (n), updated in-place
 * @param[in,out] pv_comp  Kahan compensation for pv_acc (n), updated in-place
 * @param[in,out] v_comp   Kahan compensation for v_acc (n), updated in-place
 * @param[out]    vwap_out Computed VWAP values (n)
 * @param[in]     price    New prices to add (n)
 * @param[in]     volume   New volumes to add (n)
 * @param[in]     n        Number of symbols/accumulators
 *
 * @return FC_OK on success, FC_ERR_INVALID_ARG if inputs invalid or any pointer is NULL
 *
 * @note Division by zero protection: if v_acc ≤ eps (1e-10), VWAP is set to price
 * @note Accumulators and compensation terms must be initialized to 0 before first call
 * @note Uses Kahan summation to minimize floating-point accumulation error
 * @note All arrays must be 64-byte aligned for optimal SIMD performance
 * @note Time complexity: O(n) with SIMD parallelism
 * @note Thread-safe: no shared mutable state (caller manages state arrays)
 */
FC_API fc_status_t fc_ex_sig_vwap_update(
    double* pv_acc,
    double* v_acc,
    double* pv_comp,
    double* v_comp,
    double* vwap_out,
    const double* price,
    const double* volume,
    size_t n
);

FC_END_DECLS

#endif /* FC_EX_SIG_VWAP_DEV_H */
