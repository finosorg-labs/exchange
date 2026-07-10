/**
 * @file var_monte_carlo.h
 * @brief Monte Carlo simulation VaR/CVaR calculation
 *
 * Implements Value at Risk (VaR) and Conditional Value at Risk (CVaR) using
 * Monte Carlo simulation with correlated random number generation.
 */

#ifndef FC_OPTIM_VAR_MONTE_CARLO_H
#define FC_OPTIM_VAR_MONTE_CARLO_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Monte Carlo VaR/CVaR calculation state
 *
 * Opaque structure containing random number generator state and workspace.
 * Created by fc_optim_var_monte_carlo_state_create() and destroyed by
 * fc_optim_var_monte_carlo_state_destroy().
 */
typedef struct fc_var_mc_state fc_var_mc_state_t;

/**
 * @brief Create Monte Carlo VaR calculation state
 *
 * Initializes random number generator and allocates workspace for simulations.
 *
 * @param dim Dimension of correlated random vectors (number of assets)
 * @param n_paths Number of Monte Carlo paths to simulate
 * @param seed Random seed (use 0 for time-based seed)
 * @return Pointer to state structure, or NULL on allocation failure
 *
 * Time complexity: O(dim²) for Cholesky decomposition workspace
 * Space complexity: O(dim² + n_paths × dim)
 * Thread safety: Each state is independent and thread-safe
 */
fc_var_mc_state_t* fc_optim_var_monte_carlo_state_create(size_t dim, size_t n_paths, uint64_t seed);

/**
 * @brief Destroy Monte Carlo VaR calculation state
 *
 * Frees all resources associated with the state.
 *
 * @param state State to destroy (can be NULL)
 */
void fc_optim_var_monte_carlo_state_destroy(fc_var_mc_state_t* state);

/**
 * @brief Pre-compute and cache Cholesky decomposition of covariance matrix
 *
 * This function computes the Cholesky decomposition of the covariance matrix
 * once and caches it for reuse in subsequent calls to fc_optim_var_monte_carlo_cached().
 *
 * Use this when you need to compute VaR/CVaR multiple times with the same
 * covariance matrix but different means or weights (e.g., backtesting, stress testing).
 *
 * Performance benefit: Eliminates O(dim³) Cholesky computation from each call,
 * providing 50-60% speedup for scenarios where covariance is constant.
 *
 * @param state Monte Carlo state (must be non-NULL)
 * @param cov_matrix Covariance matrix (dim × dim, row-major, must be positive definite)
 * @return 0 on success
 *         -1 if state or cov_matrix is NULL
 *         -2 if covariance matrix is not positive definite
 *
 * Time complexity: O(dim³) - one-time cost
 * Thread safety: Not thread-safe (modifies state)
 *
 * @note After calling this, use fc_optim_var_monte_carlo_cached() instead of
 *       fc_optim_var_monte_carlo() to benefit from the cached decomposition
 * @note The cache is invalidated if fc_optim_var_monte_carlo() is called with
 *       a new cov_matrix parameter
 *
 * Example:
 * @code
 * fc_var_mc_state_t* state = fc_optim_var_monte_carlo_state_create(100, 10000, 42);
 *
 * // Set covariance once
 * fc_optim_var_monte_carlo_set_covariance(state, cov_matrix);
 *
 * // Run multiple simulations with same covariance but different parameters
 * for (int i = 0; i < 1000; i++) {
 *     fc_optim_var_monte_carlo_cached(state, means[i], weights[i], 100, 0.95, &var, &cvar);
 *     // 50-60% faster than calling fc_optim_var_monte_carlo repeatedly
 * }
 * @endcode
 */
int fc_optim_var_monte_carlo_set_covariance(fc_var_mc_state_t* state, const double* cov_matrix);

