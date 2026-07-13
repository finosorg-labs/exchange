/**
 * @file marginal_risk.h
 * @brief Marginal VaR calculation for HFT risk management
 *
 * Provides functions to calculate marginal Value-at-Risk (VaR) contributions,
 * which measure how much each position contributes to the portfolio's total VaR.
 *
 * Mathematical Background:
 * - Marginal VaR_i = ∂(VaR_portfolio) / ∂(weight_i)
 * - Approximation: Marginal VaR_i ≈ Cov(r_i, r_portfolio) / σ_portfolio × VaR_portfolio
 * - Component VaR_i = Marginal VaR_i × weight_i
 * - Sum of all Component VaRs equals portfolio VaR
 *
 * Two calculation methods:
 * 1. Correlation method: Uses covariance between asset and portfolio returns
 * 2. Perturbation method: Recalculates VaR with perturbed weights
 *
 * Time Complexity: O(n×T) where n = assets, T = historical days
 * Space Complexity: O(n×T) for return series storage
 */

#ifndef FC_EX_MARGINAL_RISK_H
#define FC_EX_MARGINAL_RISK_H

#include "error.h"
#include "platform.h"
#include <stddef.h>

FC_BEGIN_DECLS

/**
 * @brief Calculate marginal VaR using correlation method
 *
 * Computes marginal VaR for each asset based on its correlation with the portfolio.
 * This is the faster method, suitable for frequent risk calculations.
 *
 * Mathematical formula:
 *   Marginal VaR_i = Cov(r_i, r_portfolio) / σ_portfolio × VaR_portfolio
 *
 * @param marginal_var Output: marginal VaR for each asset (n_assets)
 * @param returns Asset returns matrix (n_assets × n_days, row-major)
 * @param weights Current portfolio weights (n_assets)
 * @param portfolio_var Portfolio VaR value
 * @param n_assets Number of assets in portfolio
 * @param n_days Number of historical days
 * @return FC_OK on success, error code otherwise
 *
 * @pre marginal_var != NULL
 * @pre returns != NULL
 * @pre weights != NULL
 * @pre n_assets > 0
 * @pre n_days > 1
 * @pre portfolio_var >= 0
 *
 * Time complexity: O(n×T)
 * Space complexity: O(T) temporary buffer
 * Thread safety: Safe if output buffers don't overlap
 */
FC_API fc_status_t fc_ex_risk_marginal_var_correlation(
    double* restrict marginal_var,
    const double* restrict returns,
    const double* restrict weights,
    double portfolio_var,
    size_t n_assets,
    size_t n_days
);

/**
 * @brief Calculate component VaR (marginal VaR × weight)
 *
 * Computes the actual VaR contribution of each position:
 *   Component VaR_i = Marginal VaR_i × weight_i
 *
 * The sum of all component VaRs equals the portfolio VaR.
 *
 * @param component_var Output: component VaR for each asset (n_assets)
 * @param marginal_var Input: marginal VaR for each asset (n_assets)
 * @param weights Current portfolio weights (n_assets)
 * @param n_assets Number of assets in portfolio
 * @return FC_OK on success, error code otherwise
 *
 * @pre component_var != NULL
 * @pre marginal_var != NULL
 * @pre weights != NULL
 * @pre n_assets > 0
 *
 * Time complexity: O(n)
 * Space complexity: O(1)
 * Thread safety: Safe if output buffers don't overlap
 */
FC_API fc_status_t fc_ex_risk_component_var(
    double* restrict component_var,
    const double* restrict marginal_var,
    const double* restrict weights,
    size_t n_assets
);

/**
 * @brief Calculate marginal VaR using perturbation method
 *
 * Computes marginal VaR by perturbing each weight and recalculating portfolio VaR.
 * More accurate than correlation method but computationally expensive.
 *
 * Mathematical formula:
 *   Marginal VaR_i ≈ (VaR(w_i + ε) - VaR(w)) / ε
 *
 * @param marginal_var Output: marginal VaR for each asset (n_assets)
 * @param returns Asset returns matrix (n_assets × n_days, row-major)
 * @param weights Current portfolio weights (n_assets)
 * @param portfolio_var Baseline portfolio VaR value
 * @param confidence Confidence level (e.g., 0.95 for 95% VaR)
 * @param epsilon Perturbation size (default: 0.0001 or 1 basis point)
 * @param work_buffer Temporary buffer (n_days elements, caller-allocated)
 * @param n_assets Number of assets in portfolio
 * @param n_days Number of historical days
 * @return FC_OK on success, error code otherwise
 *
 * @pre marginal_var != NULL
 * @pre returns != NULL
 * @pre weights != NULL
 * @pre work_buffer != NULL
 * @pre n_assets > 0
 * @pre n_days > 1
 * @pre confidence > 0 && confidence < 1
 * @pre epsilon > 0 && epsilon < 0.1
 *
 * Time complexity: O(n × T × log(T)) due to VaR recalculation
 * Space complexity: O(T) work buffer
 * Thread safety: Safe if output buffers don't overlap
 *
 * @note This method is more accurate for non-linear portfolios but slower.
 *       Use correlation method for frequent calculations.
 */
FC_API fc_status_t fc_ex_risk_marginal_var_perturbation(
    double* restrict marginal_var,
    const double* restrict returns,
    const double* restrict weights,
    double portfolio_var,
    double confidence,
    double epsilon,
    double* restrict work_buffer,
    size_t n_assets,
    size_t n_days
);

/**
 * @brief Calculate portfolio returns from asset returns and weights
 *
 * Computes weighted portfolio return for each period:
 *   r_portfolio[t] = Σ(weights[i] × returns[i,t])
 *
 * @param portfolio_returns Output: portfolio returns (n_days)
 * @param returns Asset returns matrix (n_assets × n_days, row-major)
 * @param weights Portfolio weights (n_assets)
 * @param n_assets Number of assets
 * @param n_days Number of days
 * @return FC_OK on success, error code otherwise
 *
 * @pre portfolio_returns != NULL
 * @pre returns != NULL
 * @pre weights != NULL
 * @pre n_assets > 0
 * @pre n_days > 0
 *
 * Time complexity: O(n×T)
 * Space complexity: O(1)
 * Thread safety: Safe if output doesn't overlap with inputs
 * SIMD: Vectorized with AVX-512/AVX2/SSE4.2
 */
FC_API fc_status_t fc_ex_risk_portfolio_returns(
    double* restrict portfolio_returns,
    const double* restrict returns,
    const double* restrict weights,
    size_t n_assets,
    size_t n_days
);

/**
 * @brief Verify that component VaRs sum to portfolio VaR
 *
 * Validates the decomposition property:
 *   Σ(Component VaR_i) = Portfolio VaR
 *
 * @param component_var Component VaR for each asset (n_assets)
 * @param portfolio_var Expected portfolio VaR
 * @param n_assets Number of assets
 * @param tolerance Relative tolerance for comparison (e.g., 1e-6)
 * @return 1 if sum matches within tolerance, 0 otherwise
 *
 * @pre component_var != NULL
 * @pre n_assets > 0
 * @pre tolerance > 0
 *
 * Time complexity: O(n)
 * Space complexity: O(1)
 */
FC_API int fc_ex_risk_verify_var_decomposition(
    const double* component_var,
    double portfolio_var,
    size_t n_assets,
    double tolerance
);

FC_END_DECLS

#endif /* FC_EX_MARGINAL_RISK_H */
