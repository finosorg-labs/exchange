#ifndef FC_MATH_ERF_H
#define FC_MATH_ERF_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Compute error function erf(x) for float64 array
 *
 * Computes erf(x) = (2/√π) * ∫[0,x] exp(-t²) dt using Abramowitz and Stegun
 * approximation. Achieves maximum error < 1.5e-7 across the entire range.
 *
 * For higher precision requirements, use platform layer's fc_bigfloat_erf().
 *
 * @param input Input array
 * @param output Output array (erf values)
 * @param n Number of elements
 * @return 0 on success, -1 on error
 *
 * @note Output array must be pre-allocated with size n
 * @note Special value handling:
 *       - erf(NaN) = NaN
 *       - erf(+Inf) = 1.0
 *       - erf(-Inf) = -1.0
 *       - erf(0) = 0 exactly
 * @note Thread-safe (no shared state)
 *
 * Time complexity: O(n)
 * Space complexity: O(1) auxiliary space
 */
int fc_math_erf_f64(const double* input, double* output, size_t n);

/**
 * @brief Compute error function for float32 array
 *
 * @param input Input array
 * @param output Output array (erf values)
 * @param n Number of elements
 * @return 0 on success, -1 on error
 *
 * @note See fc_math_erf_f64 for details
 */
int fc_math_erf_f32(const float* input, float* output, size_t n);

#ifdef __cplusplus
}
#endif

#endif
