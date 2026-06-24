/**
 * @file normalize.h
 * @brief Online normalization using Welford incremental Z-Score
 *
 * Provides numerically stable online feature normalization for real-time
 * signal processing. Uses rolling window Welford algorithm to compute
 * z-scores without look-ahead bias, ensuring consistency between live
 * trading and backtesting.
 *
 * Formula: z = (x - μ) / σ
 * - μ and σ are computed incrementally over a rolling window (typically 1000 ticks)
 * - O(1) update complexity per feature
 * - No look-ahead bias (strictly causal)
 *
 * Implementation:
 * - Reuses fc_welford_update, fc_welford_mean, fc_welford_stddev from stats module (M02-09)
 * - SIMD-optimized batch normalization (AVX-512/AVX2/SSE4.2)
 * - Zero allocation on hot paths
 *
 * Use case: Normalize 50-200 dimensional feature vectors for ML inference
 * in HFT signal generation (~100-200ns latency target).
 *
 * Performance: ~100-200ns for batch normalization of multi-dimensional features
 */

#ifndef FC_EX_SIG_NORMALIZE_H
#define FC_EX_SIG_NORMALIZE_H

#include "error.h"
#include "platform.h"

FC_BEGIN_DECLS

/* Include Welford state from stats module (M02-09) */
#include "welford.h"

/**
 * @brief Normalize features to z-scores using online Welford statistics
 *
 * Computes z-score normalization for a batch of feature vectors using
 * maintained Welford states. Each feature dimension has its own independent
 * rolling window state for mean and standard deviation tracking.
 *
 * Formula: z_out[i][j] = (features[i][j] - μ_j) / σ_j
 * where μ_j and σ_j are the running mean and stddev for feature j.
 *
 * The function operates on feature matrices in row-major order:
 * - Each row represents one symbol/instrument
 * - Each column represents one feature dimension
 * - Total elements: n_symbols × n_features
 *
 * Edge cases:
 * - If σ ≤ ε (near-zero or zero stddev): z = 0.0 (avoid division by zero)
 * - If count < 2: z = 0.0 (insufficient data for stddev)
 *
 * @param[out] z_out         Normalized features (n_symbols × n_features), row-major
 * @param[in]  states        Welford states for each feature (n_features)
 * @param[in]  features      Raw feature matrix (n_symbols × n_features), row-major
 * @param[in]  n_symbols     Number of symbols/instruments
 * @param[in]  n_features    Number of feature dimensions
 *
 * @return FC_OK on success, error code otherwise
 *
 * @note All arrays must be 64-byte aligned for optimal SIMD performance
 * @note Uses runtime SIMD dispatch (AVX-512 > AVX2 > SSE4.2 > Scalar)
 * @note Time complexity: O(n_symbols * n_features) with SIMD parallelism
 * @note Thread-safe: reads from states, no modification (normalization only)
 * @note Handles NaN: returns FC_ERR_NAN_INPUT if any feature is NaN
 * @note Does NOT update Welford states (use fc_welford_update_* separately)
 */
FC_API fc_status_t fc_ex_sig_normalize_zscore(
    double* z_out,
    const fc_welford_state_t* states,
    const double* features,
    size_t n_symbols,
    int n_features
);

/**
 * @brief Update Welford states with new feature observations (batch)
 *
 * Updates multiple independent Welford states simultaneously with new
 * feature observations from multiple symbols. This is typically called
 * before normalization to maintain rolling window statistics.
 *
 * For each feature j, updates state[j] with all observations:
 *   features[0][j], features[1][j], ..., features[n_symbols-1][j]
 *
 * This enables maintaining per-feature rolling statistics across multiple
 * instruments, which is then used for z-score normalization.
 *
 * Implementation: Calls fc_welford_update from stats module (M02-09) for each value.
 *
 * @param[in,out] states      Welford states for each feature (n_features)
 * @param[in]     features    Feature matrix (n_symbols × n_features), row-major
 * @param[in]     n_symbols   Number of symbols/instruments
 * @param[in]     n_features  Number of feature dimensions
 *
 * @return FC_OK on success, error code otherwise
 *
 * @note All arrays must be properly aligned
 * @note Time complexity: O(n_symbols * n_features)
 * @note Thread-safe: each state should be accessed by a single thread
 * @note Handles NaN: returns FC_ERR_NAN_INPUT if any feature is NaN (via fc_welford_update)
 * @note For rolling window behavior, combine with window management externally
 */
FC_API fc_status_t fc_ex_sig_normalize_update_states(
    fc_welford_state_t* states,
    const double* features,
    size_t n_symbols,
    int n_features
);

FC_END_DECLS

#endif /* FC_EX_SIG_NORMALIZE_H */
