/**
 * @file stress_scenario.h
 * @brief Stress testing scenario application for HFT risk management
 *
 * Provides functions to apply historical or hypothetical stress scenarios to
 * current portfolio positions and calculate potential P&L under extreme conditions.
 *
 * Scenario Types:
 * 1. Historical scenarios: Replay actual historical market shocks
 *    - Example: 2008 financial crisis, 2020 COVID crash
 * 2. Hypothetical scenarios: User-defined extreme market movements
 *    - Example: Market down 30%, volatility up 50%
 *
 * Mathematical Model:
 * - Shock application: P&L = Σ_i (position_i × price_i × shock_i)
 * - Multiplicative shock: new_price = old_price × (1 + shock)
 * - Additive shock: new_price = old_price + shock
 *
 * Use Cases:
 * - Regulatory stress testing (CCAR, DFAST)
 * - Risk limit validation
 * - Scenario analysis for decision-making
 * - Tail risk assessment
 *
 * Time Complexity: O(n×s) where n = assets, s = scenarios
 * Space Complexity: O(n×s) for shock matrix storage
 */

#ifndef FC_EX_STRESS_SCENARIO_H
#define FC_EX_STRESS_SCENARIO_H

#include "error.h"
#include "platform.h"
#include <stddef.h>

FC_BEGIN_DECLS

/**
 * @brief Shock type enumeration
 */
typedef enum {
    FC_SHOCK_MULTIPLICATIVE = 0, /**< Multiplicative shock: new = old × (1 + shock) */
    FC_SHOCK_ADDITIVE       = 1  /**< Additive shock: new = old + shock */
} fc_shock_type_t;

/**
 * @brief Apply single stress scenario to portfolio positions
 *
 * Calculates portfolio P&L under a single stress scenario:
 *   P&L = Σ_i (position_value[i] × shock[i])
 *
 * For multiplicative shocks (most common):
 *   shock[i] represents the price change ratio (e.g., -0.30 for 30% drop)
 *
 * For additive shocks:
 *   shock[i] represents the absolute price change
 *
 * @param scenario_pnl Output: P&L under the stress scenario
 * @param position_values Current market value of each position (n_assets)
 * @param shocks Shock values for each asset (n_assets)
 * @param shock_type Type of shock (multiplicative or additive)
 * @param n_assets Number of assets in portfolio
 * @return FC_OK on success, error code otherwise
 *
 * @pre scenario_pnl != NULL
 * @pre position_values != NULL
 * @pre shocks != NULL
 * @pre n_assets > 0
 *
 * Time complexity: O(n)
 * Space complexity: O(1)
 * Thread safety: Safe if output doesn't overlap with inputs
 * SIMD: Vectorized with AVX-512/AVX2/SSE4.2
 */
FC_API fc_status_t fc_ex_risk_apply_scenario(
    double* restrict scenario_pnl,
    const double* restrict position_values,
    const double* restrict shocks,
    fc_shock_type_t shock_type,
    size_t n_assets
);

/**
 * @brief Apply multiple stress scenarios in batch
 *
 * Calculates portfolio P&L for multiple scenarios simultaneously:
 *   P&L_s = Σ_i (position_value[i] × shock[i,s])
 *
 * The shock matrix is in row-major format (n_assets × n_scenarios).
 *
 * This is the main entry point for stress testing, providing batch processing
 * to amortize overhead.
 *
 * @param scenario_pnls Output: P&L for each scenario (n_scenarios)
 * @param position_values Current market value of each position (n_assets)
 * @param shock_matrix Shock matrix (n_assets × n_scenarios, row-major)
 * @param shock_type Type of shock (multiplicative or additive)
 * @param n_assets Number of assets in portfolio
 * @param n_scenarios Number of stress scenarios
 * @return FC_OK on success, error code otherwise
 *
 * @pre scenario_pnls != NULL
 * @pre position_values != NULL
 * @pre shock_matrix != NULL
 * @pre n_assets > 0
 * @pre n_scenarios > 0
 *
 * Time complexity: O(n×s)
 * Space complexity: O(1)
 * Thread safety: Safe if output doesn't overlap with inputs
 * SIMD: Vectorized with AVX-512/AVX2/SSE4.2
 */
