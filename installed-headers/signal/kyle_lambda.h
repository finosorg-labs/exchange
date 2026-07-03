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
 * @brief Compute Kyle's Lambda for multiple symbols using covariance method
 *
 * Fast computation using λ̂ = Cov(ΔP, V) / Var(V) via covariance matrix.
 * This is the most efficient method when only λ is needed.
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
 * Space complexity: O(window) - allocates temporary workspace internally
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
 *         FC_ERR_OUT_OF_MEMORY - if workspace allocation fails
 *
 * @note Thread-safe
 * @note Returns 0.0 for symbols where volume variance is zero
 * @note This is the recommended method for real-time trading (lowest latency)
 * @note For zero-allocation hot path, use fc_ex_sig_kyle_lambda_batch_ext()
 */
FC_API fc_status_t fc_ex_sig_kyle_lambda_batch(
    double* lambda_out,
    const double* dprice,
    const double* volume,
    size_t n_symbols,
    size_t window
);

/**
 * @brief Extended Kyle's Lambda computation with validity flags and workspace
 *
 * Extended version that provides:
 * 1. Validity flags to distinguish computation failures from true zero values
 * 2. Caller-provided workspace for zero-allocation hot path
 *
 * This is the recommended API for production HFT systems where allocation
 * overhead is critical and error visibility is required.
 *
 * @param lambda_out Output array of λ values (must not be NULL, size: n_symbols)
 * @param valid_flags Output validity flags (can be NULL, size: n_symbols)
 *                    valid_flags[i] = false if computation failed (zero variance, etc.)
 *                    If NULL, validity information is not returned
 * @param dprice Price change sequences (must not be NULL, size: n_symbols × window)
 * @param volume Volume sequences (must not be NULL, size: n_symbols × window)
 * @param n_symbols Number of symbols to process (must be > 0)
 * @param window Rolling window size in ticks (must be >= 2)
 * @param workspace Temporary workspace (can be NULL for internal allocation)
 *                  If non-NULL, must have size >= fc_ex_sig_kyle_lambda_workspace_size(window)
 *                  Provides zero-allocation hot path when supplied
 * @param workspace_size Size of workspace in bytes (ignored if workspace is NULL)
 *
 * @return FC_OK on success, error code on failure
 *         FC_ERR_INVALID_ARG - if required pointers are NULL or dimensions invalid
 *         FC_ERR_NAN_INPUT - if any element is NaN
 *         FC_ERR_OUT_OF_MEMORY - if workspace is NULL and internal allocation fails
 *         FC_ERR_WORKSPACE_TOO_SMALL - if workspace_size is too small
 *
 * @note Thread-safe (requires separate workspace per thread if supplied)
 * @note When valid_flags[i] = false, lambda_out[i] is set to 0.0
 * @note Workspace can be reused across calls for same window size
 *
 * Example (zero-allocation):
 * @code
 *   size_t ws_size = fc_ex_sig_kyle_lambda_workspace_size(window);
 *   double* ws = aligned_alloc(64, ws_size);
 *   bool valid[n_symbols];
 *   fc_ex_sig_kyle_lambda_batch_ext(lambda, valid, dprice, volume,
 *                                     n_symbols, window, ws, ws_size);
 *   // Check valid[i] before using lambda[i]
 * @endcode
 */
FC_API fc_status_t fc_ex_sig_kyle_lambda_batch_ext(
    double* lambda_out,
    bool* valid_flags,
    const double* dprice,
    const double* volume,
    size_t n_symbols,
    size_t window,
    double* workspace,
    size_t workspace_size
);

/**
 * @brief Calculate required workspace size for Kyle's Lambda computation
 *
 * Returns the minimum workspace size in bytes needed for the extended API.
 *
 * @param window Rolling window size in ticks
 * @return Required workspace size in bytes
 *
 * @note Result is constant for a given window size
 * @note Workspace should be 64-byte aligned for SIMD optimization
 */
FC_API size_t fc_ex_sig_kyle_lambda_workspace_size(size_t window);

