/**
 * @file market_maker_quotes.h
 * @brief Market maker quote calculation using Avellaneda-Stoikov model
 *
 * Provides high-performance batch calculation of optimal bid/ask quotes for market makers.
 * Based on the Avellaneda-Stoikov optimal market making model, which balances inventory risk
 * and order flow to compute reservation prices and optimal spreads.
 *
 * Key features:
 * - Batch calculation for hundreds of symbols simultaneously
 * - SIMD-optimized exp/log/multiply-add operations
 * - Inventory-aware quote adjustment
 * - Volatility and arrival rate modeling
 *
 * Model formulas:
 *   reservation_price = mid - inventory × γ × σ² × T
 *   spread = γ × σ² × T + (2/γ) × ln(1 + γ/λ)
 *   bid = reservation_price - spread/2
 *   ask = reservation_price + spread/2
 *
 * Where:
 *   - mid: current mid price
 *   - inventory: current position (positive = long, negative = short)
 *   - γ (gamma): risk aversion coefficient
 *   - σ (sigma): volatility
 *   - T: time horizon
 *   - λ (lambda): order arrival rate
 *
 * Performance target: <1μs per symbol for batch of 1000+ symbols
 */

#ifndef FC_MARKET_MAKER_QUOTES_H
#define FC_MARKET_MAKER_QUOTES_H

#include "error.h"
#include "platform.h"
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Calculate optimal market maker quotes using Avellaneda-Stoikov model
 *
 * Computes optimal bid and ask prices for market makers based on current market conditions,
 * inventory position, and risk parameters. The model minimizes expected terminal wealth
 * variance while accounting for inventory risk.
 *
 * @param[out] bid_prices Output array of optimal bid prices (length n)
 * @param[out] ask_prices Output array of optimal ask prices (length n)
 * @param[in] mid_prices Input array of current mid prices (length n)
 * @param[in] inventories Input array of current inventory positions (length n)
 * @param[in] volatilities Input array of price volatilities σ (length n)
 * @param[in] arrival_rates Input array of order arrival rates λ (length n)
 * @param[in] risk_aversion Risk aversion coefficient γ (positive scalar)
 * @param[in] time_horizon Time horizon T in same units as volatilities
 * @param[in] n Number of symbols to process
 * @return FC_OK on success, error code otherwise
 *
 * Time complexity: O(n)
 * Space complexity: O(1) additional space
 * Thread safety: Thread-safe (no global state)
 *
 * @note All input arrays must have at least n elements
 * @note Output arrays must be pre-allocated with at least n elements
 * @note risk_aversion must be positive
 * @note time_horizon must be positive
 * @note arrival_rates must be positive
 * @note volatilities must be non-negative
 * @note For n >= 1000, use this batch function to amortize cgo overhead
 * @note NaN/Inf in inputs will propagate to outputs
 */
FC_API fc_status_t fc_market_maker_quotes(
    double* bid_prices,
    double* ask_prices,
    const double* mid_prices,
    const double* inventories,
    const double* volatilities,
    const double* arrival_rates,
    double risk_aversion,
    double time_horizon,
    size_t n
);

/**
 * @brief Calculate reservation price component
 *
 * Computes the reservation price, which is the mid price adjusted for inventory risk.
 * This is an intermediate calculation in the Avellaneda-Stoikov model.
 *
 * Formula: reservation_price = mid - inventory × γ × σ² × T
 *
 * @param[out] reservation_prices Output array of reservation prices (length n)
 * @param[in] mid_prices Input array of current mid prices (length n)
 * @param[in] inventories Input array of current inventory positions (length n)
 * @param[in] volatilities Input array of price volatilities σ (length n)
 * @param[in] risk_aversion Risk aversion coefficient γ
 * @param[in] time_horizon Time horizon T
 * @param[in] n Number of symbols to process
 * @return FC_OK on success, error code otherwise
 *
 * Time complexity: O(n)
 * Space complexity: O(1) additional space
 * Thread safety: Thread-safe
 */
FC_API fc_status_t fc_market_maker_reservation_price(
    double* reservation_prices,
    const double* mid_prices,
    const double* inventories,
    const double* volatilities,
    double risk_aversion,
    double time_horizon,
    size_t n
);

/**
 * @brief Calculate optimal spread component
 *
 * Computes the optimal bid-ask spread based on inventory risk and order flow intensity.
 * This balances the tradeoff between capturing spread and adverse selection risk.
 *
 * Formula: spread = γ × σ² × T + (2/γ) × ln(1 + γ/λ)
 *
 * @param[out] spreads Output array of optimal spreads (length n)
 * @param[in] volatilities Input array of price volatilities σ (length n)
 * @param[in] arrival_rates Input array of order arrival rates λ (length n)
 * @param[in] risk_aversion Risk aversion coefficient γ
 * @param[in] time_horizon Time horizon T
 * @param[in] n Number of symbols to process
 * @return FC_OK on success, error code otherwise
 *
 * Time complexity: O(n)
 * Space complexity: O(1) additional space
 * Thread safety: Thread-safe
 *
 * @note This function uses log() which may be expensive; SIMD optimization is applied
 */
FC_API fc_status_t fc_market_maker_optimal_spread(
    double* spreads,
    const double* volatilities,
    const double* arrival_rates,
    double risk_aversion,
    double time_horizon,
    size_t n
);

#ifdef __cplusplus
}
#endif

#endif // FC_MARKET_MAKER_QUOTES_H
