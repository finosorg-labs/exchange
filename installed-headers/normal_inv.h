#ifndef FC_MATH_NORMAL_INV_H
#define FC_MATH_NORMAL_INV_H

#include "fc_bigfloat.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Compute inverse standard normal cumulative distribution function (quantile function)
 *
 * Computes Φ⁻¹(p) for each probability p in the input array, where Φ is the standard normal CDF.
 * Uses Peter J. Acklam's algorithm (improved Beasley-Springer-Moro) for high accuracy.
 *
 * The algorithm uses rational function approximations:
 * - For central region (0.02425 ≤ p ≤ 0.97575): polynomial ratio around p = 0.5
 * - For tail regions (p < 0.02425 or p > 0.97575): rational approximation based on sqrt(-2*log(p))
 *
 * @param input Input array of probabilities (must be in range (0, 1))
 * @param output Output array of quantiles (standard normal variates)
 * @param n Number of elements
 * @return 0 on success, -1 on error
 *
 * @note Output array must be pre-allocated with size n
 * @note Input values must satisfy 0 < p < 1 (exclusive bounds)
 * @note Special value handling:
 *       - p = 0.5 → 0.0
 *       - p → 0⁺ → -∞
 *       - p → 1⁻ → +∞
 *       - p ≤ 0 or p ≥ 1 → NaN
 *       - p = NaN → NaN
 * @note Thread-safe (no shared state)
 * @note Accuracy: relative error < 1.15e-9 for all p ∈ (0, 1)
 *
 * Time complexity: O(n)
 * Space complexity: O(1) auxiliary space
 */
int fc_math_normal_inv_f64(const double* input, double* output, size_t n);

/**
 * @brief Compute inverse standard normal CDF using Beasley-Springer-Moro algorithm
 *
 * Original Beasley-Springer-Moro algorithm with moderate accuracy.
 * For higher accuracy, use fc_math_normal_inv_f64() which implements Acklam's improved version.
 *
 * @param input Input array of probabilities (must be in range (0, 1))
 * @param output Output array of quantiles (standard normal variates)
 * @param n Number of elements
 * @return 0 on success, -1 on error
 *
 * @note Output array must be pre-allocated with size n
 * @note Accuracy: relative error < 3e-9 for 0.001 < p < 0.999
 * @note Thread-safe (no shared state)
 *
 * Time complexity: O(n)
 * Space complexity: O(1) auxiliary space
 */
int fc_math_normal_inv_bsm_f64(const double* input, double* output, size_t n);

/**
 * @brief Compute inverse standard normal CDF for float32 array
 *
 * @param input Input array of probabilities (must be in range (0, 1))
 * @param output Output array of quantiles
 * @param n Number of elements
 * @return 0 on success, -1 on error
 *
 * @note See fc_math_normal_inv_f64 for details
 * @note Accuracy: relative error < 2e-7 for all p ∈ (0, 1)
 */
int fc_math_normal_inv_f32(const float* input, float* output, size_t n);

/**
 * @brief Compute inverse standard normal CDF for float32 using BSM algorithm
 *
 * @param input Input array of probabilities (must be in range (0, 1))
 * @param output Output array of quantiles
 * @param n Number of elements
 * @return 0 on success, -1 on error
 *
 * @note See fc_math_normal_inv_bsm_f64 for details
 * @note Accuracy: relative error < 4e-7 for 0.001 < p < 0.999
 */
int fc_math_normal_inv_bsm_f32(const float* input, float* output, size_t n);

/**
 * @brief Compute inverse standard normal CDF with high precision using bigfloat
 *
 * Computes Φ⁻¹(p) using arbitrary-precision arithmetic.
 * Uses Newton-Raphson iteration with bigfloat normal CDF/PDF for refinement.
 *
 * @param input Input array of probabilities
 * @param output Array of bigfloat pointers to receive results (must be pre-allocated)
 * @param n Number of elements
 * @param precision_bits Precision in bits (0 = default precision)
 * @return 0 on success, -1 on error
 *
 * @note Each output[i] must be a valid bigfloat pointer created with fc_bigfloat_create
 * @note Thread-safe (no shared state)
 * @note Initial guess from double-precision Beasley-Springer-Moro, refined to target precision
 *
 * Time complexity: O(n)
 */
int fc_math_normal_inv_bigfloat_f64(
    const double* input,
    fc_bigfloat_t* const* output,
    size_t n,
    fc_uint64_t precision_bits
);

#ifdef __cplusplus
}
#endif

#endif
