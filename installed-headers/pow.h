#ifndef FC_MATH_POW_H
#define FC_MATH_POW_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Compute power function for float64 arrays
 *
 * Computes pow(x, y) for each pair of elements in the input arrays using
 * vectorized exp(y * log(x)) computation.
 *
 * @param base Base array (x values)
 * @param exponent Exponent array (y values)
 * @param output Output array (x^y values)
 * @param n Number of elements
 * @return 0 on success, -1 on error
 *
 * @note All arrays must be pre-allocated with size n
 * @note Special value handling:
 *       - pow(x, 0) = 1 for any x (including NaN)
 *       - pow(1, y) = 1 for any y (including NaN)
 *       - pow(x, NaN) = NaN for x != 1
 *       - pow(NaN, y) = NaN for y != 0
 *       - pow(x, y) = NaN for x < 0 and non-integer y
 *       - pow(0, y) = 0 for y > 0, +Inf for y < 0
 *       - pow(+Inf, y) = +Inf for y > 0, 0 for y < 0
 * @note Thread-safe (no shared state)
 *
 * Time complexity: O(n)
 * Space complexity: O(1) auxiliary space
 */
int fc_math_pow_f64(const double* base, const double* exponent, double* output, size_t n);

/**
 * @brief Compute power function for float32 arrays
 *
 * Computes pow(x, y) for each pair of elements in the input arrays using
 * vectorized exp(y * log(x)) computation.
 *
 * @param base Base array (x values)
 * @param exponent Exponent array (y values)
 * @param output Output array (x^y values)
 * @param n Number of elements
 * @return 0 on success, -1 on error
 *
 * @note All arrays must be pre-allocated with size n
 * @note Special value handling: same as fc_math_pow_f64
 * @note Thread-safe (no shared state)
 *
 * Time complexity: O(n)
 * Space complexity: O(1) auxiliary space
 */
int fc_math_pow_f32(const float* base, const float* exponent, float* output, size_t n);

/**
 * @brief Compute power with scalar exponent for float64 array
 *
 * Computes pow(x, y) for each element x in base array with fixed exponent y.
 *
 * @param base Base array (x values)
 * @param exponent Scalar exponent (y value)
 * @param output Output array (x^y values)
 * @param n Number of elements
 * @return 0 on success, -1 on error
 *
 * @note Arrays must be pre-allocated with size n
 * @note Special value handling: same as fc_math_pow_f64
 * @note Thread-safe (no shared state)
 *
 * Time complexity: O(n)
 * Space complexity: O(1) auxiliary space
 */
int fc_math_pow_scalar_f64(const double* base, double exponent, double* output, size_t n);

/**
 * @brief Compute power with scalar exponent for float32 array
 *
 * Computes pow(x, y) for each element x in base array with fixed exponent y.
 *
 * @param base Base array (x values)
 * @param exponent Scalar exponent (y value)
 * @param output Output array (x^y values)
 * @param n Number of elements
 * @return 0 on success, -1 on error
 *
 * @note Arrays must be pre-allocated with size n
 * @note Special value handling: same as fc_math_pow_f32
 * @note Thread-safe (no shared state)
 *
 * Time complexity: O(n)
 * Space complexity: O(1) auxiliary space
 */
int fc_math_pow_scalar_f32(const float* base, float exponent, float* output, size_t n);

#ifdef __cplusplus
}
#endif

#endif
