#ifndef FC_MATH_NORMAL_H
#define FC_MATH_NORMAL_H

#include "fc_bigfloat.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Compute standard normal probability density function (PDF)
 *
 * Computes φ(x) = (1/√(2π)) * exp(-x²/2) for each element in the input array.
 * Uses SIMD-optimized exponential function for high performance.
 *
 * @param input Input array (standard normal variates)
 * @param output Output array (probability densities)
 * @param n Number of elements
 * @return 0 on success, -1 on error
 *
 * @note Output array must be pre-allocated with size n
 * @note Special value handling:
 *       - φ(NaN) = NaN
 *       - φ(±Inf) = 0
 * @note Thread-safe (no shared state)
 *
 * Time complexity: O(n)
 * Space complexity: O(1) auxiliary space
 */
int fc_math_normal_pdf_f64(const double* input, double* output, size_t n);

/**
 * @brief Compute standard normal cumulative distribution function (CDF)
 *
 * Computes Φ(x) = (1/2) * [1 + erf(x/√2)] for each element in the input array.
 * Uses Abramowitz-Stegun approximation for erf() with ~1.5e-7 accuracy.
 *
 * For higher precision, use fc_math_normal_cdf_bigfloat_f64().
 *
 * @param input Input array (standard normal variates)
 * @param output Output array (cumulative probabilities)
 * @param n Number of elements
 * @return 0 on success, -1 on error
 *
 * @note Output array must be pre-allocated with size n
 * @note Special value handling:
 *       - Φ(NaN) = NaN
 *       - Φ(+Inf) = 1.0
 *       - Φ(-Inf) = 0.0
 * @note Thread-safe (no shared state)
 *
 * Time complexity: O(n)
 * Space complexity: O(1) auxiliary space
 */
int fc_math_normal_cdf_f64(const double* input, double* output, size_t n);

/**
 * @brief Compute standard normal PDF for float32 array
 *
 * @param input Input array (standard normal variates)
 * @param output Output array (probability densities)
 * @param n Number of elements
 * @return 0 on success, -1 on error
 *
 * @note See fc_math_normal_pdf_f64 for details
 */
int fc_math_normal_pdf_f32(const float* input, float* output, size_t n);

/**
 * @brief Compute standard normal CDF for float32 array
 *
 * @param input Input array (standard normal variates)
 * @param output Output array (cumulative probabilities)
 * @param n Number of elements
 * @return 0 on success, -1 on error
 *
 * @note See fc_math_normal_cdf_f64 for details
 */
int fc_math_normal_cdf_f32(const float* input, float* output, size_t n);

/**
 * @brief Compute standard normal PDF with high precision using bigfloat
 *
 * Computes φ(x) = (1/√(2π)) * exp(-x²/2) using arbitrary-precision arithmetic.
 * Provides configurable precision for financial-grade calculations.
 *
 * @param input Input array (standard normal variates)
 * @param output Array of bigfloat pointers to receive results (must be pre-allocated)
 * @param n Number of elements
 * @param precision_bits Precision in bits (0 = default precision)
 * @return 0 on success, -1 on error
 *
 * @note Each output[i] must be a valid bigfloat pointer created with fc_bigfloat_create
 * @note Thread-safe (no shared state)
 *
 * Time complexity: O(n)
 */
int fc_math_normal_pdf_bigfloat_f64(
    const double* input,
    fc_bigfloat_t* const* output,
    size_t n,
    fc_uint64_t precision_bits
);

/**
 * @brief Compute standard normal CDF with high precision using bigfloat
 *
 * Computes Φ(x) = (1/2) * [1 + erf(x/√2)] using arbitrary-precision arithmetic.
 * Uses platform layer's fc_bigfloat_erf() for < 1 ULP accuracy.
 *
 * @param input Input array (standard normal variates)
 * @param output Array of bigfloat pointers to receive results (must be pre-allocated)
 * @param n Number of elements
 * @param precision_bits Precision in bits (0 = default precision)
 * @return 0 on success, -1 on error
 *
 * @note Each output[i] must be a valid bigfloat pointer created with fc_bigfloat_create
 * @note Thread-safe (no shared state)
 *
 * Time complexity: O(n)
 */
int fc_math_normal_cdf_bigfloat_f64(
    const double* input,
    fc_bigfloat_t* const* output,
    size_t n,
    fc_uint64_t precision_bits
);

#ifdef __cplusplus
}
#endif

#endif
