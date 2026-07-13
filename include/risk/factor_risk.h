/**
 * @file factor_risk.h
 * @brief Factor risk decomposition for HFT risk management (Barra-style)
 *
 * Provides functions to decompose portfolio risk into factor risk (systematic)
 * and specific risk (idiosyncratic) using multi-factor risk models.
 *
 * Mathematical Background:
 * - Factor model: r_i = Σ_k (β_ik × f_k) + ε_i
 * - Portfolio return: r_p = Σ_i (w_i × r_i)
 * - Factor exposure: X_k = Σ_i (w_i × β_ik)
 * - Portfolio variance: σ²_p = X^T × F × X + Σ_i (w_i² × σ²_ε_i)
 *   where F is factor covariance matrix, σ²_ε_i is specific variance
 *
 * Risk Decomposition:
 * - Factor risk: X^T × F × X (systematic risk from common factors)
 * - Specific risk: Σ_i (w_i² × σ²_ε_i) (idiosyncratic risk)
 * - Total risk: Factor risk + Specific risk
 *
 * Common factor types:
 * - Market factor (overall market movement)
 * - Size factor (market cap effect)
 * - Value factor (valuation metrics)
 * - Momentum factor (price trends)
 * - Volatility factor (historical volatility)
 * - Liquidity factor (trading volume)
 * - Industry factors (sector exposure)
 *
 * Time Complexity: O(n×f + f²) where n = assets, f = factors
 * Space Complexity: O(n×f + f²) for exposure and covariance matrices
 */

#ifndef FC_EX_FACTOR_RISK_H
#define FC_EX_FACTOR_RISK_H

#include "error.h"
#include "platform.h"
#include <stddef.h>

FC_BEGIN_DECLS

/**
 * @brief Calculate portfolio factor exposures
 *
 * Computes the portfolio's exposure to each factor:
 *   X_k = Σ_i (weights[i] × factor_exposures[i,k])
 *
 * This represents the portfolio's aggregate loading on each systematic factor.
 *
 * @param portfolio_exposures Output: portfolio exposure to each factor (n_factors)
 * @param weights Asset weights in portfolio (n_assets)
 * @param factor_exposures Factor exposure matrix (n_assets × n_factors, row-major)
 * @param n_assets Number of assets in portfolio
 * @param n_factors Number of factors
 * @return FC_OK on success, error code otherwise
 *
 * @pre portfolio_exposures != NULL
 * @pre weights != NULL
 * @pre factor_exposures != NULL
 * @pre n_assets > 0
 * @pre n_factors > 0
 *
 * Time complexity: O(n×f)
 * Space complexity: O(1)
 * Thread safety: Safe if output doesn't overlap with inputs
 * SIMD: Vectorized with AVX-512/AVX2/SSE4.2
 */
FC_API fc_status_t fc_ex_risk_portfolio_factor_exposures(
    double* restrict portfolio_exposures,
    const double* restrict weights,
    const double* restrict factor_exposures,
    size_t n_assets,
    size_t n_factors
);

/**
 * @brief Calculate factor risk contribution
 *
 * Computes the factor (systematic) risk component:
 *   Factor Risk = X^T × F × X
 *
 * where X is the portfolio factor exposure vector and F is the
 * factor covariance matrix.
 *
 * This captures risk from common factors affecting multiple assets.
 *
 * @param factor_risk Output: factor risk contribution (scalar)
 * @param portfolio_exposures Portfolio factor exposures (n_factors)
 * @param factor_covariance Factor covariance matrix (n_factors × n_factors, row-major, symmetric)
 * @param n_factors Number of factors
 * @param work_buffer Temporary buffer (n_factors elements, caller-allocated)
 * @return FC_OK on success, error code otherwise
 *
 * @pre factor_risk != NULL
 * @pre portfolio_exposures != NULL
 * @pre factor_covariance != NULL
 * @pre work_buffer != NULL
 * @pre n_factors > 0
 *
 * Time complexity: O(f²)
 * Space complexity: O(f) work buffer
 * Thread safety: Safe if buffers don't overlap
 * SIMD: Vectorized matrix-vector multiplication
 */
FC_API fc_status_t fc_ex_risk_factor_risk(
    double* restrict factor_risk,
    const double* restrict portfolio_exposures,
    const double* restrict factor_covariance,
    size_t n_factors,
    double* restrict work_buffer
);

/**
 * @brief Calculate specific risk contribution
 *
 * Computes the specific (idiosyncratic) risk component:
 *   Specific Risk = Σ_i (weights[i]² × specific_variance[i])
 *
 * This captures risk unique to individual assets, uncorrelated with factors.
 *
 * @param specific_risk Output: specific risk contribution (scalar)
 * @param weights Asset weights in portfolio (n_assets)
 * @param specific_variance Specific variance for each asset (n_assets)
 * @param n_assets Number of assets
 * @return FC_OK on success, error code otherwise
 *
 * @pre specific_risk != NULL
 * @pre weights != NULL
 * @pre specific_variance != NULL
 * @pre n_assets > 0
 *
 * Time complexity: O(n)
 * Space complexity: O(1)
 * Thread safety: Safe if output doesn't overlap with inputs
 * SIMD: Vectorized with AVX-512/AVX2/SSE4.2
 */
