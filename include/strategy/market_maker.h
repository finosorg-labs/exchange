/**
 * @file market_maker.h
 * @brief Market maker strategy using Avellaneda-Stoikov model
 *
 * Strategy-level market maker implementation that wraps the core market_maker_quotes
 * functions with strategy-specific parameters and batch processing.
 *
 * This module provides:
 * - Batch optimal quote calculation for multiple symbols
 * - Strategy parameter structure (fc_ex_strat_mm_params_t)
 * - Integration with strategy engine event flow
 *
 * Formulas (Avellaneda-Stoikov):
 *   reservation_price = mid - inventory × gamma × sigma² × (T-t)
 *   spread = gamma × sigma² × (T-t) + (2/gamma) × ln(1 + gamma/kappa)
 *   bid = reservation_price - spread/2
 *   ask = reservation_price + spread/2
 *
 * Where:
 *   - mid: current mid price (micro-price from orderbook)
 *   - inventory: current position (positive = long, negative = short)
 *   - gamma: risk aversion coefficient
 *   - sigma: volatility
 *   - T-t: remaining time horizon
 *   - kappa: order arrival rate (lambda in original paper)
 */

#ifndef FC_EX_STRAT_MARKET_MAKER_H
#define FC_EX_STRAT_MARKET_MAKER_H

#include "error.h"
#include "platform.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Market maker strategy parameters (Structure of Arrays format)
 *
 * All array pointers must point to C-allocated aligned memory for zero-copy cgo.
 * Arrays are indexed by symbol ID [0..n-1].
 */
typedef struct {
    const double* mid;       /**< [n] Mid prices (micro-price from orderbook) */
    const double* inventory; /**< [n] Current inventory positions (signed) */
    const double* sigma;     /**< [n] Volatilities */
    const double* kappa;     /**< [n] Order arrival rates (lambda) */
    double gamma;            /**< Risk aversion coefficient (positive scalar) */
    double t_minus_t;        /**< Remaining time horizon T-t (positive) */
    size_t n;                /**< Number of symbols */
} fc_ex_strat_mm_params_t;

/**
 * @brief Calculate optimal market maker quotes for multiple symbols
 *
 * Batch calculation of bid/ask quotes using Avellaneda-Stoikov model.
 * Wraps fc_market_maker_quotes with strategy-specific parameter structure.
 *
 * This function computes:
 * 1. Reservation price adjusted for inventory risk
 * 2. Optimal spread balancing adverse selection and inventory risk
 * 3. Final bid = reservation_price - spread/2
 * 4. Final ask = reservation_price + spread/2
 *
 * @param[out] bid_out Output array of optimal bid prices (length n)
 * @param[out] ask_out Output array of optimal ask prices (length n)
 * @param[out] reserve_out Output array of reservation prices (length n), can be NULL
 * @param[in] params Strategy parameters (all arrays must have length n)
 * @return FC_OK on success, error code otherwise
 *
 * Time complexity: O(n)
 * Space complexity: O(1) additional space
 * Thread safety: Thread-safe (no global state)
 *
 * Input validation:
 * - params must not be NULL
 * - params->mid, inventory, sigma, kappa must not be NULL
 * - params->gamma must be positive
 * - params->t_minus_t must be positive
 * - params->n must be > 0
 * - params->kappa[i] must be positive for all i
 * - params->sigma[i] must be non-negative for all i
 *
 * Performance notes:
 * - For n >= 1000, this batch function amortizes cgo overhead (50-100ns fixed cost)
 * - SIMD optimization applied for exp/log/multiply-add operations
 * - Uses AVX-512 > AVX2 > SSE4.2 > scalar dispatch
 *
 * @note NaN/Inf in inputs will propagate to outputs
 * @note All input arrays must point to C-allocated memory for zero-copy cgo
 */
FC_API fc_status_t fc_ex_strat_mm_quotes(
    double* bid_out,
    double* ask_out,
    double* reserve_out,
    const fc_ex_strat_mm_params_t* params
);

/**
 * @brief Calculate reservation prices only (intermediate calculation)
 *
 * Computes inventory-adjusted mid prices without the spread calculation.
 * Useful for monitoring or when spread is calculated separately.
 *
 * Formula: reservation_price = mid - inventory × gamma × sigma² × (T-t)
 *
 * @param[out] reserve_out Output array of reservation prices (length n)
 * @param[in] params Strategy parameters
 * @return FC_OK on success, error code otherwise
 *
 * Time complexity: O(n)
 * Space complexity: O(1) additional space
 * Thread safety: Thread-safe
 */
FC_API fc_status_t
fc_ex_strat_mm_reservation_price(double* reserve_out, const fc_ex_strat_mm_params_t* params);

/**
 * @brief Calculate optimal spreads only (intermediate calculation)
 *
 * Computes optimal bid-ask spreads without the reservation price.
 * Useful for spread analysis or when reservation price is known.
 *
 * Formula: spread = gamma × sigma² × (T-t) + (2/gamma) × ln(1 + gamma/kappa)
 *
 * @param[out] spread_out Output array of optimal spreads (length n)
 * @param[in] params Strategy parameters
 * @return FC_OK on success, error code otherwise
 *
 * Time complexity: O(n)
 * Space complexity: O(1) additional space
 * Thread safety: Thread-safe
 *
 * @note This function uses log() which may be expensive; SIMD optimization is applied
 */
FC_API fc_status_t
fc_ex_strat_mm_optimal_spread(double* spread_out, const fc_ex_strat_mm_params_t* params);

#ifdef __cplusplus
}
#endif

#endif /* FC_EX_STRAT_MARKET_MAKER_H */
