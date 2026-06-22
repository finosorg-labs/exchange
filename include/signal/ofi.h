/**
 * @file ofi.h
 * @brief Order Flow Imbalance (OFI) signal computation
 *
 * OFI is a core microstructure signal that measures the net change in bid/ask order
 * quantities at the best price levels. It has strong linear correlation with short-term
 * price movements (R² 0.6-0.8 for high-liquidity instruments).
 *
 * Formula: OFI_t = e_b - e_a
 * where e_b and e_a are bid/ask side contributions based on three scenarios:
 * - Price improvement: full new quantity
 * - Price unchanged: quantity delta
 * - Price deterioration: negative previous quantity
 *
 * Batch operations amortize cgo overhead and leverage SIMD vectorization.
 * Single-tick operations should use the pure Go implementation.
 */

#ifndef FC_EX_SIG_OFI_H
#define FC_EX_SIG_OFI_H

#include <error.h>
#include <platform.h>

FC_BEGIN_DECLS

/**
 * @brief Single instrument top-of-book quote snapshot
 *
 * Represents the best bid/ask price and quantity at a specific time point.
 */
typedef struct {
    double bid_p; /**< Best bid price */
    double bid_q; /**< Best bid quantity */
    double ask_p; /**< Best ask price */
    double ask_q; /**< Best ask quantity */
} fc_ex_top_quote_t;

/**
 * @brief Multi-level book snapshot (for weighted OFI)
 *
 * All arrays point to C-side allocated shared SoA buffers (zero-copy from order book).
 * For batch operations, use flattened arrays (bid_p[n_symbols*n_levels]) for better
 * SIMD efficiency instead of array-of-structs.
 */
typedef struct {
    const double* bid_p; /**< Bid prices [n_levels] */
    const double* bid_q; /**< Bid quantities [n_levels] */
    const double* ask_p; /**< Ask prices [n_levels] */
    const double* ask_q; /**< Ask quantities [n_levels] */
    int n_levels;        /**< Number of price levels */
} fc_ex_book_levels_t;

/**
 * @brief Compute OFI for multiple symbols (batch operation)
 *
 * Calculates Order Flow Imbalance for multiple symbols in a single batch,
 * amortizing cgo overhead and leveraging SIMD vectorization.
 *
 * Three-scenario logic per side:
 * - Price improvement (bid up/ask down): e = +qty_current
 * - Price unchanged: e = qty_current - qty_prev
 * - Price deterioration (bid down/ask up): e = -qty_prev
 *
 * OFI_t = e_bid - e_ask
 *
 * For multi-level weighted OFI:
 * OFI = Σ(w_n * OFI_n) where w_n ∝ 1/n²
 *
 * @param[out] ofi_out       Output OFI values (n_symbols)
 * @param[in]  bid_p_cur     Current bid prices (n_symbols × n_levels)
 * @param[in]  bid_q_cur     Current bid quantities (n_symbols × n_levels)
 * @param[in]  ask_p_cur     Current ask prices (n_symbols × n_levels)
 * @param[in]  ask_q_cur     Current ask quantities (n_symbols × n_levels)
 * @param[in]  bid_p_prev    Previous bid prices (n_symbols × n_levels)
 * @param[in]  bid_q_prev    Previous bid quantities (n_symbols × n_levels)
 * @param[in]  ask_p_prev    Previous ask prices (n_symbols × n_levels)
 * @param[in]  ask_q_prev    Previous ask quantities (n_symbols × n_levels)
 * @param[in]  level_weights Level weights w_n (n_levels), typically ∝ 1/n²
 * @param[in]  n_symbols     Number of symbols
 * @param[in]  n_levels      Number of price levels per symbol
 *
 * @return FC_OK on success, error code otherwise
 *
 * @note All input arrays must be 64-byte aligned for optimal SIMD performance
 * @note Uses runtime SIMD dispatch (AVX-512 > AVX2 > SSE4.2 > Scalar)
 * @note Time complexity: O(n_symbols * n_levels) with SIMD parallelism
 * @note Thread-safe: no shared mutable state
 */
FC_API fc_status_t fc_ex_sig_ofi_batch(
    double* ofi_out,
    const double* bid_p_cur,
    const double* bid_q_cur,
    const double* ask_p_cur,
    const double* ask_q_cur,
    const double* bid_p_prev,
    const double* bid_q_prev,
    const double* ask_p_prev,
    const double* ask_q_prev,
    const double* level_weights,
    size_t n_symbols,
    int n_levels
);

/**
 * @brief Compute rolling integral of OFI time series (∫OFI)
 *
 * Computes cumulative sum of OFI over T ticks for multiple symbols.
 * Uses Kahan compensated summation for numerical stability.
 *
 * Formula: ∫OFI = Σ(t=0 to T-1) OFI_t
 *
 * Typical windows: T = 100-1000 ticks
 *
 * Empirical relationship: ΔP(t+H) ≈ β · ∫OFI + ε
 *
 * @param[out] integral_out  Output ∫OFI values (n_symbols)
 * @param[in]  ofi_series    OFI time series (n_symbols × T), row-major
 * @param[in]  n_symbols     Number of symbols
 * @param[in]  T             Window length (number of ticks)
 *
 * @return FC_OK on success, error code otherwise
 *
 * @note Uses Kahan summation to minimize floating-point accumulation error
 * @note Time complexity: O(n_symbols * T)
 * @note Thread-safe: no shared mutable state
 */
FC_API fc_status_t
fc_ex_sig_ofi_integral(double* integral_out, const double* ofi_series, size_t n_symbols, size_t T);

FC_END_DECLS

#endif /* FC_EX_SIG_OFI_H */
