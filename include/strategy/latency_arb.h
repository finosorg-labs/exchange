/**
 * @file latency_arb.h
 * @brief Latency arbitrage strategy using linear prediction and deviation detection
 *
 * Strategy-level latency arbitrage implementation for cross-venue speed-based trading.
 * Provides batch computation of:
 * - Linear prediction coefficients (P_exp = a * P_fast + b) via OLS regression
 * - Deviation-based signal detection (|P_slow - P_exp| > threshold)
 * - Batch signal evaluation for offline backtesting and parameter tuning
 *
 * This module provides the computational core (C/SIMD) for offline/periodic calibration
 * and backtesting. The real-time hot path (tick-by-tick prediction + hit detection + order
 * submission <200ns) is implemented in FPGA hardware to bypass software stack overhead.
 *
 * Key formulas:
 *   Calibration (offline):  P_slow = a * P_fast + b + epsilon
 *                          (a, b) <- OLS regression on historical prices
 *
 *   Prediction (runtime):   P_exp = a * P_fast + b
 *
 *   Signal generation:      signal = 1 if |P_slow - P_exp| > theta, else 0
 *                          deviation = P_slow - P_exp
 *
 * Trading logic (implemented in FPGA for hot path, Go for lifecycle):
 *   - Fast market price update triggers prediction
 *   - Compare predicted slow market price with actual slow market price
 *   - If deviation exceeds threshold, submit order to slow market (<200ns)
 *   - Monitor fill confirmations and cancel on timeout (Go/OMS layer)
 *   - Clear positions to prevent directional accumulation (Go layer)
 */

#ifndef FC_EX_STRAT_LATENCY_ARB_H
#define FC_EX_STRAT_LATENCY_ARB_H

#include "error.h"
#include "platform.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Calibrate linear prediction coefficients for latency arbitrage pairs
 *
 * Computes the linear prediction model for each pair using OLS regression:
 *   P_slow = a * P_fast + b + epsilon
 *
 * This is typically run offline or periodically (e.g., daily) to calibrate
 * the relationship between fast and slow markets. The resulting coefficients
 * are then loaded into FPGA for real-time prediction.
 *
 * For each pair: (a[i], b[i]) = argmin ||hist_slow[i,:] - a*hist_fast[i,:] - b||^2
 *
 * Uses OLS regression via fc_optim_least_squares for numerical stability.
 *
 * @param[out] coef_a_out Output array of slope coefficients a (length n_pairs)
 * @param[out] coef_b_out Output array of intercept coefficients b (length n_pairs)
 * @param[in] hist_fast Fast market price history, shape (n_pairs × window), row-major
 * @param[in] hist_slow Slow market price history, shape (n_pairs × window), row-major
 * @param[in] n_pairs Number of cross-venue pairs to calibrate
 * @param[in] window Number of historical observations per pair (must be > 1)
 * @return FC_OK on success, error code otherwise
 *
 * Time complexity: O(n_pairs × window)
 * Space complexity: O(n_pairs × window) temporary allocation
 * Thread safety: Thread-safe (no global state)
 *
 * Input validation:
 * - coef_a_out, coef_b_out, hist_fast, hist_slow must not be NULL
 * - n_pairs must be > 0
 * - window must be > 1
 *
 * Performance notes:
 * - For n_pairs >= 10, uses batch processing to amortize overhead
 * - Each regression uses O(window) time (simple linear regression)
 * - Heap allocation for workspace (consider stack allocation for hot paths)
 *
 * @note If a pair's regression fails (rank deficiency), coef_a_out[i] = 0.0, coef_b_out[i] = 0.0
 * @note Input prices should be raw prices (not log-transformed)
 * @note NaN/Inf in inputs will propagate or cause regression failure
 * @note Coefficients are loaded into FPGA; software layer does not use them in hot path
 */
FC_API fc_status_t fc_ex_strat_latarb_calibrate(
    double* coef_a_out,
    double* coef_b_out,
    const double* hist_fast,
    const double* hist_slow,
    size_t n_pairs,
    size_t window
);

/**
 * @brief Generate latency arbitrage signals from price deviations (batch evaluation)
 *
 * Batch evaluation of latency arbitrage signals for offline backtesting and
 * parameter tuning. Computes predicted slow market prices and detects deviations
 * exceeding thresholds.
 *
 * For each pair i:
 *   P_exp[i] = coef_a[i] * price_fast[i] + coef_b[i]
 *   deviation[i] = price_slow[i] - P_exp[i]
 *   hit[i] = 1 if |deviation[i]| > theta[i], else 0
 *
 * This function is NOT used in the real-time hot path (FPGA handles that).
 * It is used for:
 * - Offline backtesting of calibrated coefficients
 * - Parameter sensitivity analysis (varying theta)
 * - Software fallback/monitoring (non-latency-sensitive)
 *
 * @param[out] dev_out Output array of deviations P_slow - P_exp (length n)
 * @param[out] hit_out Output array of hit indicators (1 if |dev|>theta, 0 otherwise) (length n)
 * @param[in] price_fast Current fast market prices (length n)
 * @param[in] price_slow Current slow market prices (length n)
 * @param[in] coef_a Slope coefficients a (length n)
 * @param[in] coef_b Intercept coefficients b (length n)
 * @param[in] theta Deviation thresholds (length n)
 * @param[in] n Number of pairs
 * @return FC_OK on success, error code otherwise
 *
 * Time complexity: O(n)
 * Space complexity: O(1) additional space
 * Thread safety: Thread-safe (no shared state)
 *
 * Input validation:
 * - All pointer parameters must not be NULL
 * - n must be > 0
 * - theta[i] must be non-negative for all i
 *
 * Performance notes:
 * - Uses SIMD-optimized multiply-add for batch prediction
 * - Uses SIMD-optimized absolute value and comparison for hit detection
 * - For n >= 1000, cgo overhead is amortized
 *
 * Special value handling:
 * - If price_fast[i] or price_slow[i] is NaN: dev_out[i] = NaN, hit_out[i] = 0
 * - If coef_a[i] or coef_b[i] is NaN: dev_out[i] = NaN, hit_out[i] = 0
 * - If theta[i] <= 0: hit_out[i] = 0 (no signal generation)
 *
 * @note Real-time signal generation happens in FPGA, not via this function
 * @note Use this for backtesting, parameter tuning, and monitoring only
 * @note For production hot path, coefficients are loaded into FPGA at startup
 */
FC_API fc_status_t fc_ex_strat_latarb_signal(
    double* dev_out,
    int* hit_out,
    const double* price_fast,
    const double* price_slow,
    const double* coef_a,
    const double* coef_b,
    const double* theta,
    size_t n
);

#ifdef __cplusplus
}
#endif

#endif /* FC_EX_STRAT_LATENCY_ARB_H */
