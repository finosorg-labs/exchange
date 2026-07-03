/**
 * @file postprocess.h
 * @brief Signal post-processing operations
 *
 * Provides batch post-processing operations for trading signals:
 * - Threshold filtering: zero out signals below minimum threshold
 * - Reversal detection: identify signal direction changes
 * - EMA smoothing: exponential moving average filtering
 * - Clipping: limit signal values to specified range
 *
 * All operations are batched to amortize cgo overhead and leverage SIMD.
 */

#ifndef FC_EX_SIG_POSTPROCESS_H
#define FC_EX_SIG_POSTPROCESS_H

#include "error.h"
#include "platform.h"

FC_BEGIN_DECLS

/**
 * @brief Post-processing configuration
 *
 * Controls signal filtering and smoothing behavior.
 */
typedef struct {
    double threshold; /**< Threshold filter: |sig| < threshold → sig = 0 */
    double ema_alpha; /**< EMA smoothing coefficient (0 < alpha ≤ 1) */
    double clip_lo;   /**< Lower clip bound */
    double clip_hi;   /**< Upper clip bound */
} fc_ex_sig_postproc_cfg_t;

/**
 * @brief Post-process signals with filtering and smoothing
 *
 * Applies a series of transformations to raw signals:
 * 1. Threshold filtering: signals with |value| < threshold are zeroed
 * 2. EMA smoothing: sig_out = alpha * sig + (1-alpha) * ema_state
 * 3. Clipping: sig_out = clamp(sig_out, clip_lo, clip_hi)
 *
 * Processing order:
 * - Threshold filter applied first (on input)
 * - EMA smoothing applied second
 * - Clipping applied last (on output)
 *
 * For reversal detection, use fc_ex_sig_detect_reversal() separately.
 *
 * @param[out] sig_out     Post-processed signals (n)
 * @param[in,out] ema_state EMA state for each signal (n), updated in-place
 * @param[in]  sig_in      Raw input signals (n)
 * @param[in]  cfg         Post-processing configuration
 * @param[in]  n           Number of signals
 *
 * @return FC_OK on success, error code otherwise
 *
 * @note All arrays must be 64-byte aligned for optimal SIMD performance
 * @note EMA state must be initialized before first call (typically to 0.0)
 * @note For disable EMA: set ema_alpha = 1.0
 * @note For no clipping: set clip_lo = -INFINITY, clip_hi = +INFINITY
 * @note NaN/Inf propagation: special values in input will propagate through EMA
 * @note Uses runtime SIMD dispatch (AVX-512 > AVX2 > SSE4.2 > Scalar)
 * @note Time complexity: O(n) with SIMD parallelism
 * @note Thread-safe: no shared mutable state
 */
FC_API fc_status_t fc_ex_sig_postprocess(
    double* sig_out,
    double* ema_state,
    const double* sig_in,
    const fc_ex_sig_postproc_cfg_t* cfg,
    size_t n
);

/**
 * @brief Detect signal reversals in batch
 *
 * Identifies points where signal changes sign (crosses zero).
 * Outputs +1 for positive-to-negative transition, -1 for negative-to-positive,
 * 0 for no reversal.
 *
 * @param[out] reversal_out Reversal indicators (n), values: {-1, 0, +1}
 * @param[in]  sig_prev     Previous signal values (n)
 * @param[in]  sig_cur      Current signal values (n)
 * @param[in]  n            Number of signals
 *
 * @return FC_OK on success, error code otherwise
 *
 * @note Reversal detection logic:
 *       - If sig_prev > 0 and sig_cur < 0: reversal = +1 (bearish)
 *       - If sig_prev < 0 and sig_cur > 0: reversal = -1 (bullish)
 *       - Otherwise: reversal = 0 (no change)
 * @note Time complexity: O(n) with SIMD parallelism
 * @note Thread-safe: no shared mutable state
 */
FC_API fc_status_t fc_ex_sig_detect_reversal(
    double* reversal_out,
    const double* sig_prev,
    const double* sig_cur,
    size_t n
);

/**
 * @brief Sanitize signals by replacing NaN/Inf with specified value
 *
 * Scans input array and replaces any NaN or Inf values with a replacement value
 * (typically 0.0). Useful for cleaning signals before post-processing.
 *
 * @param[out] sig_out      Sanitized signals (n), can be same as sig_in for in-place
 * @param[in]  sig_in       Input signals (n)
 * @param[in]  n            Number of signals
 * @param[in]  replacement  Value to replace NaN/Inf with (typically 0.0)
 *
 * @return FC_OK on success, error code otherwise
 *
 * @note Can be used in-place by passing same pointer for sig_out and sig_in
 * @note Uses runtime SIMD dispatch for performance
 * @note Time complexity: O(n) with SIMD parallelism
 * @note Thread-safe: no shared mutable state
 */
FC_API fc_status_t fc_ex_sig_sanitize_special_values(
    double* sig_out,
    const double* sig_in,
    size_t n,
    double replacement
);

/**
 * @brief Initialize EMA state array
 *
 * Provides common initialization strategies for EMA state before first call
 * to fc_ex_sig_postprocess().
 *
 * Strategies:
 * - Zero initialization: state = 0 (causes first signal to be scaled by alpha)
 * - Signal initialization: state = first signal value (no scaling on first call)
 * - Custom initialization: state = custom value
 *
 * @param[out] ema_state    EMA state array to initialize (n)
 * @param[in]  init_values  Initialization values (n), or NULL for zero init
 * @param[in]  n            Number of signals
 *
 * @return FC_OK on success, error code otherwise
 *
 * @note If init_values is NULL, initializes all states to 0.0
 * @note If init_values is provided, copies values to ema_state
 * @note For "no scaling" behavior, initialize with first signal value
 * @note Time complexity: O(n)
 * @note Thread-safe: no shared mutable state
 */
FC_API fc_status_t fc_ex_sig_init_ema_state(double* ema_state, const double* init_values, size_t n);

FC_END_DECLS

#endif /* FC_EX_SIG_POSTPROCESS_H */