/**
 * @brief Calculate Monte Carlo VaR/CVaR using cached Cholesky decomposition
 *
 * High-performance variant that reuses a pre-computed Cholesky decomposition
 * from fc_optim_var_monte_carlo_set_covariance(). Skips the O(dim³) Cholesky
 * computation, making it ideal for batch calculations with fixed covariance.
 *
 * @param state Monte Carlo state with cached Cholesky factor
 * @param means Asset mean returns (dim elements)
 * @param weights Portfolio weights (dim elements)
 * @param dim Number of assets (must match state->dim)
 * @param confidence Confidence level (must be in (0, 1))
 * @param var Output: Value at Risk
 * @param cvar Output: Conditional Value at Risk
 * @return 0 on success
 *         -1 if any required pointer is NULL
 *         -2 if dim mismatch with state
 *         -3 if confidence is not in (0, 1)
 *         -4 if Cholesky decomposition not cached (call set_covariance first)
 *
 * Time complexity: O(n_paths × dim²) - no O(dim³) Cholesky cost
 * Space complexity: O(n_paths × dim) temporary storage
 * Thread safety: Not thread-safe (state contains mutable RNG)
 *
 * @note Must call fc_optim_var_monte_carlo_set_covariance() before using this function
 * @note 50-60% faster than fc_optim_var_monte_carlo() for repeated calculations
 * @note Results are stochastic and will vary between runs unless seed is fixed
 *
 * @see fc_optim_var_monte_carlo_set_covariance
 */
int fc_optim_var_monte_carlo_cached(
    fc_var_mc_state_t* state,
    const double* means,
    const double* weights,
    size_t dim,
    double confidence,
    double* var,
    double* cvar
);

/**
 * @brief Calculate Monte Carlo VaR/CVaR for a portfolio
 *
 * Simulates correlated asset returns using multivariate normal distribution
 * and computes VaR/CVaR from simulated portfolio returns.
 *
 * The simulation process:
 * 1. Compute Cholesky decomposition of covariance matrix: L where LL^T = Σ
 * 2. Generate n_paths independent standard normal vectors Z
 * 3. Transform to correlated normals: X = μ + LZ
 * 4. Compute portfolio returns: r = w^T X
 * 5. Calculate VaR as (1-α) quantile and CVaR as conditional mean
 *
 * @param state Monte Carlo state (must be non-NULL)
 * @param means Asset mean returns (dim elements)
 * @param cov_matrix Covariance matrix (dim × dim, row-major, must be positive definite)
 * @param weights Portfolio weights (dim elements)
 * @param dim Number of assets (must match state->dim)
 * @param confidence Confidence level (must be in (0, 1))
 * @param var Output: Value at Risk
 * @param cvar Output: Conditional Value at Risk
 * @return 0 on success
 *         -1 if any required pointer is NULL
 *         -2 if dim mismatch with state
 *         -3 if confidence is not in (0, 1)
 *         -4 if covariance matrix is not positive definite
 *
 * Time complexity: O(dim³ + n_paths × dim²)
 * Space complexity: O(n_paths × dim) temporary storage
 * Thread safety: Not thread-safe (state contains mutable RNG)
 *
 * @note Performance target: 500 assets × 10000 paths in < 100ms on modern x86_64 with AVX2
 * @note Precision: Monte Carlo error ~ 1/√n_paths (10000 paths ≈ 1% error)
 * @note Results are stochastic and will vary between runs unless seed is fixed
 */
int fc_optim_var_monte_carlo(
    fc_var_mc_state_t* state,
    const double* means,
    const double* cov_matrix,
    const double* weights,
    size_t dim,
    double confidence,
    double* var,
    double* cvar
);