FC_API fc_status_t fc_ex_risk_apply_scenarios_batch(
    double* restrict scenario_pnls,
    const double* restrict position_values,
    const double* restrict shock_matrix,
    fc_shock_type_t shock_type,
    size_t n_assets,
    size_t n_scenarios
);

/**
 * @brief Extract historical scenario from returns data
 *
 * Extracts shocks from historical returns for a specific time period:
 *   shock[i] = Σ_{t=start}^{end} return[i,t]
 *
 * This converts historical returns into a scenario shock vector.
 *
 * @param shocks Output: extracted shock values (n_assets)
 * @param returns Historical returns matrix (n_assets × n_days, row-major)
 * @param start_day Starting day index (inclusive)
 * @param end_day Ending day index (inclusive)
 * @param n_assets Number of assets
 * @param n_days Total number of historical days
 * @return FC_OK on success, error code otherwise
 *
 * @pre shocks != NULL
 * @pre returns != NULL
 * @pre n_assets > 0
 * @pre start_day <= end_day
 * @pre end_day < n_days
 *
 * Time complexity: O(n×d) where d = end_day - start_day + 1
 * Space complexity: O(1)
 * Thread safety: Safe if output doesn't overlap with inputs
 * SIMD: Vectorized with AVX-512/AVX2/SSE4.2
 */
FC_API fc_status_t fc_ex_risk_extract_historical_scenario(
    double* restrict shocks,
    const double* restrict returns,
    size_t start_day,
    size_t end_day,
    size_t n_assets,
    size_t n_days
);

/**
 * @brief Calculate Value-at-Risk from scenario results
 *
 * Computes VaR from a set of scenario P&L values by finding the appropriate quantile.
 * This provides a scenario-based VaR estimate.
 *
 * @param var_estimate Output: VaR estimate at given confidence level
 * @param scenario_pnls P&L values from scenarios (n_scenarios)
 * @param confidence Confidence level (e.g., 0.95 for 95% VaR)
 * @param n_scenarios Number of scenarios
 * @param work_buffer Temporary buffer for sorting (n_scenarios elements, caller-allocated)
 * @return FC_OK on success, error code otherwise
 *
 * @pre var_estimate != NULL
 * @pre scenario_pnls != NULL
 * @pre work_buffer != NULL
 * @pre n_scenarios > 0
 * @pre confidence > 0 && confidence < 1
 *
 * Time complexity: O(s × log s) due to sorting
 * Space complexity: O(s) work buffer
 * Thread safety: Safe if buffers don't overlap
 *
 * @note The input scenario_pnls is not modified; sorting happens in work_buffer
 */
FC_API fc_status_t fc_ex_risk_scenario_var(
    double* restrict var_estimate,
    const double* restrict scenario_pnls,
    double confidence,
    size_t n_scenarios,
    double* restrict work_buffer
);

/**
 * @brief Find worst-case scenario
 *
 * Identifies the scenario with the largest loss (most negative P&L).
 *
 * @param worst_pnl Output: P&L of worst scenario
 * @param worst_index Output: Index of worst scenario
 * @param scenario_pnls P&L values from scenarios (n_scenarios)
 * @param n_scenarios Number of scenarios
 * @return FC_OK on success, error code otherwise
 *
 * @pre worst_pnl != NULL
 * @pre worst_index != NULL
 * @pre scenario_pnls != NULL
 * @pre n_scenarios > 0
 *
 * Time complexity: O(s)
 * Space complexity: O(1)
 * Thread safety: Safe if output doesn't overlap with inputs
 */
FC_API fc_status_t fc_ex_risk_worst_scenario(
    double* restrict worst_pnl,
    size_t* restrict worst_index,
    const double* restrict scenario_pnls,
    size_t n_scenarios
);

FC_END_DECLS

#endif /* FC_EX_STRESS_SCENARIO_H */
