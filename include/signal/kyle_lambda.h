/**
 * @file kyle_lambda.h
 * @brief Kyle's Lambda price impact coefficient computation
 *
 * Provides high-performance computation of Kyle's Lambda (λ), which measures
 * the price impact per unit of volume. This coefficient estimates the market's
 * liquidity and is used for execution cost evaluation and dynamic position sizing.
 *
 * Mathematical definition:
 *   ΔP = λ · ΔV + ε
 *   λ̂  = Cov(ΔP, V) / Var(V)
 *
 * Where:
 *   ΔP = price change
 *   V  = volume (signed volume or trade direction)
 *   λ  = price impact coefficient (higher = less liquid = higher execution cost)
 *
 * Use cases:
 * - Execution cost estimation
 * - Dynamic position sizing (Kelly criterion)
 * - Market impact modeling
 * - Liquidity assessment
 *
 * Typical parameters:
 * - Window size: 100-1000 ticks for rolling estimation
 * - Update frequency: Per tick or batched
 * - Expected latency: ~1-5μs per symbol
 *
 * Key features:
 * - Batch processing for multiple symbols
 * - SIMD optimization (AVX-512, AVX2, SSE4.2)
 * - Numerically stable computation
 * - Zero heap allocation on hot path
 * - Thread-safe (no global state)
 */

#ifndef FC_EX_SIG_KYLE_LAMBDA_H
#define FC_EX_SIG_KYLE_LAMBDA_H

#include "error.h"
#include "platform.h"

FC_BEGIN_DECLS

/**
 * @brief Compute Kyle's Lambda for multiple symbols using rolling windows
 *
 * Computes λ̂ = Cov(ΔP, V) / Var(V) for each symbol using a rolling window
 * of price changes and volumes.
 *
 * Input data layout:
 * - dprice: [n_symbols × window] - price changes for each symbol
 *           dprice[i * window + j] = ΔP for symbol i at tick j
 * - volume: [n_symbols × window] - volumes for each symbol
 *           volume[i * window + j] = V for symbol i at tick j
 *
 * Output:
 * - lambda_out[i] = Kyle's Lambda for symbol i
 *
 * Time complexity: O(n_symbols × window)
 * Space complexity: O(1) - no heap allocation
 *
 * @param lambda_out Output array of λ values (must not be NULL, size: n_symbols)
 * @param dprice Price change sequences (must not be NULL, size: n_symbols × window)
 * @param volume Volume sequences (must not be NULL, size: n_symbols × window)
 * @param n_symbols Number of symbols to process (must be > 0)
 * @param window Rolling window size in ticks (must be >= 2)
 *
 * @return FC_OK on success, error code on failure
 *         FC_ERR_INVALID_ARG - if any pointer is NULL or dimensions invalid
 *         FC_ERR_NAN_INPUT - if any element is NaN
 *         FC_ERR_NUMERICAL - if volume variance is zero (no variation in volume)
 *
 * @note Thread-safe
 * @note Returns 0.0 for symbols where volume variance is zero
 * @note Handles division by zero gracefully
 * @note For optimal performance, ensure:
 *       - Arrays are 64-byte aligned for AVX-512
 *       - n_symbols >= 8 for effective SIMD utilization
 *       - window >= 100 for statistical reliability
 */
FC_API fc_status_t fc_ex_sig_kyle_lambda_batch(
    double* lambda_out,
    const double* dprice,
    const double* volume,
    size_t n_symbols,
    size_t window
);

FC_END_DECLS

#endif /* FC_EX_SIG_KYLE_LAMBDA_H */