FC_API fc_status_t fc_ex_risk_specific_risk(
    double* restrict specific_risk,
    const double* restrict weights,
    const double* restrict specific_variance,
    size_t n_assets
);

/**
 * @brief Decompose portfolio risk into factor and specific components
 *
 * Performs complete Barra-style risk decomposition:
 *   Total Risk = Factor Risk + Specific Risk
 *
 * This is the main entry point for factor risk analysis.
 *
 * @param factor_risk Output: factor (systematic) risk contribution
 * @param specific_risk Output: specific (idiosyncratic) risk contribution
 * @param total_risk Output: total portfolio risk (sum of components)
 * @param portfolio_exposures Output: portfolio factor exposures (n_factors, optional, NULL to skip)
 * @param weights Asset weights in portfolio (n_assets)
 * @param factor_exposures Factor exposure matrix (n_assets × n_factors, row-major)
 * @param factor_covariance Factor covariance matrix (n_factors × n_factors, row-major, symmetric)
 * @param specific_variance Specific variance for each asset (n_assets)
 * @param n_assets Number of assets in portfolio
 * @param n_factors Number of factors
 * @param work_buffer Temporary buffer (n_factors elements, caller-allocated)
 * @return FC_OK on success, error code otherwise
 *
 * @pre factor_risk != NULL
 * @pre specific_risk != NULL
 * @pre total_risk != NULL
 * @pre weights != NULL
 * @pre factor_exposures != NULL
 * @pre factor_covariance != NULL
 * @pre specific_variance != NULL
 * @pre work_buffer != NULL
 * @pre n_assets > 0
 * @pre n_factors > 0
 *
 * Time complexity: O(n×f + f²)
 * Space complexity: O(f) work buffer
 * Thread safety: Safe if buffers don't overlap
 * SIMD: Vectorized throughout
 */
FC_API fc_status_t fc_ex_risk_factor_decomposition(
    double* restrict factor_risk,
    double* restrict specific_risk,
    double* restrict total_risk,
    double* restrict portfolio_exposures,
    const double* restrict weights,
    const double* restrict factor_exposures,
    const double* restrict factor_covariance,
    const double* restrict specific_variance,
    size_t n_assets,
    size_t n_factors,
    double* restrict work_buffer
);

/**
 * @brief Calculate marginal factor risk contribution for each asset
 *
 * Computes how much each asset contributes to factor risk:
 *   Marginal Factor Risk_i = 2 × Σ_k Σ_l (β_ik × F_kl × X_l) × w_i
 *
 * This identifies which positions drive systematic risk.
 *
 * @param marginal_factor_risk Output: marginal factor risk for each asset (n_assets)
 * @param weights Asset weights in portfolio (n_assets)
 * @param factor_exposures Factor exposure matrix (n_assets × n_factors, row-major)
 * @param factor_covariance Factor covariance matrix (n_factors × n_factors, row-major, symmetric)
 * @param portfolio_exposures Portfolio factor exposures (n_factors)
 * @param n_assets Number of assets
 * @param n_factors Number of factors
 * @param work_buffer Temporary buffer (n_factors elements, caller-allocated)
 * @return FC_OK on success, error code otherwise
 *
 * @pre marginal_factor_risk != NULL
 * @pre weights != NULL
 * @pre factor_exposures != NULL
 * @pre factor_covariance != NULL
 * @pre portfolio_exposures != NULL
 * @pre work_buffer != NULL
 * @pre n_assets > 0
 * @pre n_factors > 0
 *
 * Time complexity: O(n×f²)
 * Space complexity: O(f) work buffer
 * Thread safety: Safe if buffers don't overlap
 * SIMD: Vectorized matrix operations
 */
FC_API fc_status_t fc_ex_risk_marginal_factor_risk(
    double* restrict marginal_factor_risk,
    const double* restrict weights,
    const double* restrict factor_exposures,
    const double* restrict factor_covariance,
    const double* restrict portfolio_exposures,
    size_t n_assets,
    size_t n_factors,
    double* restrict work_buffer
);

/**
 * @brief Verify that factor + specific risk equals total risk
 *
 * Validates the decomposition property:
 *   Factor Risk + Specific Risk ≈ Total Risk
 *
 * @param factor_risk Computed factor risk
 * @param specific_risk Computed specific risk
 * @param total_risk Expected total risk
 * @param tolerance Relative tolerance for comparison (e.g., 1e-6)
 * @return 1 if decomposition is valid within tolerance, 0 otherwise
 *
 * @pre tolerance > 0
 *
 * Time complexity: O(1)
 * Space complexity: O(1)
 */
FC_API int fc_ex_risk_verify_factor_decomposition(
    double factor_risk,
    double specific_risk,
    double total_risk,
    double tolerance
);

FC_END_DECLS

#endif /* FC_EX_FACTOR_RISK_H */