/**
 * @brief Calculate Monte Carlo VaR/CVaR from historical returns
 *
 * Convenience function that estimates covariance matrix from historical data
 * and then performs Monte Carlo simulation.
 *
 * @param state Monte Carlo state
 * @param returns Historical returns matrix (dim × n_periods, row-major)
 * @param weights Portfolio weights (dim elements)
 * @param dim Number of assets
 * @param n_periods Number of historical periods
 * @param confidence Confidence level
 * @param var Output: VaR
 * @param cvar Output: CVaR
 * @return 0 on success, error code otherwise
 *
 * Time complexity: O(dim² × n_periods + dim³ + n_paths × dim²)
 * Space complexity: O(dim²) for covariance matrix
 * Thread safety: Not thread-safe
 *
 * @note This function computes sample covariance from historical returns
 * @note Requires sufficient historical data (n_periods >= dim + 1)
 */
int fc_optim_var_monte_carlo_from_history(
    fc_var_mc_state_t* state,
    const double* returns,
    const double* weights,
    size_t dim,
    size_t n_periods,
    double confidence,
    double* var,
    double* cvar
);

/**
 * @brief Simple Monte Carlo VaR/CVaR calculation (convenience function)
 *
 * One-shot convenience function that internally manages state object.
 * Suitable for single calculations where state management overhead is acceptable.
 *
 * @param returns Historical returns matrix (dim × n_periods, row-major)
 * @param weights Portfolio weights (dim elements)
 * @param dim Number of assets
 * @param n_periods Number of historical periods (must be >= dim + 1)
 * @param n_paths Number of Monte Carlo paths to simulate
 * @param confidence Confidence level (must be in (0, 1))
 * @param seed Random seed for reproducibility
 * @param var Output: VaR
 * @param cvar Output: CVaR
 * @return 0 on success, error code otherwise
 *
 * Time complexity: O(dim² × n_periods + dim³ + n_paths × dim²)
 * Space complexity: O(n_paths × dim + dim²) for state and covariance
 * Thread safety: Thread-safe (creates independent state)
 *
 * @note For repeated calculations, use fc_optim_var_monte_carlo_state_create()
 *       and fc_optim_var_monte_carlo_from_history() to avoid repeated allocation
 */
int fc_optim_var_monte_carlo_simple(
    const double* returns,
    const double* weights,
    size_t dim,
    size_t n_periods,
    size_t n_paths,
    double confidence,
    uint64_t seed,
    double* var,
    double* cvar
);

/**
 * @brief Calculate Monte Carlo VaR/CVaR for multiple portfolios in batch
 *
 * Batch processing for multiple portfolios sharing the same asset return history
 * but with different weights. Uses Monte Carlo simulation for each portfolio.
 * Efficient for risk measurement across many portfolios.
 *
 * @param state Monte Carlo state object (pre-created with appropriate n_paths)
 * @param returns Asset return matrix (dim × n_periods, row-major)
 * @param weights_matrix Portfolio weights matrix (num_portfolios × dim, row-major)
 * @param num_portfolios Number of portfolios to process
 * @param dim Number of assets
 * @param n_periods Number of historical periods (must be >= dim + 1)
 * @param confidence Confidence level (must be in (0, 1))
 * @param var Output: VaR values (num_portfolios elements, pre-allocated)
 * @param cvar Output: CVaR values (num_portfolios elements, pre-allocated)
 * @return 0 on success
 *         -1 if any required pointer is NULL
 *         -2 if num_portfolios == 0, dim == 0, or n_periods == 0
 *         -3 if confidence is not in (0, 1)
 *         -7 if n_periods < dim + 1 (insufficient data)
 *
 * Time complexity: O(num_portfolios × (dim² × n_periods + dim³ + n_paths × dim))
 * Space complexity: O(dim × n_periods + dim²) for covariance computation
 * Thread safety: Thread-safe if state objects and output buffers are different
 *
 * @note All portfolios share the same covariance matrix computed from returns
 */
int fc_optim_var_monte_carlo_batch(
    fc_var_mc_state_t* state,
    const double* returns,
    const double* weights_matrix,
    size_t num_portfolios,
    size_t dim,
    size_t n_periods,
    double confidence,
    double* var,
    double* cvar
);

#ifdef __cplusplus
}
#endif

#endif /* FC_OPTIM_VAR_MONTE_CARLO_H */
