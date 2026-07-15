/**
 * @file var_historical.h
 * @brief Historical simulation VaR/CVaR calculation
 *
 * Implements Value at Risk (VaR) and Conditional Value at Risk (CVaR) using
 * the historical simulation method, which uses the empirical distribution
 * of historical returns without making distributional assumptions.
 */

#ifndef FC_OPTIM_VAR_HISTORICAL_H
#define FC_OPTIM_VAR_HISTORICAL_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Calculate historical simulation VaR/CVaR from return series
 *
 * Computes VaR and CVaR from historical return data using empirical quantiles.
 * This method makes no distributional assumptions and directly uses the
 * observed distribution of returns.
 *
 * VaR is computed as the (1-α) quantile of the return distribution.
 * CVaR is computed as the average of all returns at or below the VaR level.
 *
 * @param returns Historical returns (n elements, typically sorted in ascending order internally)
 * @param n Number of historical observations (must be > 0)
 * @param confidence Confidence level (must be in (0, 1), typically 0.95 or 0.99)
 * @param var Output: Value at Risk (negative indicates loss)
 * @param cvar Output: Conditional Value at Risk (negative indicates loss)
 * @return 0 on success
 *         -1 if returns, var, or cvar is NULL
 *         -2 if n == 0
 *         -3 if confidence is not in (0, 1)
 *
 * Time complexity: O(n log n) due to sorting
 * Space complexity: O(n) for working copy of returns
 * Thread safety: Thread-safe (creates working copy of input)
 *
 * @note The input returns array is not modified (a copy is sorted internally)
 * @note For small sample sizes, results may be sensitive to outliers
 * @note Requires sufficient historical data (typically n >= 250 for reliable estimates)
 */
int fc_optim_var_historical(
    const double* returns,
    size_t n,
    double confidence,
    double* var,
    double* cvar
);

/**
 * @brief Calculate historical simulation VaR/CVaR for portfolio with weights
 *
 * Computes VaR and CVaR for a weighted portfolio given multi-asset return history.
 * Portfolio returns are computed as weighted sum of individual asset returns.
 *
 * @param returns Asset return matrix (m × n, row-major: m assets × n time periods)
 * @param weights Portfolio weights (m elements, must sum to ~1.0)
 * @param m Number of assets
 * @param n Number of time periods
 * @param confidence Confidence level (must be in (0, 1))
 * @param var Output: Portfolio VaR
 * @param cvar Output: Portfolio CVaR
 * @return 0 on success
 *         -1 if any required pointer is NULL
 *         -2 if m == 0 or n == 0
 *         -3 if confidence is not in (0, 1)
 *
 * Time complexity: O(mn + n log n)
 * Space complexity: O(n) for portfolio returns
 * Thread safety: Thread-safe
 *
 * @note Performance: 500 assets × 1000 periods in < 10ms on modern x86_64
 */
int fc_optim_var_historical_portfolio(
    const double* returns,
    const double* weights,
    size_t m,
    size_t n,
    double confidence,
    double* var,
    double* cvar
);

/**
 * @brief Calculate historical VaR/CVaR for multiple portfolios in batch
 *
 * Batch processing for multiple portfolios sharing the same asset return history
 * but with different weights. Efficient for risk measurement across many portfolios.
 *
 * @param returns Asset return matrix (m × n, row-major)
 * @param weights_matrix Portfolio weights matrix (num_portfolios × m, row-major)
 * @param num_portfolios Number of portfolios to process
 * @param m Number of assets
 * @param n Number of time periods
 * @param confidence Confidence level
 * @param var Output: VaR values (num_portfolios elements, pre-allocated)
 * @param cvar Output: CVaR values (num_portfolios elements, pre-allocated)
 * @return 0 on success
 *         -1 if any required pointer is NULL
 *         -2 if num_portfolios == 0, m == 0, or n == 0
 *         -3 if confidence is not in (0, 1)
 *
 * Time complexity: O(num_portfolios × (mn + n log n))
 * Space complexity: O(n) per portfolio for temporary storage
 * Thread safety: Thread-safe if different output buffers are used
 */
int fc_optim_var_historical_batch(
    const double* returns,
    const double* weights_matrix,
    size_t num_portfolios,
    size_t m,
    size_t n,
    double confidence,
    double* var,
    double* cvar
);

/**
 * @brief Calculate historical VaR/CVaR for multiple single-asset return series in batch
 *
 * Batch processing for multiple single assets, each with its own historical return series.
 * Efficient for risk measurement across many individual assets.
 *
 * @param returns_matrix Matrix of return series (num_assets × n, row-major)
 *                       Each row is a separate asset's historical returns
 * @param num_assets Number of assets to process
 * @param n Number of time periods (same for all assets)
 * @param confidence Confidence level (must be in (0, 1))
 * @param var Output: VaR values (num_assets elements, pre-allocated)
 * @param cvar Output: CVaR values (num_assets elements, pre-allocated)
 * @return 0 on success
 *         -1 if any required pointer is NULL
 *         -2 if num_assets == 0 or n == 0
 *         -3 if confidence is not in (0, 1)
 *
 * Time complexity: O(num_assets × n log n)
 * Space complexity: O(n) per asset for temporary storage
 * Thread safety: Thread-safe if different output buffers are used
 */
int fc_optim_var_historical_batch_from_returns(
    const double* returns_matrix,
    size_t num_assets,
    size_t n,
    double confidence,
    double* var,
    double* cvar
);

#ifdef __cplusplus
}
#endif

#endif /* FC_OPTIM_VAR_HISTORICAL_H */
