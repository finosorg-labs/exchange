/**
 * @file var_parametric.h
 * @brief Parametric VaR/CVaR calculation using normal distribution assumption
 *
 * Implements Value at Risk (VaR) and Conditional Value at Risk (CVaR) using
 * the parametric method, which assumes returns follow a normal distribution.
 * This method uses analytical formulas based on mean and standard deviation.
 */

#ifndef FC_OPTIM_VAR_PARAMETRIC_H
#define FC_OPTIM_VAR_PARAMETRIC_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Calculate parametric VaR/CVaR for a single portfolio
 *
 * Computes VaR and CVaR assuming portfolio returns follow a normal distribution.
 * Uses analytical formulas:
 * - VaR_α = μ - σ·Φ^(-1)(1-α)
 * - CVaR_α = μ - σ·φ(Φ^(-1)(1-α))/(1-α)
 *
 * Where:
 * - μ is the portfolio mean return
 * - σ is the portfolio standard deviation
 * - Φ^(-1) is the inverse normal CDF
 * - φ is the normal PDF
 * - α is the confidence level (e.g., 0.95 for 95% confidence)
 *
 * @param mean Portfolio mean return
 * @param stddev Portfolio standard deviation (must be > 0)
 * @param confidence Confidence level (must be in (0, 1), typically 0.95 or 0.99)
 * @param var Output: Value at Risk (negative value indicates loss)
 * @param cvar Output: Conditional Value at Risk (negative value indicates loss)
 * @return 0 on success
 *         -1 if var or cvar is NULL
 *         -2 if stddev <= 0
 *         -3 if confidence is not in (0, 1)
 *
 * Time complexity: O(1)
 * Space complexity: O(1)
 * Thread safety: Thread-safe
 *
 * @note VaR and CVaR are reported as returns (not losses), so negative values
 *       indicate potential losses
 * @note Precision: Relative error < 1e-10 compared to analytical solution
 */
int fc_optim_var_parametric(
    double mean,
    double stddev,
    double confidence,
    double* var,
    double* cvar
);

/**
 * @brief Calculate parametric VaR/CVaR for multiple portfolios
 *
 * Batch version that computes VaR/CVaR for n portfolios in parallel,
 * leveraging SIMD optimizations in the normal inverse CDF calculations.
 *
 * @param n Number of portfolios (must be > 0)
 * @param means Portfolio mean returns (n elements, can contain any finite values)
 * @param stddevs Portfolio standard deviations (n elements, all must be > 0)
 * @param confidence Confidence level (must be in (0, 1))
 * @param var Output: VaR values (n elements, pre-allocated by caller)
 * @param cvar Output: CVaR values (n elements, pre-allocated by caller)
 * @return 0 on success
 *         -1 if any required pointer is NULL
 *         -2 if n == 0
 *         -3 if any stddev <= 0 or confidence not in (0, 1)
 *
 * Time complexity: O(n)
 * Space complexity: O(n) temporary space
 * Thread safety: Thread-safe if different output buffers are used
 *
 * @note Performance target: 500 portfolios in < 5ms on modern x86_64 with AVX2
 * @note Uses SIMD-optimized normal inverse CDF from math module
 */
int fc_optim_var_parametric_batch(
    size_t n,
    const double* means,
    const double* stddevs,
    double confidence,
    double* var,
    double* cvar
);

/**
 * @brief Calculate parametric VaR/CVaR from historical returns data
 *
 * Convenience function that computes mean and standard deviation from historical
 * returns, then applies the parametric method. Assumes returns are normally
 * distributed.
 *
 * @param returns Historical returns data (n_periods elements)
 * @param n_periods Number of historical periods (must be >= 2)
 * @param confidence Confidence level (must be in (0, 1))
 * @param var Output: VaR
 * @param cvar Output: CVaR
 * @return 0 on success
 *         -1 if any required pointer is NULL
 *         -2 if n_periods < 2
 *         -3 if confidence not in (0, 1)
 *         -4 if memory allocation failed
 *
 * Time complexity: O(n_periods)
 * Space complexity: O(1)
 * Thread safety: Thread-safe
 *
 * @note Uses sample standard deviation (divides by n-1)
 * @note Internally calls fc_stats_mean_variance_f64() from stats module
 */
int fc_optim_var_parametric_from_returns(
    const double* returns,
    size_t n_periods,
    double confidence,
    double* var,
    double* cvar
);

/**
 * @brief Calculate parametric VaR/CVaR for portfolio from multi-asset returns
 *
 * Computes portfolio statistics from multi-asset historical returns, then
 * applies parametric method. This is the most complete convenience function
 * that handles the full workflow from raw data to VaR/CVaR.
 *
 * Workflow:
 * 1. Compute mean return for each asset using stats module
 * 2. Compute covariance matrix using stats module
 * 3. Calculate portfolio mean: μ_p = w^T μ
 * 4. Calculate portfolio variance: σ²_p = w^T Σ w (using GEMV from matrix module)
 * 5. Apply parametric formulas
 *
 * @param returns Historical returns matrix (dim × n_periods, row-major)
 * @param weights Portfolio weights (dim elements, should sum to 1.0)
 * @param dim Number of assets (must be > 0)
 * @param n_periods Number of historical periods (must be >= dim + 1)
 * @param confidence Confidence level (must be in (0, 1))
 * @param var Output: VaR
 * @param cvar Output: CVaR
 * @return 0 on success
 *         -1 if any required pointer is NULL
 *         -2 if dim == 0 or n_periods insufficient
 *         -3 if confidence not in (0, 1)
 *         -4 if memory allocation failed
 *
 * Time complexity: O(dim² × n_periods + dim²)
 * Space complexity: O(dim²) for covariance matrix
 * Thread safety: Thread-safe
 *
 * @note Requires sufficient historical data: n_periods >= dim + 1
 * @note Uses sample covariance (divides by n-1)
 * @note Portfolio variance must be positive (guaranteed if covariance is valid)
 */
int fc_optim_var_parametric_from_portfolio_returns(
    const double* returns,
    const double* weights,
    size_t dim,
    size_t n_periods,
    double confidence,
    double* var,
    double* cvar
);

#ifdef __cplusplus
}
#endif

#endif /* FC_OPTIM_VAR_PARAMETRIC_H */
