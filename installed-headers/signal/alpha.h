/**
 * @file alpha.h
 * @brief Alpha factor aggregation and dynamic weighting
 *
 * Provides batch operations for combining multiple signals and ML outputs into
 * final Alpha factors with confidence scoring and dynamic weighting.
 *
 * Core operations:
 * - Multi-signal weighted aggregation
 * - Confidence scoring based on signal agreement
 * - Weight normalization and dynamic reweighting
 * - Signal quality assessment
 *
 * All operations are batched for performance and SIMD optimization.
 */

#ifndef FC_EX_SIG_ALPHA_H
#define FC_EX_SIG_ALPHA_H

#include "error.h"
#include "platform.h"

FC_BEGIN_DECLS

/**
 * @brief Alpha aggregation configuration
 *
 * Controls how multiple signals are combined into final Alpha factors.
 */
typedef struct {
    int normalize_weights;  /**< If true, normalize weights to sum to 1.0 */
    int per_symbol_weights; /**< If true, weights are (n_symbols × n_signals), else (n_signals) */
    double min_confidence;  /**< Minimum confidence threshold (0.0-1.0), signals below are zeroed */
    double strength_scale;  /**< Strength scaling factor for normalization (default 1.0).
                                 Used to adjust for different signal magnitudes:
                                 - 1.0: signals pre-normalized to [-1, 1]
                                 - 100.0: large signals like OFI integrals
                                 - 3.0: z-score signals (typically [-3, 3])
                                 Confidence uses tanh(avg_strength / strength_scale) */
} fc_ex_alpha_cfg_t;

/**
 * @brief Aggregate multiple signals into Alpha factors with confidence scoring
 *
 * Combines multiple signal streams using weighted sum to produce final Alpha factors.
 * Simultaneously computes confidence scores based on signal agreement/disagreement.
 *
 * Aggregation formula:
 *   alpha[i] = Σ(weights[j] * signals[i][j]) for j in [0, n_signals)
 *
 * Confidence scoring:
 *   - Measures signal alignment (all positive/negative vs. mixed)
 *   - Higher when signals agree on direction
 *   - Range: [0.0, 1.0], where 1.0 = perfect agreement
 *
 * Weight modes:
 *   - Uniform weights: weights = (n_signals), applied to all symbols
 *   - Per-symbol weights: weights = (n_symbols × n_signals), custom per symbol
 *
 * @param[out] alpha_out      Alpha factors (n_symbols)
 * @param[out] confidence_out Confidence scores (n_symbols), range [0.0, 1.0]
 * @param[in]  signals        Signal matrix (n_symbols × n_signals), row-major
 * @param[in]  weights        Signal weights (n_signals) or (n_symbols × n_signals)
 * @param[in]  cfg            Aggregation configuration
 * @param[in]  n_symbols      Number of symbols
 * @param[in]  n_signals      Number of signals per symbol
 *
 * @return FC_OK on success, error code otherwise
 *
 * @note cfg.strength_scale adjusts confidence for different signal magnitudes
 * @note Confidence strength_factor = tanh(avg_signal_strength / cfg.strength_scale)
 * @note Default strength_scale=1.0 assumes signals in [-1, 1] range
 * @note All arrays must be 64-byte aligned for optimal SIMD performance
 * @note signals layout: signals[symbol_idx * n_signals + signal_idx]
 * @note If cfg.per_symbol_weights = false: weights[signal_idx]
 * @note If cfg.per_symbol_weights = true: weights[symbol_idx * n_signals + signal_idx]
 * @note If cfg.normalize_weights = true: weights normalized to sum to 1.0 per symbol
 * @note NaN/Inf handling: signals with NaN/Inf contribute 0 to weighted sum and reduce confidence
 * @note Confidence = 0 when all signals are NaN/Inf or conflicting
 * @note If confidence < cfg.min_confidence, alpha is set to 0.0
 * @note Uses runtime SIMD dispatch (AVX-512 > AVX2 > SSE4.2 > Scalar)
 * @note Time complexity: O(n_symbols * n_signals) with SIMD parallelism
 * @note Thread-safe: no shared mutable state
 */