/**
 * @brief Compute Kyle's Lambda with full regression statistics using OLS
 *
 * Extended computation using ordinary least squares regression (optim module).
 * Provides λ plus additional statistics: R², residuals, standard error.
 * Use when regression diagnostics are needed (e.g., model validation, backtesting).
 *
 * Solves: ΔP = α + λ·V + ε via QR decomposition (regression with intercept).
 *
 * This method fits a linear model with an intercept term, where the slope coefficient λ
 * is mathematically equivalent to Cov(ΔP, V) / Var(V) computed by the covariance method.
 * The λ estimates should match the covariance method within numerical precision.
 *
 * Time complexity: O(n_symbols × window²)
 * Space complexity: O(window) - allocates workspace internally
 *
 * @param lambda_out Output array of λ values (must not be NULL, size: n_symbols)
 * @param r_squared Output array of R² values (can be NULL, size: n_symbols)
 * @param std_error Output array of residual standard errors (can be NULL, size: n_symbols)
 * @param residuals Output residuals (can be NULL, size: n_symbols × window)
 *                  residuals[i * window + j] = residual for symbol i at tick j
 * @param dprice Price change sequences (must not be NULL, size: n_symbols × window)
 * @param volume Volume sequences (must not be NULL, size: n_symbols × window)
 * @param n_symbols Number of symbols to process (must be > 0)
 * @param window Rolling window size in ticks (must be >= 2)
 *
 * @return FC_OK on success, error code on failure
 *         FC_ERR_INVALID_ARG - if lambda_out/dprice/volume is NULL or dimensions invalid
 *         FC_ERR_NAN_INPUT - if any input element is NaN
 *         FC_ERR_OUT_OF_MEMORY - if workspace allocation fails
 *
 * @note Thread-safe
 * @note Slower than fc_ex_sig_kyle_lambda_batch() due to full regression computation
 * @note R² indicates model fit quality (0-1, higher is better)
 * @note std_error measures residual spread
 * @note Use this for offline analysis; use fast covariance method for real-time trading
 * @note For zero-allocation hot path, use fc_ex_sig_kyle_lambda_ols_ext()
 */
FC_API fc_status_t fc_ex_sig_kyle_lambda_ols(
    double* lambda_out,
    double* r_squared,
    double* std_error,
    double* residuals,
    const double* dprice,
    const double* volume,
    size_t n_symbols,
    size_t window
);

/**
 * @brief Extended OLS Kyle's Lambda with validity flags and workspace
 *
 * Extended version of OLS method that provides:
 * 1. Validity flags to identify regression failures
 * 2. Caller-provided workspace for zero-allocation hot path
 *
 * @param lambda_out Output array of λ values (must not be NULL, size: n_symbols)
 * @param valid_flags Output validity flags (can be NULL, size: n_symbols)
 *                    valid_flags[i] = false if regression failed
 * @param r_squared Output array of R² values (can be NULL, size: n_symbols)
 * @param std_error Output array of residual standard errors (can be NULL, size: n_symbols)
 * @param residuals Output residuals (can be NULL, size: n_symbols × window)
 * @param dprice Price change sequences (must not be NULL, size: n_symbols × window)
 * @param volume Volume sequences (must not be NULL, size: n_symbols × window)
 * @param n_symbols Number of symbols to process (must be > 0)
 * @param window Rolling window size in ticks (must be >= 2)
 * @param workspace Temporary workspace (can be NULL for internal allocation)
 *                  If non-NULL, must have size >= fc_ex_sig_kyle_lambda_ols_workspace_size(window)
 * @param workspace_size Size of workspace in bytes (ignored if workspace is NULL)
 *
 * @return FC_OK on success, error code on failure
 *         FC_ERR_INVALID_ARG - if required pointers are NULL or dimensions invalid
 *         FC_ERR_NAN_INPUT - if any element is NaN
 *         FC_ERR_OUT_OF_MEMORY - if workspace is NULL and internal allocation fails
 *         FC_ERR_WORKSPACE_TOO_SMALL - if workspace_size is too small
 *
 * @note Thread-safe (requires separate workspace per thread if supplied)
 * @note When valid_flags[i] = false, outputs for symbol i are set to zero
 */
FC_API fc_status_t fc_ex_sig_kyle_lambda_ols_ext(
    double* lambda_out,
    bool* valid_flags,
    double* r_squared,
    double* std_error,
    double* residuals,
    const double* dprice,
    const double* volume,
    size_t n_symbols,
    size_t window,
    double* workspace,
    size_t workspace_size
);

/**
 * @brief Calculate required workspace size for OLS Kyle's Lambda computation
 *
 * @param window Rolling window size in ticks
 * @return Required workspace size in bytes
 */
FC_API size_t fc_ex_sig_kyle_lambda_ols_workspace_size(size_t window);

FC_END_DECLS

#endif /* FC_EX_SIG_KYLE_LAMBDA_H */
