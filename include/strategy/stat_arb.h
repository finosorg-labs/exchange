/**
 * @file stat_arb.h
 * @brief Statistical arbitrage strategy using cointegration and Z-Score
 *
 * Strategy-level statistical arbitrage implementation for pairs trading based on
 * cointegration relationships. Provides batch computation of:
 * - Cointegration beta estimation (Engle-Granger two-step method)
 * - Real-time spread calculation: S = log(Pa) - beta * log(Pb)
 * - Rolling window Z-Score normalization for mean reversion signals
 *
 * This module provides the computational core (C/SIMD) while the Go layer
 * (stat_arb.go) handles two-leg execution orchestration, stop-loss/take-profit,
 * and intraday position clearing.
 *
 * Key formulas:
 *   beta <- OLS regression of log(Pa) on log(Pb) (offline/periodic)
 *   S = log(Pa) - beta * log(Pb)                 (real-time spread)
 *   z = (S - mu) / sigma                          (rolling window Z-Score)
 *
 * Trading logic (implemented in Go):
 *   Entry: |z| > threshold (e.g., 2.0)
 *   Exit: z -> 0 (mean reversion)
 *   Stop-loss: |z| > 4.0 (cointegration breakdown)
 *   Intraday clearing: force close before market close
 */

#ifndef FC_EX_STRAT_STAT_ARB_H
#define FC_EX_STRAT_STAT_ARB_H

#include "error.h"
#include "platform.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Rolling window statistics state for Z-Score computation
 *
 * Maintains a circular buffer of spread values and incrementally computes
 * mean and standard deviation using a fixed-size rolling window.
 * This structure tracks one pair's rolling statistics.
 */
typedef struct {
    double* buffer;     /**< Circular buffer holding window_size spread values */
    size_t window_size; /**< Fixed window size for rolling statistics */
    size_t count;       /**< Number of samples seen (min(total_samples, window_size)) */
    size_t head;        /**< Current write position in circular buffer */
    double sum;         /**< Running sum of values in window (for mean) */
    double sum_sq;      /**< Running sum of squared values (for variance) */
} fc_ex_strat_zscore_state_t;

/**
 * @brief Initialize a Z-Score state structure
 *
 * Allocates and initializes the circular buffer for rolling window statistics.
 * Must be called before using the state in fc_ex_strat_coint_spread_z.
 *
 * @param[out] state Pointer to state structure to initialize
 * @param[in] window_size Rolling window size (must be > 1)
 * @return FC_OK on success, error code otherwise
 *
 * Time complexity: O(window_size)
 * Space complexity: O(window_size)
 * Thread safety: Not thread-safe (caller must synchronize)
 *
 * @note Caller must call fc_ex_strat_zscore_state_free to release memory
 * @note window_size should typically be 20-100 for intraday mean reversion
 */
FC_API fc_status_t
fc_ex_strat_zscore_state_init(fc_ex_strat_zscore_state_t* state, size_t window_size);

/**
 * @brief Free resources allocated by Z-Score state
 *
 * Releases the circular buffer memory. The state structure itself is not freed.
 *
 * @param[in,out] state Pointer to state structure to free
 *
 * Time complexity: O(1)
 * Space complexity: O(1)
 * Thread safety: Not thread-safe (caller must synchronize)
 */
FC_API void fc_ex_strat_zscore_state_free(fc_ex_strat_zscore_state_t* state);

/**
 * @brief Reset Z-Score state to initial conditions
 *
 * Clears the circular buffer and resets statistics without deallocating memory.
 * Useful for starting a new trading session or after detecting cointegration breakdown.
 *
 * @param[in,out] state Pointer to state structure to reset
 *
 * Time complexity: O(window_size)
 * Space complexity: O(1)
 * Thread safety: Not thread-safe (caller must synchronize)
 */
FC_API void fc_ex_strat_zscore_state_reset(fc_ex_strat_zscore_state_t* state);