FC_API fc_status_t fc_ex_sig_alpha_aggregate(
    double* alpha_out,
    double* confidence_out,
    const double* signals,
    const double* weights,
    const fc_ex_alpha_cfg_t* cfg,
    size_t n_symbols,
    int n_signals
);

/**
 * @brief Normalize signal weights to sum to 1.0
 *
 * Ensures weights sum to 1.0 for proper Alpha scaling. Can operate in uniform
 * or per-symbol mode.
 *
 * Normalization:
 *   weights_norm[i] = weights[i] / Σ(weights)
 *
 * @param[out] weights_out    Normalized weights (n_weights)
 * @param[in]  weights_in     Input weights (n_weights)
 * @param[in]  n_symbols      Number of symbols (1 for uniform weights)
 * @param[in]  n_signals      Number of signals
 *
 * @return FC_OK on success, error code otherwise
 *
 * @note For uniform weights: n_symbols = 1, n_weights = n_signals
 * @note For per-symbol weights: n_weights = n_symbols * n_signals
 * @note If sum of weights is zero or near-zero, returns FC_ERR_INVALID_ARG
 * @note Can be used in-place by passing same pointer for weights_out and weights_in
 * @note Uses runtime SIMD dispatch for performance
 * @note Time complexity: O(n_weights) with SIMD parallelism
 * @note Thread-safe: no shared mutable state
 */
FC_API fc_status_t fc_ex_sig_normalize_weights(
    double* weights_out,
    const double* weights_in,
    size_t n_symbols,
    int n_signals
);

/**
 * @brief Compute signal agreement score (Sharpe-like metric for signal quality)
 *
 * Measures how well signals align in direction and magnitude. Higher scores
 * indicate stronger agreement across signals.
 *
 * Agreement score:
 *   - Positive when most signals agree on direction
 *   - Magnitude increases with stronger individual signals
 *   - Range: typically [-1.0, 1.0] for normalized signals
 *
 * @param[out] agreement_out  Agreement scores (n_symbols)
 * @param[in]  signals        Signal matrix (n_symbols × n_signals)
 * @param[in]  n_symbols      Number of symbols
 * @param[in]  n_signals      Number of signals
 *
 * @return FC_OK on success, error code otherwise
 *
 * @note Agreement = mean(signals) / (std(signals) + epsilon)
 * @note Similar to Sharpe ratio: reward consistency, penalize volatility
 * @note High positive = strong bullish consensus
 * @note High negative = strong bearish consensus
 * @note Near zero = conflicting signals or no consensus
 * @note Uses runtime SIMD dispatch for performance
 * @note Time complexity: O(n_symbols * n_signals)
 * @note Thread-safe: no shared mutable state
 */
FC_API fc_status_t fc_ex_sig_compute_agreement(
    double* agreement_out,
    const double* signals,
    size_t n_symbols,
    int n_signals
);

/**
 * @brief Apply inverse volatility weighting to signals
 *
 * Weights signals by inverse of their recent volatility (standard deviation).
 * Less volatile signals receive higher weights under the assumption that
 * they are more stable/reliable.
 *
 * Weight formula:
 *   w[j] = (1 / std[j]) / Σ(1 / std[k])
 *
 * @param[out] weights_out    Inverse volatility weights (n_signals)
 * @param[in]  signals_hist   Historical signals (window_size × n_signals)
 * @param[in]  work_buffer    Work buffer for temporary data (n_signals doubles)
 * @param[in]  window_size    Length of historical window
 * @param[in]  n_signals      Number of signals
 *
 * @return FC_OK on success, error code otherwise
 *
 * @note signals_hist layout: signals_hist[time_idx * n_signals + signal_idx]
 * @note Computes std deviation for each signal column across time window
 * @note If std is zero or near-zero, signal receives minimum weight
 * @note Output weights are normalized to sum to 1.0
 * @note work_buffer must have space for at least n_signals doubles
 * @note Time complexity: O(window_size * n_signals)
 * @note Thread-safe: no shared mutable state
 */
FC_API fc_status_t fc_ex_sig_inverse_vol_weights(
    double* weights_out,
    const double* signals_hist,
    double* work_buffer,
    size_t window_size,
    int n_signals
);

FC_END_DECLS

#endif /* FC_EX_SIG_ALPHA_H */
