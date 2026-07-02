/**
 * @file delta_hedge.h
 * @brief Delta hedging strategy for options portfolio Greeks management
 *
 * Strategy-level delta hedging implementation for options portfolios.
 * Provides batch computation of:
 * - Portfolio net delta aggregation (Δ_net = Σ Δᵢ × Nᵢ)
 * - Hedge quantity calculation (N_hedge = -Δ_net / Δ_future)
 * - Multi-book batch processing for operational efficiency
 *
 * This module provides the computational core (C/SIMD) for portfolio delta
 * aggregation and hedge quantity calculation. The runtime decision logic
 * (band thresholds, rehedge timing, Γ/Vega monitoring) is implemented in
 * the Go orchestration layer.
 *
 * Key formulas:
 *   Portfolio net delta:    Δ_net = Σᵢ Δᵢ × Nᵢ
 *                          where Δᵢ = delta of leg i (from BSM Greeks)
 *                                Nᵢ = position quantity of leg i (signed)
 *
 *   Hedge quantity:        N_hedge = -Δ_net / Δ_future
 *                          where Δ_future = delta of hedging instrument
 *
 * Band mechanism (implemented in Go layer):
 *   - Only rehedge when |Δ_net| exceeds band threshold (0.01-0.05)
 *   - Avoids excessive trading friction from minor delta changes
 *   - Band width adjusts dynamically based on Γ and time to expiry
 *   - Residual delta after rounding carries into next band check
 *
 * Trading logic (Go layer lifecycle management):
 *   - Options trade triggers BSM Greeks calculation (M11/M04)
 *   - Aggregate portfolio net delta across all option legs
 *   - Check if |Δ_net| exceeds band threshold (Go decision)
 *   - Calculate hedge quantity N_hedge (this module)
 *   - Round to contract lot size (Go layer, user-configured rounding)
 *   - Execute hedge order via futures/spot/ETF (Go/OMS layer)
 *   - Monitor Γ/Vega continuously to prevent tail risk (Go monitoring)
 */

#ifndef FC_EX_STRAT_DELTA_HEDGE_H
#define FC_EX_STRAT_DELTA_HEDGE_H

#include "error.h"
#include "platform.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Aggregate portfolio net delta and calculate hedge quantities (batch)
 *
 * Batch computation of portfolio net delta and hedge quantities for multiple books.
 * For each book:
 *   1. Aggregate net delta: Δ_net = Σᵢ₌₁ⁿ Δᵢ × Nᵢ
 *   2. Calculate hedge quantity: N_hedge = -Δ_net / Δ_future
 *
 * This function processes multiple option books in a single call to amortize
 * cgo overhead. Each book can have a different number of legs (up to max_legs).
 *
 * Input layout:
 * - leg_delta: [n_books × max_legs] row-major, padded
 * - leg_qty: [n_books × max_legs] row-major, padded
 * - n_legs: [n_books] actual number of legs per book
 * - delta_future: [n_books] delta of hedging instrument per book
 *
 * For book i with n_legs[i] legs:
 *   - leg_delta[i*max_legs + j] for j ∈ [0, n_legs[i])
 *   - leg_qty[i*max_legs + j] for j ∈ [0, n_legs[i])
 *   - Unused slots (j >= n_legs[i]) are ignored
 *
 * @param[out] delta_net_out Portfolio net delta for each book (length n_books)
 * @param[out] hedge_qty_out Hedge quantity for each book (length n_books), continuous value (not
 * rounded)
 * @param[in] leg_delta Delta values for all legs, shape (n_books × max_legs), row-major
 * @param[in] leg_qty Position quantities for all legs, shape (n_books × max_legs), row-major,
 * signed
 * @param[in] n_legs Number of active legs per book (length n_books)
 * @param[in] delta_future Delta of hedging instrument per book (length n_books)
 * @param[in] n_books Number of option books to process
 * @param[in] max_legs Maximum number of legs per book (stride for input arrays)
 * @return FC_OK on success, error code otherwise
 *
 * Time complexity: O(n_books × max_legs)
 * Space complexity: O(1) additional space (in-place computation)
 * Thread safety: Thread-safe (no global state)
 *
 * Input validation:
 * - delta_net_out, hedge_qty_out, leg_delta, leg_qty, n_legs, delta_future must not be NULL
 * - n_books must be > 0
 * - max_legs must be > 0
 * - n_legs[i] must be in [1, max_legs] for all i
 * - delta_future[i] must not be zero (division by zero check)
 *
 * Performance notes:
 * - For n_books >= 10, uses batch processing to amortize cgo overhead
 * - Uses SIMD-optimized multiply-add for delta aggregation
 * - Uses Kahan compensated summation for high-precision accumulation
 * - Hedge quantity calculation uses SIMD division where possible
 *
 * Special value handling:
 * - If any leg_delta[i,j] or leg_qty[i,j] is NaN: delta_net_out[i] = NaN, hedge_qty_out[i] = NaN
 * - If delta_future[i] is NaN or zero: hedge_qty_out[i] = NaN (avoid division by zero)
 * - If delta_net_out[i] is NaN: hedge_qty_out[i] = NaN (propagate NaN)
 *
 * Rounding notes:
 * - hedge_qty_out contains continuous (unrounded) hedge quantities
 * - Rounding to contract lot sizes is performed in Go layer based on strategy config
 * - Residual delta after rounding is carried into next band threshold check (Go layer)
 * - This separation allows flexible rounding strategies (round half up, towards zero, etc.)
 *
 * @note Greeks (Δᵢ) are pre-computed by BSM Greeks batch calculation (M11/M04)
 * @note Band threshold checking and rehedge timing decisions are in Go layer
 * @note This function does NOT perform lot size rounding (Go layer responsibility)
 * @note For single-book hot path, consider calling from Go directly (avoid batch overhead)
 */
FC_API fc_status_t fc_ex_strat_delta_aggregate(
    double* delta_net_out,
    double* hedge_qty_out,
    const double* leg_delta,
    const double* leg_qty,
    const int* n_legs,
    const double* delta_future,
    size_t n_books,
    int max_legs
);

#ifdef __cplusplus
}
#endif

#endif /* FC_EX_STRAT_DELTA_HEDGE_H */