/**
 * @brief Estimate cointegration beta coefficients for multiple pairs
 *
 * Computes the cointegration coefficient beta for each pair using Engle-Granger
 * two-step method (simplified to OLS regression). This is typically run offline
 * or periodically (e.g., daily) to calibrate the spread relationship.
 *
 * For each pair: beta[i] = argmin ||log_pa[i,:] - beta * log_pb[i,:]||^2
 *
 * Uses QR decomposition via fc_optim_least_squares for numerical stability.
 *
 * @param[out] beta_out Output array of beta coefficients (length n_pairs)
 * @param[in] log_pa Log prices of asset A, shape (n_pairs × window), column-major
 * @param[in] log_pb Log prices of asset B, shape (n_pairs × window), column-major
 * @param[in] n_pairs Number of pairs to estimate
 * @param[in] window Number of historical observations per pair (must be > 1)
 * @return FC_OK on success, error code otherwise
 *
 * Time complexity: O(n_pairs × window)
 * Space complexity: O(n_pairs × window) temporary allocation
 * Thread safety: Thread-safe (no global state)
 *
 * Input validation:
 * - beta_out, log_pa, log_pb must not be NULL
 * - n_pairs must be > 0
 * - window must be > 1
 *
 * Performance notes:
 * - For n_pairs >= 10, uses batch processing to amortize overhead
 * - Each regression uses O(window) time (simple linear regression)
 * - Heap allocation for workspace (consider stack allocation for hot paths)
 *
 * @note If a pair's regression fails (rank deficiency), beta_out[i] = 0.0
 * @note Input log prices should be pre-computed using fc_math_log_f64
 * @note NaN/Inf in inputs will propagate or cause regression failure
 */
FC_API fc_status_t fc_ex_strat_coint_beta(
    double* beta_out,
    const double* log_pa,
    const double* log_pb,
    size_t n_pairs,
    size_t window
);

/**
 * @brief Compute spread and rolling Z-Score for multiple pairs
 *
 * Real-time calculation of cointegration spread and standardized Z-Score
 * for mean reversion signal generation. This is the hot path function called
 * on every price update.
 *
 * For each pair i:
 *   spread[i] = log(pa[i]) - beta[i] * log(pb[i])
 *   z[i] = (spread[i] - mu[i]) / sigma[i]
 *
 * Where mu and sigma are computed from rolling window using z_states.
 *
 * @param[out] spread_out Output array of spread values (length n_pairs)
 * @param[out] z_out Output array of Z-Scores (length n_pairs)
 * @param[in] pa Current prices of asset A (length n_pairs)
 * @param[in] pb Current prices of asset B (length n_pairs)
 * @param[in] beta Cointegration coefficients (length n_pairs)
 * @param[in,out] z_states Array of rolling window states (length n_pairs)
 * @param[in] n_pairs Number of pairs
 * @return FC_OK on success, error code otherwise
 *
 * Time complexity: O(n_pairs)
 * Space complexity: O(1) additional space (updates z_states in-place)
 * Thread safety: Not thread-safe (z_states modified, caller must synchronize)
 *
 * Input validation:
 * - All pointer parameters must not be NULL
 * - n_pairs must be > 0
 * - pa[i], pb[i] must be positive (log domain)
 * - z_states[i] must be initialized via fc_ex_strat_zscore_state_init
 *
 * Performance notes:
 * - Uses SIMD-optimized fc_math_log_f64 for batch logarithm computation
 * - Rolling window statistics updated incrementally (O(1) per pair)
 * - For n_pairs >= 1000, cgo overhead is amortized
 *
 * Special value handling:
 * - If pa[i] or pb[i] <= 0: spread_out[i] = NaN, z_out[i] = NaN
 * - If window not yet full (count < window_size): z_out[i] = 0.0
 * - If sigma[i] == 0: z_out[i] = 0.0 (avoid division by zero)
 *
 * @note Logarithms computed in batch for SIMD efficiency
 * @note Z-Score requires at least 2 samples in window for valid sigma
 * @note First window_size-1 updates will return z=0 until window fills
 */
FC_API fc_status_t fc_ex_strat_coint_spread_z(
    double* spread_out,
    double* z_out,
    const double* pa,
    const double* pb,
    const double* beta,
    fc_ex_strat_zscore_state_t* z_states,
    size_t n_pairs
);

#ifdef __cplusplus
}
#endif

#endif /* FC_EX_STRAT_STAT_ARB_H */
