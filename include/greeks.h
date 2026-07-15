#ifndef FC_OPTIM_GREEKS_H
#define FC_OPTIM_GREEKS_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Compute Black-Scholes Greeks for a batch of options.
 *
 * Calculates the five primary Greeks (Delta, Gamma, Vega, Theta, Rho) for
 * European options using the Black-Scholes model. All calculations use
 * SIMD-optimized mathematical functions for high performance.
 *
 * Greeks formulas (for call options):
 * - Delta = N(d1)
 * - Gamma = N'(d1) / (S·σ·√T)
 * - Vega = S·N'(d1)·√T  [Note: per unit volatility, not per percentage point]
 * - Theta = -S·N'(d1)·σ/(2√T) - r·K·e^(-rT)·N(d2)
 * - Rho = K·T·e^(-rT)·N(d2)
 *
 * For put options:
 * - Delta = N(d1) - 1
 * - Gamma = same as call
 * - Vega = same as call  [Note: per unit volatility, not per percentage point]
 * - Theta = -S·N'(d1)·σ/(2√T) + r·K·e^(-rT)·N(-d2)
 * - Rho = -K·T·e^(-rT)·N(-d2)
 *
 * Where:
 * - d1 = [ln(S/K) + (r + σ²/2)T] / (σ√T)
 * - d2 = d1 - σ√T
 * - N(x) is the standard normal CDF
 * - N'(x) is the standard normal PDF
 *
 * @param n Number of options to process. Must be > 0.
 * @param S Spot prices (n elements). Must be non-NULL and all > 0.
 * @param K Strike prices (n elements). Must be non-NULL and all > 0.
 * @param T Times to expiration in years (n elements). Must be non-NULL and all > 0.
 * @param r Risk-free rates (n elements). Must be non-NULL.
 * @param sigma Volatilities (n elements). Must be non-NULL and all > 0.
 * @param is_call Option types (n elements): 1 for call, 0 for put. Must be non-NULL.
 * @param delta Output: Delta values (n elements). Can be NULL if not needed.
 * @param gamma Output: Gamma values (n elements). Can be NULL if not needed.
 * @param vega Output: Vega values (n elements). Can be NULL if not needed.
 * @param theta Output: Theta values (n elements). Can be NULL if not needed.
 * @param rho Output: Rho values (n elements). Can be NULL if not needed.
 * @return 0 on success
 *         -1 if required pointers are NULL (S, K, T, r, sigma, is_call)
 *         -2 if n == 0
 *         -3 if any input value is invalid (S/K/sigma <= 0, T <= 0, r is NaN/Inf)
 *
 * Time complexity: O(n)
 * Space complexity: O(n) temporary space (managed by arena allocator)
 * Thread safety: Thread-safe if inputs are not shared across threads
 *
 * @note Performance target: 5000 options in < 1ms on modern x86_64 CPU with AVX2
 * @note Precision: Relative error < 1e-12 compared to analytical solution
 * @note All output arrays must be pre-allocated by caller if non-NULL
 * @note At least one output pointer must be non-NULL, otherwise the function
 *       does nothing useful
 */
int fc_optim_greeks_batch(
    size_t n,
    const double* S,
    const double* K,
    const double* T,
    const double* r,
    const double* sigma,
    const int* is_call,
    double* delta,
    double* gamma,
    double* vega,
    double* theta,
    double* rho
);

#ifdef __cplusplus
}
#endif

#endif
